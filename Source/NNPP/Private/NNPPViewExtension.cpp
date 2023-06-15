// Fill out your copyright notice in the Description page of Project Settings.

#include "NNPPViewExtension.h"

#include "CopyBufferToTexCS.h"
#include "CopyTexToBufferCS.h"
#include "CopyTexturePS.h"
#include "NNECore.h"
#include "NNECoreModelData.h"
#include "NNECoreRuntimeRDG.h"
#include "NNPPSettings.h"
#include "Renderer/Private/ScreenPass.h"
#include "Renderer/Private/PostProcess/PostProcessMaterial.h"
#include "UObject/WeakInterfacePtr.h"

using namespace UE::NNECore;

FNNPPViewExtension& FNNPPViewExtension::Get()
{
	static TSharedPtr<FNNPPViewExtension> Instance;

	if (!Instance.IsValid())
	{
		 Instance = FSceneViewExtensions::NewExtension<FNNPPViewExtension>();
	}

	return *Instance;
}

FNNPPViewExtension::FNNPPViewExtension(const FAutoRegister& AutoRegister) :
	FSceneViewExtensionBase{AutoRegister}
{
	InitializeModel();
}

bool FNNPPViewExtension::IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const
{
	const UNNPPSettings* Settings = GetDefault<UNNPPSettings>();
	return Settings->bEnabled && ModelRDG.IsValid();
}

int32 FNNPPViewExtension::GetPriority() const
{
	// Comes at the last. Wait until everything has finished
	return INT32_MIN;
}

void FNNPPViewExtension::SubscribeToPostProcessingPass(EPostProcessingPass Pass,
	FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled)
{
	if (Pass == EPostProcessingPass::Tonemap)
	{
		InOutPassCallbacks.Add(
			FAfterPassCallbackDelegate::CreateRaw(
				this, &FNNPPViewExtension::PostProcessPassAfterTonemap_RenderThread));
	}
}

void FNNPPViewExtension::InitializeModel()
{
	ModelRDG = nullptr;

	const TWeakInterfacePtr<INNERuntimeRDG> RuntimeRDG = GetRuntime<INNERuntimeRDG>(TEXT("NNERuntimeRDGHlsl"));
	if (!RuntimeRDG.IsValid())
	{
		return;
	}

	const TWeakInterfacePtr<INNERuntime> NNERuntime = UE::NNECore::GetRuntime<INNERuntime>(TEXT("NNERuntimeRDGHlsl"));
	if (!NNERuntime.IsValid())
	{
		return;
	}

	const UNNPPSettings* Settings = GetDefault<UNNPPSettings>();
	if (!Settings->NNPPModelPath.IsValid())
	{
		return;
	}

	UNNEModelData* ModelData = Cast<UNNEModelData>(Settings->NNPPModelPath.TryLoad());
	if (!IsValid(ModelData))
	{
		return;
	}

	ModelRDG = RuntimeRDG->CreateModelRDG(ModelData);
}

FScreenPassTexture FNNPPViewExtension::PostProcessPassAfterTonemap_RenderThread(FRDGBuilder& GraphBuilder,
	const FSceneView& View, const FPostProcessMaterialInputs& Inputs)
{
	check(View.bIsViewInfo);

	const FViewInfo& ViewInfo = StaticCast<const FViewInfo&>(View);
	const FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);

	const TConstArrayView<FTensorDesc> InputTensorDescs = ModelRDG->GetInputTensorDescs();
	const FTensorShape InputTensorShape = FTensorShape::MakeFromSymbolic(InputTensorDescs[0].GetShape());

	// Tensor shape in the format of [BatchSize, ChannelSize, Height, Width]
	const int32 ModelInputX = InputTensorShape.GetData()[3];
	const int32 ModelInputY = InputTensorShape.GetData()[2];
	const FIntPoint TextureSize(ModelInputX, ModelInputY);

	// Step 1: Downscale scene color
	FScreenPassTexture SceneColor = Inputs.GetInput(EPostProcessMaterialInput::SceneColor);
	check(SceneColor.IsValid());
	const FRDGTextureRef SceneColorRDG = SceneColor.Texture;

	const FRDGTextureDesc DownscaledTexDesc = FRDGTextureDesc::Create2D(
		TextureSize,
		PF_FloatRGB,
		FClearValueBinding::None,
		TexCreate_ShaderResource | TexCreate_RenderTargetable | TexCreate_UAV);
	FRDGTextureRef DownscaledSceneColor = GraphBuilder.CreateTexture(DownscaledTexDesc, TEXT("NNPP_DownscaledSceneColor"));
	FRDGTextureSRVRef DownscaledSceneColorSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(DownscaledSceneColor));
	FRDGTextureUAVRef DownscaledSceneColorUAV = GraphBuilder.CreateUAV(DownscaledSceneColor);

	{
		const FScreenPassTextureViewport InputViewport{SceneColorRDG};
		const FScreenPassTextureViewport OutputViewport{DownscaledSceneColor};

		FCopyTexturePS::FParameters* CopyTextureParam = GraphBuilder.AllocParameters<FCopyTexturePS::FParameters>();
		CopyTextureParam->InputTexture = SceneColor.Texture;
		CopyTextureParam->InputSampler = TStaticSamplerState<>::GetRHI();
		CopyTextureParam->RenderTargets[0] = FRenderTargetBinding{DownscaledSceneColor, ERenderTargetLoadAction::ELoad};

		const TShaderMapRef<FCopyTexturePS> CopyTexturePS{GlobalShaderMap};

		AddDrawScreenPass(
			GraphBuilder,
			RDG_EVENT_NAME("NNPP_DownscaleSceneTexture"),
			ViewInfo,
			OutputViewport,
			InputViewport,
			CopyTexturePS,
			CopyTextureParam);
	}

	// Step 2: Create input buffer
	constexpr int32 BytesPerElement = sizeof(float);
	const int32 NumElements = TextureSize.X * TextureSize.Y * 3; // RGB
	FRDGBufferRef InputBuffer = GraphBuilder.CreateBuffer(
		FRDGBufferDesc::CreateStructuredDesc(BytesPerElement, NumElements),
		TEXT("NNPP_InputBuffer"));
	FRDGBufferUAVRef InputBufferUAV = GraphBuilder.CreateUAV(InputBuffer, PF_R32_FLOAT);

	FCopyTexToBufferCS::FParameters* CopyTexToBufferParam = GraphBuilder.AllocParameters<FCopyTexToBufferCS::FParameters>();
	CopyTexToBufferParam->InputTexture = DownscaledSceneColorSRV;
	CopyTexToBufferParam->OutputBuffer = InputBufferUAV;
	CopyTexToBufferParam->InputDim = FIntVector{TextureSize.X, TextureSize.Y, 3}; // RGB channel

	const TShaderMapRef<FCopyTexToBufferCS> CopyTexToBufferCS{GlobalShaderMap};

	constexpr int32 ThreadPerWarp = 32;
	FIntVector GroupCount;
	GroupCount.X = FMath::DivideAndRoundUp(TextureSize.X, ThreadPerWarp);
	GroupCount.Y = FMath::DivideAndRoundUp(TextureSize.Y, ThreadPerWarp);
	GroupCount.Z = 1;

	FComputeShaderUtils::AddPass(
		GraphBuilder,
		RDG_EVENT_NAME("NNPP_SetupInputBuffer"),
		CopyTexToBufferCS,
		CopyTexToBufferParam,
		GroupCount);

	// Step 3: Run neural network
	FRDGBufferRef OutputBuffer = GraphBuilder.CreateBuffer(
		FRDGBufferDesc::CreateStructuredDesc(BytesPerElement, NumElements),
		TEXT("NNPP_OutputBuffer"));
	FRDGBufferSRVRef OutputBufferSRV = GraphBuilder.CreateSRV(OutputBuffer, PF_R32_FLOAT);

	const FTensorBindingRDG InputBinding{InputBuffer};
	const FTensorBindingRDG OutputBinding{OutputBuffer};

	ModelRDG->SetInputTensorShapes({InputTensorShape});
	ModelRDG->EnqueueRDG(GraphBuilder, {InputBinding}, {OutputBinding});

	// Step 4: Copy output buffer to downscaled scene color
	FCopyBufferToTexCS::FParameters* CopyBufferToTexParam = GraphBuilder.AllocParameters<FCopyBufferToTexCS::FParameters>();
	CopyBufferToTexParam->InputBuffer = OutputBufferSRV;
	CopyBufferToTexParam->OutputTexture = DownscaledSceneColorUAV;
	CopyBufferToTexParam->InputDim = FIntVector{TextureSize.X, TextureSize.Y, 3}; // RGB channel

	const TShaderMapRef<FCopyBufferToTexCS> CopyBufferToTexCS{GlobalShaderMap};

	FComputeShaderUtils::AddPass(
		GraphBuilder,
		RDG_EVENT_NAME("NNPP_SetupOutputTexture"),
		CopyBufferToTexCS,
		CopyBufferToTexParam,
		GroupCount);

	// Step 5: Copy to output render target
	FScreenPassRenderTarget OutputRenderTarget = Inputs.OverrideOutput;

	// If the override output is provided it means that this is the last pass in post processing.
	if (!OutputRenderTarget.IsValid())
	{
		OutputRenderTarget = FScreenPassRenderTarget::CreateFromInput(
			GraphBuilder,
			SceneColor,
			ViewInfo.GetOverwriteLoadAction(),
			TEXT("OutputRenderTarget"));
	}

	{
		const FScreenPassTextureViewport InputViewport{DownscaledSceneColor};
		const FScreenPassTextureViewport OutputViewport(static_cast<FScreenPassTexture>(OutputRenderTarget));

		FCopyTexturePS::FParameters* Parameters = GraphBuilder.AllocParameters<FCopyTexturePS::FParameters>();
		Parameters->InputTexture = DownscaledSceneColor;
		Parameters->InputSampler = TStaticSamplerState<>::GetRHI();
		Parameters->RenderTargets[0] = OutputRenderTarget.GetRenderTargetBinding();

		const TShaderMapRef<FCopyTexturePS> CopyTexturePS{GlobalShaderMap};

		AddDrawScreenPass(
			GraphBuilder,
			RDG_EVENT_NAME("NNPP_CopyToFrameBuffer"),
			ViewInfo,
			OutputViewport,
			InputViewport,
			CopyTexturePS,
			Parameters);
	}

	return static_cast<FScreenPassTexture>(MoveTemp(OutputRenderTarget));
}
// Fill out your copyright notice in the Description page of Project Settings.

#include "Layers/TanhLayer.h"
#include "Layers/NNLayerUtils.h"

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FTanhLayerComputeShaderParameters, )
	SHADER_PARAMETER(FIntVector, InputDim)
	SHADER_PARAMETER(FIntVector, OutputDim)
	SHADER_PARAMETER(FIntVector, InputDimIndexMult)
END_GLOBAL_SHADER_PARAMETER_STRUCT()
IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FTanhLayerComputeShaderParameters, "TanhLayerUniform");

class FTanhLayerComputeShader : public FGlobalShader
{
public:

	using FParameters = FTanhLayerComputeShaderParameters;
	DECLARE_NNLAYER_COMPUTE_SHADER(FTanhLayerComputeShader);

	FTanhLayerComputeShader() {}
	FTanhLayerComputeShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		InputBuffer.Bind(Initializer.ParameterMap, TEXT("InputBuffer"));
		OutputBuffer.Bind(Initializer.ParameterMap, TEXT("OutputBuffer"));
	}

	void BindShaderBuffers(
		FRHICommandList&           RHICmdList,
		FUnorderedAccessViewRHIRef OutputBufferUAV,
		FShaderResourceViewRHIRef  InputBufferSRV)
	{
		FRHIComputeShader* ComputeShaderRHI = RHICmdList.GetBoundComputeShader();

		SetUAVParameter(RHICmdList, ComputeShaderRHI, OutputBuffer, OutputBufferUAV);
		SetSRVParameter(RHICmdList, ComputeShaderRHI, InputBuffer, InputBufferSRV);
	}

	void UnbindShaderBuffers(FRHICommandList& RHICmdList)
	{
		FRHIComputeShader* ComputeShaderRHI = RHICmdList.GetBoundComputeShader();

		SetUAVParameter(RHICmdList, ComputeShaderRHI, OutputBuffer, FUnorderedAccessViewRHIRef());
		SetSRVParameter(RHICmdList, ComputeShaderRHI, InputBuffer, FShaderResourceViewRHIRef());
	}

private:

	LAYOUT_FIELD(FShaderResourceParameter, InputBuffer);
	LAYOUT_FIELD(FShaderResourceParameter, OutputBuffer);
};

IMPLEMENT_GLOBAL_SHADER(FTanhLayerComputeShader, "/Plugin/NNPP/TanhLayer.usf", "TanhLayer", SF_Compute);


FTanhLayer::FTanhLayer() :
	FNNLayerBase(ENNLayerType::Tanh)
{

}

FTanhLayer::~FTanhLayer()
{

}

void FTanhLayer::SetupLayer(FIntVector InInputDim)
{
	FNNLayerBase::SetupLayer(InInputDim);

	OutputDim = InInputDim;

	// Release all output buffer resources
	FNNLayerBase::ReleaseRenderResources();

	FRHIResourceCreateInfo CreateInfo;

	OutputBuffer = RHICreateStructuredBuffer(
		sizeof(float),                                             // Stride
		sizeof(float) * OutputDim.X * OutputDim.Y * OutputDim.Z,   // Size
		BUF_UnorderedAccess | BUF_ShaderResource,                  // Usage
		CreateInfo                                                 // Create info
	);
	OutputBufferUAV = RHICreateUnorderedAccessView(OutputBuffer, true, false);
	OutputBufferSRV = RHICreateShaderResourceView(OutputBuffer);
}

void FTanhLayer::ReleaseRenderResources()
{
	FNNLayerBase::ReleaseRenderResources();
}

void FTanhLayer::RunLayer_RenderThread(
	FRHICommandList&          RHICmdList,
	FShaderResourceViewRHIRef InputBufferSRV,
	FShaderResourceViewRHIRef OptionalInputBufferSRV /*= nullptr*/)
{
	check(IsInRenderingThread());

	// Bind shader textures
	TShaderMapRef<FTanhLayerComputeShader> TanhLayerCS(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
	RHICmdList.SetComputeShader(TanhLayerCS.GetComputeShader());

	TanhLayerCS->BindShaderBuffers(RHICmdList, OutputBufferUAV, InputBufferSRV);

	// Bind shader uniform
	FTanhLayerComputeShader::FParameters UniformParam;
	UniformParam.InputDim          = InputDim;
	UniformParam.OutputDim         = OutputDim;
	UniformParam.InputDimIndexMult = FIntVector(OutputDim.X, 1, 1);
	TanhLayerCS->SetShaderParameters(RHICmdList, UniformParam);

	// Dispatch shader
	const int32 ThreadGroupCountX = FMath::CeilToInt(OutputDim.Y * OutputDim.Z / 32.0f);
	const int32 ThreadGroupCountY = OutputDim.X;
	const int32 ThreadGroupCountZ = 1;
	DispatchComputeShader(RHICmdList, TanhLayerCS, ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);

	// Unbind shader textures
	TanhLayerCS->UnbindShaderBuffers(RHICmdList);

	// Make UAV safe for read
	RHICmdList.TransitionResource(
		EResourceTransitionAccess::EReadable,
		EResourceTransitionPipeline::EComputeToCompute,
		OutputBufferUAV,
		nullptr);
}

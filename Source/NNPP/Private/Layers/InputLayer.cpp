// Fill out your copyright notice in the Description page of Project Settings.

#include "Layers/InputLayer.h"
#include "Layers/NNLayerUtils.h"

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FInputLayerComputeShaderParameters, )
	SHADER_PARAMETER(int32, InputDimX)
	SHADER_PARAMETER(int32, InputDimY)
	SHADER_PARAMETER(int32, InputDimZ)
	SHADER_PARAMETER(int32, InputDimIndexMultX)
	SHADER_PARAMETER(int32, InputDimIndexMultY)
	SHADER_PARAMETER(int32, InputDimIndexMultZ)
END_GLOBAL_SHADER_PARAMETER_STRUCT()
IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FInputLayerComputeShaderParameters, "InputLayerUniform");

class FInputLayerComputeShader : public FGlobalShader
{
public:

	using FParameters = FInputLayerComputeShaderParameters;
	DECLARE_NNLAYER_COMPUTE_SHADER(FInputLayerComputeShader);

	FInputLayerComputeShader() {}
	FInputLayerComputeShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		InputTexture.Bind(Initializer.ParameterMap, TEXT("InputTexture"));
		OutputBuffer.Bind(Initializer.ParameterMap, TEXT("OutputBuffer"));
	}

	void BindShaderBuffers(
		FRHICommandList&           RHICmdList,
		FUnorderedAccessViewRHIRef OutputBufferUAV,
		FShaderResourceViewRHIRef  InputTextureSRV)
	{
		FRHIComputeShader* ComputeShaderRHI = RHICmdList.GetBoundComputeShader();

		SetUAVParameter(RHICmdList, ComputeShaderRHI, OutputBuffer, OutputBufferUAV);
		SetSRVParameter(RHICmdList, ComputeShaderRHI, InputTexture, InputTextureSRV);
	}

	void UnbindShaderBuffers(FRHICommandList& RHICmdList)
	{
		FRHIComputeShader* ComputeShaderRHI = RHICmdList.GetBoundComputeShader();

		SetUAVParameter(RHICmdList, ComputeShaderRHI, OutputBuffer, FUnorderedAccessViewRHIRef());
		SetSRVParameter(RHICmdList, ComputeShaderRHI, InputTexture, FShaderResourceViewRHIRef());
	}

private:

	LAYOUT_FIELD(FShaderResourceParameter, InputTexture);
	LAYOUT_FIELD(FShaderResourceParameter, OutputBuffer);
};

IMPLEMENT_GLOBAL_SHADER(FInputLayerComputeShader, "/Plugin/NNPP/InputLayer.usf", "InputLayer", SF_Compute);

FInputLayer::FInputLayer() :
	FNNLayerBase()
{

}

FInputLayer::~FInputLayer()
{

}

void FInputLayer::SetupLayer(FIntVector InInputDim)
{
	FNNLayerBase::SetupLayer(InInputDim);
}

void FInputLayer::ReleaseResource()
{

}

void FInputLayer::RunLayer_RenderThread(
	FRHICommandList&          RHICmdList,
	FShaderResourceViewRHIRef InputBufferSRV,
	FShaderResourceViewRHIRef OptionalInputBufferSRV /*= nullptr*/)
{
	check(IsInRenderingThread());

	// Bind shader textures
	TShaderMapRef<FInputLayerComputeShader> InputLayerCS(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
	RHICmdList.SetComputeShader(InputLayerCS.GetComputeShader());

	InputLayerCS->BindShaderBuffers(RHICmdList, OutputBufferUAV, InputBufferSRV);

	// Bind shader uniform
	FInputLayerComputeShader::FParameters UniformParam;
	UniformParam.InputDimX          = InputDim.X;
	UniformParam.InputDimY          = InputDim.Y;
	UniformParam.InputDimZ          = InputDim.Z;
	UniformParam.InputDimIndexMultX = InputDim.Y * InputDim.Z;
	UniformParam.InputDimIndexMultY = InputDim.Z;
	UniformParam.InputDimIndexMultZ = 1;
	InputLayerCS->SetShaderParameters(RHICmdList, UniformParam);

	// Dispatch shader
	const int32 ThreadGroupCountX = FMath::CeilToInt(InputDim.X / 32.0f);
	const int32 ThreadGroupCountY = FMath::CeilToInt(InputDim.Y / 32.0f);
	const int32 ThreadGroupCountZ = 1;
	DispatchComputeShader(RHICmdList, InputLayerCS, ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);

	// Unbind shader textures
	InputLayerCS->UnbindShaderBuffers(RHICmdList);

	// Make UAV safe for read
	RHICmdList.TransitionResource(
		EResourceTransitionAccess::EReadable,
		EResourceTransitionPipeline::EComputeToCompute,
		OutputBufferUAV,
		nullptr);
}

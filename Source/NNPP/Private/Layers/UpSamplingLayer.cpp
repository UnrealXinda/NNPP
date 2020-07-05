// Fill out your copyright notice in the Description page of Project Settings.

#include "Layers/UpSamplingLayer.h"
#include "Layers/NNLayerUtils.h"

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FUpSamplingLayerComputeShaderParameters, )
	SHADER_PARAMETER(FIntVector, InputDim)
	SHADER_PARAMETER(FIntVector, OutputDim)
	SHADER_PARAMETER(FIntVector, InputDimIndexMult)
	SHADER_PARAMETER(FIntVector, OutputDimIndexMult)
	SHADER_PARAMETER(FIntPoint,  Size)
END_GLOBAL_SHADER_PARAMETER_STRUCT()
IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FUpSamplingLayerComputeShaderParameters, "UpSamplingLayerUniform");

class FUpSamplingLayerComputeShader : public FGlobalShader
{
public:

	using FParameters = FUpSamplingLayerComputeShaderParameters;
	DECLARE_NNLAYER_COMPUTE_SHADER(FUpSamplingLayerComputeShader);

	FUpSamplingLayerComputeShader() {}
	FUpSamplingLayerComputeShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
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

IMPLEMENT_GLOBAL_SHADER(FUpSamplingLayerComputeShader, "/Plugin/NNPP/UpSamplingLayer.usf", "UpSamplingLayer", SF_Compute);

FUpSamplingLayer::FUpSamplingLayer() :
	FNNLayerBase(ENNLayerType::UpSampling)
{

}

FUpSamplingLayer::~FUpSamplingLayer()
{

}

void FUpSamplingLayer::SetupLayer(FIntVector InInputDim)
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

void FUpSamplingLayer::ReleaseRenderResources()
{
	FNNLayerBase::ReleaseRenderResources();
}

void FUpSamplingLayer::RunLayer_RenderThread(
	FRHICommandList&          RHICmdList,
	FShaderResourceViewRHIRef InputBufferSRV,
	FShaderResourceViewRHIRef OptionalInputBufferSRV /*= nullptr*/)
{
check(IsInRenderingThread());

	// Bind shader textures
	TShaderMapRef<FUpSamplingLayerComputeShader> UpSamplingLayerCS(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
	RHICmdList.SetComputeShader(UpSamplingLayerCS.GetComputeShader());

	UpSamplingLayerCS->BindShaderBuffers(RHICmdList, OutputBufferUAV, InputBufferSRV);

	// Bind shader uniform
	FUpSamplingLayerComputeShader::FParameters UniformParam;
	UniformParam.InputDim           = InputDim;
	UniformParam.OutputDim          = OutputDim;
	UniformParam.InputDimIndexMult  = FIntVector(InputDim.Y * InputDim.Z, InputDim.Z, 1);
	UniformParam.OutputDimIndexMult = FIntVector(OutputDim.Y * OutputDim.Z, OutputDim.Z, 1);
	UniformParam.Size               = Size;
	UpSamplingLayerCS->SetShaderParameters(RHICmdList, UniformParam);

	// Dispatch shader
	const int32 ThreadGroupCountX = FMath::CeilToInt(OutputDim.X / 32.0f);
	const int32 ThreadGroupCountY = FMath::CeilToInt(OutputDim.Y / 32.0f);
	const int32 ThreadGroupCountZ = OutputDim.Z;
	DispatchComputeShader(RHICmdList, UpSamplingLayerCS, ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);

	// Unbind shader textures
	UpSamplingLayerCS->UnbindShaderBuffers(RHICmdList);

	// Make UAV safe for read
	RHICmdList.TransitionResource(
		EResourceTransitionAccess::EReadable,
		EResourceTransitionPipeline::EComputeToCompute,
		OutputBufferUAV,
		nullptr);
}

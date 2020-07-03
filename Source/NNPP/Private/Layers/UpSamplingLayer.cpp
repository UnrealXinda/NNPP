// Fill out your copyright notice in the Description page of Project Settings.

#include "Layers/UpSamplingLayer.h"
#include "Layers/NNLayerUtils.h"

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FUpSamplingLayerComputeShaderParameters, )
	SHADER_PARAMETER(int32, InputDimX)
	SHADER_PARAMETER(int32, InputDimY)
	SHADER_PARAMETER(int32, InputDimZ)
	SHADER_PARAMETER(int32, OutputDimX)
	SHADER_PARAMETER(int32, OutputDimY)
	SHADER_PARAMETER(int32, OutputDimZ)
	SHADER_PARAMETER(int32, InputDimIndexMultX)
	SHADER_PARAMETER(int32, InputDimIndexMultY)
	SHADER_PARAMETER(int32, InputDimIndexMultZ)
	SHADER_PARAMETER(int32, OutputDimIndexMultX)
	SHADER_PARAMETER(int32, OutputDimIndexMultY)
	SHADER_PARAMETER(int32, OutputDimIndexMultZ)
	SHADER_PARAMETER(int32, SizeX)
	SHADER_PARAMETER(int32, SizeY)
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
	FNNLayerBase()
{

}

FUpSamplingLayer::~FUpSamplingLayer()
{

}

void FUpSamplingLayer::SetupLayer(FIntVector InInputDim)
{
	FNNLayerBase::SetupLayer(InInputDim);
}

void FUpSamplingLayer::ReleaseResource()
{

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
	UniformParam.InputDimX           = InputDim.X;
	UniformParam.InputDimY           = InputDim.Y;
	UniformParam.InputDimZ           = InputDim.Z;
	UniformParam.OutputDimX          = OutputDim.X;
	UniformParam.OutputDimY          = OutputDim.Y;
	UniformParam.OutputDimZ          = OutputDim.Z;
	UniformParam.InputDimIndexMultX  = InputDim.Y * InputDim.Z;
	UniformParam.InputDimIndexMultY  = InputDim.X;
	UniformParam.InputDimIndexMultZ  = 1;
	UniformParam.OutputDimIndexMultX = OutputDim.Y * OutputDim.Z;
	UniformParam.OutputDimIndexMultY = OutputDim.Z;
	UniformParam.OutputDimIndexMultZ = 1;
	UniformParam.SizeX               = Size.X;
	UniformParam.SizeY               = Size.Y;
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

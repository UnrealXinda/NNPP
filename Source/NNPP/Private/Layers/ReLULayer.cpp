// Fill out your copyright notice in the Description page of Project Settings.

#include "Layers/ReLULayer.h"
#include "Layers/NNLayerUtils.h"

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FReLULayerComputeShaderParameters, )
	SHADER_PARAMETER(FIntVector, InputDim)
	SHADER_PARAMETER(FIntVector, OutputDim)
	SHADER_PARAMETER(FIntVector, InputDimIndexMult)
END_GLOBAL_SHADER_PARAMETER_STRUCT()
IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FReLULayerComputeShaderParameters, "ReLULayerUniform");

class FReLULayerComputeShader : public FGlobalShader
{
public:

	using FParameters = FReLULayerComputeShaderParameters;
	DECLARE_NNLAYER_COMPUTE_SHADER(FReLULayerComputeShader);

	FReLULayerComputeShader() {}
	FReLULayerComputeShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
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

IMPLEMENT_GLOBAL_SHADER(FReLULayerComputeShader, "/Plugin/NNPP/ReLULayer.usf", "ReLULayer", SF_Compute);


FReLULayer::FReLULayer() :
	FNNLayerBase(ENNLayerType::ReLU)
{

}

FReLULayer::~FReLULayer()
{

}

void FReLULayer::RunLayer_RenderThread(
	FRHICommandList&           RHICmdList,
	FUnorderedAccessViewRHIRef OutputBufferUAV,
	FShaderResourceViewRHIRef  InputBufferSRV,
	FShaderResourceViewRHIRef  OptionalInputBufferSRV /*= nullptr*/)
{
	check(IsInRenderingThread());

	// Bind shader textures
	TShaderMapRef<FReLULayerComputeShader> ReLULayerCS(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
	RHICmdList.SetComputeShader(ReLULayerCS.GetComputeShader());

	ReLULayerCS->BindShaderBuffers(RHICmdList, OutputBufferUAV, InputBufferSRV);

	// Bind shader uniform
	FReLULayerComputeShader::FParameters UniformParam;
	UniformParam.InputDim          = InputDim;
	UniformParam.OutputDim         = OutputDim;
	UniformParam.InputDimIndexMult = FIntVector(OutputDim.X, 1, 1);
	ReLULayerCS->SetShaderParameters(RHICmdList, UniformParam);

	// Dispatch shader
	const int32 ThreadGroupCountX = FMath::CeilToInt(OutputDim.Y * OutputDim.Z / 32.0f);
	const int32 ThreadGroupCountY = OutputDim.X;
	const int32 ThreadGroupCountZ = 1;
	DispatchComputeShader(RHICmdList, ReLULayerCS, ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);

	// Unbind shader textures
	ReLULayerCS->UnbindShaderBuffers(RHICmdList);

	// Make UAV safe for read
	RHICmdList.TransitionResource(
		EResourceTransitionAccess::EReadable,
		EResourceTransitionPipeline::EComputeToCompute,
		OutputBufferUAV,
		nullptr);
}

// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Layers/BatchNormLayer.h"
#include "Layers/NNLayerUtils.h"

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FBatchNormLayerComputeShaderParameters, )
	SHADER_PARAMETER(int32, InputDimX)
	SHADER_PARAMETER(int32, InputDimY)
	SHADER_PARAMETER(int32, InputDimZ)
	SHADER_PARAMETER(int32, OutputDimX)
	SHADER_PARAMETER(int32, OutputDimY)
	SHADER_PARAMETER(int32, OutputDimZ)
END_GLOBAL_SHADER_PARAMETER_STRUCT()
IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FBatchNormLayerComputeShaderParameters, "BatchNormLayerUniform");

class FBatchNormLayerComputeShader : public FGlobalShader
{
public:

	using FParameters = FBatchNormLayerComputeShaderParameters;
	DECLARE_NNLAYER_COMPUTE_SHADER(FBatchNormLayerComputeShader);

	FBatchNormLayerComputeShader() {}
	FBatchNormLayerComputeShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		InputBuffer.Bind(Initializer.ParameterMap, TEXT("InputBuffer"));
		WeightBuffer.Bind(Initializer.ParameterMap, TEXT("WeightBuffer"));
		OutputBuffer.Bind(Initializer.ParameterMap, TEXT("OutputBuffer"));
	}

	void BindShaderBuffers(
		FRHICommandList&           RHICmdList,
		FUnorderedAccessViewRHIRef OutputBufferUAV,
		FShaderResourceViewRHIRef  InputBufferSRV,
		FShaderResourceViewRHIRef  WeightBufferSRV)
	{
		FRHIComputeShader* ComputeShaderRHI = RHICmdList.GetBoundComputeShader();

		SetUAVParameter(RHICmdList, ComputeShaderRHI, OutputBuffer, OutputBufferUAV);
		SetSRVParameter(RHICmdList, ComputeShaderRHI, InputBuffer, InputBufferSRV);
		SetSRVParameter(RHICmdList, ComputeShaderRHI, WeightBuffer, WeightBufferSRV);
	}

	void UnbindShaderBuffers(FRHICommandList& RHICmdList)
	{
		FRHIComputeShader* ComputeShaderRHI = RHICmdList.GetBoundComputeShader();

		SetUAVParameter(RHICmdList, ComputeShaderRHI, OutputBuffer, FUnorderedAccessViewRHIRef());
		SetSRVParameter(RHICmdList, ComputeShaderRHI, InputBuffer, FShaderResourceViewRHIRef());
		SetSRVParameter(RHICmdList, ComputeShaderRHI, WeightBuffer, FShaderResourceViewRHIRef());
	}

private:

	LAYOUT_FIELD(FShaderResourceParameter, InputBuffer);
	LAYOUT_FIELD(FShaderResourceParameter, WeightBuffer);
	LAYOUT_FIELD(FShaderResourceParameter, OutputBuffer);
};

IMPLEMENT_GLOBAL_SHADER(FBatchNormLayerComputeShader, "/Plugin/NNPP/BatchNormLayer.usf", "BatchNormLayer", SF_Compute);

FBatchNormLayer::FBatchNormLayer() :
	FNNLayerBase()
{

}

FBatchNormLayer::~FBatchNormLayer()
{

}

void FBatchNormLayer::SetupLayer(FIntVector InInputDim)
{
	FNNLayerBase::SetupLayer(InInputDim);
}

void FBatchNormLayer::ReleaseResource()
{

}

void FBatchNormLayer::RunLayer_RenderThread(
	FRHICommandList&          RHICmdList,
	FShaderResourceViewRHIRef InputBufferSRV,
	FShaderResourceViewRHIRef OptionalInputBufferSRV /*= nullptr*/)
{
	check(IsInRenderingThread());

	// Bind shader textures
	TShaderMapRef<FBatchNormLayerComputeShader> BatchNormLayerCS(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
	RHICmdList.SetComputeShader(BatchNormLayerCS.GetComputeShader());

	BatchNormLayerCS->BindShaderBuffers(RHICmdList, OutputBufferUAV, InputBufferSRV, WeightBufferSRV);

	// Bind shader uniform
	FBatchNormLayerComputeShader::FParameters UniformParam;
	UniformParam.InputDimX  = InputDim.X;
	UniformParam.InputDimY  = InputDim.Y;
	UniformParam.InputDimZ  = InputDim.Z;
	UniformParam.OutputDimX = OutputDim.X;
	UniformParam.OutputDimY = OutputDim.Y;
	UniformParam.OutputDimZ = OutputDim.Z;
	BatchNormLayerCS->SetShaderParameters(RHICmdList, UniformParam);

	// Dispatch shader
	const int ThreadGroupCountX = FMath::CeilToInt(OutputDim.X * OutputDim.Y / 32.0f);
	const int ThreadGroupCountY = FMath::CeilToInt(OutputDim.Z);
	const int ThreadGroupCountZ = 1;
	DispatchComputeShader(RHICmdList, BatchNormLayerCS, ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);

	// Unbind shader textures
	BatchNormLayerCS->UnbindShaderBuffers(RHICmdList);

	// Make UAV safe for read
	RHICmdList.TransitionResource(
		EResourceTransitionAccess::EReadable,
		EResourceTransitionPipeline::EComputeToCompute,
		OutputBufferUAV,
		nullptr);
}

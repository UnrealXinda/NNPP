// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Layers/AddLayer.h"
#include "Layers/NNLayerUtils.h"

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FAddLayerComputeShaderParameters, )
	SHADER_PARAMETER(FIntVector, InputDim)
	SHADER_PARAMETER(FIntVector, OutputDim)
	SHADER_PARAMETER(FIntVector, InputDimIndexMult)
	SHADER_PARAMETER(FIntVector, OutputDimIndexMult)
END_GLOBAL_SHADER_PARAMETER_STRUCT()
IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FAddLayerComputeShaderParameters, "AddLayerUniform");

class FAddLayerComputeShader : public FGlobalShader
{
public:

	using FParameters = FAddLayerComputeShaderParameters;
	DECLARE_NNLAYER_COMPUTE_SHADER(FAddLayerComputeShader);

	FAddLayerComputeShader() {}
	FAddLayerComputeShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		InputBuffer1.Bind(Initializer.ParameterMap, TEXT("InputBuffer1"));
		InputBuffer2.Bind(Initializer.ParameterMap, TEXT("InputBuffer2"));
		OutputBuffer.Bind(Initializer.ParameterMap, TEXT("OutputBuffer"));
	}

	void BindShaderBuffers(
		FRHICommandList&           RHICmdList,
		FShaderResourceViewRHIRef  InputBuffer1SRV,
		FShaderResourceViewRHIRef  InputBuffer2SRV,
		FUnorderedAccessViewRHIRef OutputBufferUAV)
	{
		FRHIComputeShader* ComputeShaderRHI = RHICmdList.GetBoundComputeShader();

		SetSRVParameter(RHICmdList, ComputeShaderRHI, InputBuffer1, InputBuffer1SRV);
		SetSRVParameter(RHICmdList, ComputeShaderRHI, InputBuffer2, InputBuffer2SRV);
		SetUAVParameter(RHICmdList, ComputeShaderRHI, OutputBuffer, OutputBufferUAV);
	}

	void UnbindShaderBuffers(FRHICommandList& RHICmdList)
	{
		FRHIComputeShader* ComputeShaderRHI = RHICmdList.GetBoundComputeShader();

		SetSRVParameter(RHICmdList, ComputeShaderRHI, InputBuffer1, FShaderResourceViewRHIRef());
		SetSRVParameter(RHICmdList, ComputeShaderRHI, InputBuffer2, FShaderResourceViewRHIRef());
		SetUAVParameter(RHICmdList, ComputeShaderRHI, OutputBuffer, FUnorderedAccessViewRHIRef());
	}

private:

	LAYOUT_FIELD(FShaderResourceParameter, InputBuffer1);
	LAYOUT_FIELD(FShaderResourceParameter, InputBuffer2);
	LAYOUT_FIELD(FShaderResourceParameter, OutputBuffer);
};

IMPLEMENT_GLOBAL_SHADER(FAddLayerComputeShader, "/Plugin/NNPP/AddLayer.usf", "AddLayer", SF_Compute);

FAddLayer::FAddLayer() :
	FNNLayerBase(ENNLayerType::Add)
{

}

FAddLayer::~FAddLayer()
{

}

void FAddLayer::SetupLayer(FIntVector InInputDim)
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

void FAddLayer::ReleaseRenderResources()
{
	FNNLayerBase::ReleaseRenderResources();
}

void FAddLayer::RunLayer_RenderThread(
	FRHICommandList&          RHICmdList,
	FShaderResourceViewRHIRef InputBufferSRV,
	FShaderResourceViewRHIRef OptionalInputBufferSRV /*= nullptr*/)
{
	check(IsInRenderingThread());

	// Bind shader textures
	TShaderMapRef<FAddLayerComputeShader> AddLayerCS(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
	RHICmdList.SetComputeShader(AddLayerCS.GetComputeShader());

	AddLayerCS->BindShaderBuffers(RHICmdList, InputBufferSRV, OptionalInputBufferSRV, OutputBufferUAV);

	// Bind shader uniform
	FAddLayerComputeShader::FParameters UniformParam;
	UniformParam.InputDim           = InputDim;
	UniformParam.OutputDim          = OutputDim;
	UniformParam.InputDimIndexMult  = FIntVector(InputDim.Y * InputDim.Z, InputDim.Z, 1);
	UniformParam.OutputDimIndexMult = FIntVector(OutputDim.Y * OutputDim.Z, OutputDim.Z, 1);
	AddLayerCS->SetShaderParameters(RHICmdList, UniformParam);

	// Dispatch shader
	const int32 ThreadGroupCountX = FMath::CeilToInt(OutputDim.X / 32.0f);
	const int32 ThreadGroupCountY = FMath::CeilToInt(OutputDim.Y / 32.0f);
	const int32 ThreadGroupCountZ = 1;
	DispatchComputeShader(RHICmdList, AddLayerCS, ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);

	// Unbind shader textures
	AddLayerCS->UnbindShaderBuffers(RHICmdList);

	// Make UAV safe for read
	RHICmdList.TransitionResource(
		EResourceTransitionAccess::EReadable,
		EResourceTransitionPipeline::EComputeToCompute,
		OutputBufferUAV,
		nullptr);
}

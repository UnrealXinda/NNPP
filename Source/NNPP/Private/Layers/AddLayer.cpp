// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Layers/AddLayer.h"
#include "Layers/NNLayerUtils.h"

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FAddLayerComputeShaderParameters, )
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
	FNNLayerBase()
{

}

FAddLayer::~FAddLayer()
{

}

void FAddLayer::SetupLayer(FIntVector InInputDim)
{
	FNNLayerBase::SetupLayer(InInputDim);
}

void FAddLayer::ReleaseResource()
{

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
	UniformParam.InputDimX           = InputDim.X;
	UniformParam.InputDimY           = InputDim.Y;
	UniformParam.InputDimZ           = InputDim.Z;
	UniformParam.OutputDimX          = OutputDim.X;
	UniformParam.OutputDimY          = OutputDim.Y;
	UniformParam.OutputDimZ          = OutputDim.Z;
	UniformParam.InputDimIndexMultX  = InputDim.Y * InputDim.Z;
	UniformParam.InputDimIndexMultY  = InputDim.Z;
	UniformParam.InputDimIndexMultZ  = 1;
	UniformParam.OutputDimIndexMultX = OutputDim.Y * OutputDim.Z;
	UniformParam.OutputDimIndexMultY = OutputDim.Z;
	UniformParam.OutputDimIndexMultZ = 1;
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

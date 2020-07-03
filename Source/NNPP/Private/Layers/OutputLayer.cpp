// Fill out your copyright notice in the Description page of Project Settings.

#include "Layers/OutputLayer.h"
#include "Layers/NNLayerUtils.h"

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FOutputLayerComputeShaderParameters, )
	SHADER_PARAMETER(int32, InputDimX)
	SHADER_PARAMETER(int32, InputDimY)
	SHADER_PARAMETER(int32, InputDimZ)
	SHADER_PARAMETER(int32, InputDimIndexMultX)
	SHADER_PARAMETER(int32, InputDimIndexMultY)
	SHADER_PARAMETER(int32, InputDimIndexMultZ)
END_GLOBAL_SHADER_PARAMETER_STRUCT()
IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FOutputLayerComputeShaderParameters, "OutputLayerUniform");

class FOutputLayerComputeShader : public FGlobalShader
{
public:

	using FParameters = FOutputLayerComputeShaderParameters;
	DECLARE_NNLAYER_COMPUTE_SHADER(FOutputLayerComputeShader);

	FOutputLayerComputeShader() {}
	FOutputLayerComputeShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		InputBuffer.Bind(Initializer.ParameterMap, TEXT("InputBuffer"));
		OutputTexture.Bind(Initializer.ParameterMap, TEXT("OutputTexture"));
	}

	void BindShaderBuffers(
		FRHICommandList&           RHICmdList,
		FUnorderedAccessViewRHIRef OutputTextureUAV,
		FShaderResourceViewRHIRef  InputBufferSRV)
	{
		FRHIComputeShader* ComputeShaderRHI = RHICmdList.GetBoundComputeShader();

		SetUAVParameter(RHICmdList, ComputeShaderRHI, OutputTexture, OutputTextureUAV);
		SetSRVParameter(RHICmdList, ComputeShaderRHI, InputBuffer, InputBufferSRV);
	}

	void UnbindShaderBuffers(FRHICommandList& RHICmdList)
	{
		FRHIComputeShader* ComputeShaderRHI = RHICmdList.GetBoundComputeShader();

		SetUAVParameter(RHICmdList, ComputeShaderRHI, OutputTexture, FUnorderedAccessViewRHIRef());
		SetSRVParameter(RHICmdList, ComputeShaderRHI, InputBuffer, FShaderResourceViewRHIRef());
	}

private:

	LAYOUT_FIELD(FShaderResourceParameter, InputBuffer);
	LAYOUT_FIELD(FShaderResourceParameter, OutputTexture);
};

IMPLEMENT_GLOBAL_SHADER(FOutputLayerComputeShader, "/Plugin/NNPP/OutputLayer.usf", "OutputLayer", SF_Compute);

FOutputLayer::FOutputLayer() :
	FNNLayerBase()
{

}

FOutputLayer::~FOutputLayer()
{

}

void FOutputLayer::SetupLayer(FIntVector InInputDim)
{
	FNNLayerBase::SetupLayer(InInputDim);
}

void FOutputLayer::ReleaseResource()
{

}

void FOutputLayer::RunLayer_RenderThread(
	FRHICommandList&          RHICmdList,
	FShaderResourceViewRHIRef InputBufferSRV,
	FShaderResourceViewRHIRef OptionalInputBufferSRV /*= nullptr*/)
{
	check(IsInRenderingThread());

	// Bind shader textures
	TShaderMapRef<FOutputLayerComputeShader> OutputLayerCS(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
	RHICmdList.SetComputeShader(OutputLayerCS.GetComputeShader());

	//OutputLayerCS->BindShaderBuffers(RHICmdList, ???, InputBufferSRV);

	// Bind shader uniform
	FOutputLayerComputeShader::FParameters UniformParam;
	UniformParam.InputDimX          = InputDim.X;
	UniformParam.InputDimY          = InputDim.Y;
	UniformParam.InputDimZ          = InputDim.Z;
	UniformParam.InputDimIndexMultX = InputDim.Y * InputDim.Z;
	UniformParam.InputDimIndexMultY = InputDim.Z;
	UniformParam.InputDimIndexMultZ = 1;
	OutputLayerCS->SetShaderParameters(RHICmdList, UniformParam);

	// Dispatch shader
	const int32 ThreadGroupCountX = FMath::CeilToInt(OutputDim.X / 32.0f);
	const int32 ThreadGroupCountY = FMath::CeilToInt(OutputDim.Y / 32.0f);
	const int32 ThreadGroupCountZ = 1;
	DispatchComputeShader(RHICmdList, OutputLayerCS, ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);

	// Unbind shader textures
	OutputLayerCS->UnbindShaderBuffers(RHICmdList);
}

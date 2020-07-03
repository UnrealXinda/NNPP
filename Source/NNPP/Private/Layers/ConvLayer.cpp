// Fill out your copyright notice in the Description page of Project Settings.

#include "Layers/ConvLayer.h"
#include "Layers/NNLayerUtils.h"


#define PPCAT_NX(A, B) A ## B
#define PPCAT(A, B) PPCAT_NX(A, B)
#define STRINGIZE_NX(A) #A
#define STRINGIZE(A) STRINGIZE_NX(A)

#define DECLARE_CONV_LAYER_SHADER_PARAM(FilterSize)                                                      \
BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FConvLayer##FilterSize##ComputeShaderParameters, )                  \
	SHADER_PARAMETER(FIntVector,  InputDim)                                                              \
	SHADER_PARAMETER(FIntVector,  OutputDim)                                                             \
	SHADER_PARAMETER(FIntVector,  InputDimIndexMult)                                                     \
	SHADER_PARAMETER(FIntVector,  OutputDimIndexMult)                                                    \
	SHADER_PARAMETER(FIntVector4, WeightDim)                                                             \
	SHADER_PARAMETER(FIntVector4, WeightDimIndexMult)                                                    \
	SHADER_PARAMETER(FIntPoint,   Stride)                                                                \
END_GLOBAL_SHADER_PARAMETER_STRUCT()                                                                     \
IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FConvLayer##FilterSize##ComputeShaderParameters,                \
STRINGIZE(PPCAT(PPCAT(ConvLayer, FilterSize), Uniform)));

#define DECLARE_CONV_LAYER_SHADER_CLASS(FilterSize)                                                         \
class FConvLayer##FilterSize##ComputeShader : public FGlobalShader									        \
{                                                                                                           \
public:																								        \
																									        \
	using FParameters = FConvLayer##FilterSize##ComputeShaderParameters;							        \
	DECLARE_NNLAYER_COMPUTE_SHADER(FConvLayer##FilterSize##ComputeShader);							        \
																									        \
	FConvLayer##FilterSize##ComputeShader() {}																\
	FConvLayer##FilterSize##ComputeShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer) \
		: FGlobalShader(Initializer)																        \
	{																								        \
		InputBuffer.Bind(Initializer.ParameterMap, TEXT("InputBuffer"));							        \
		WeightBuffer.Bind(Initializer.ParameterMap, TEXT("WeightBuffer"));							        \
		OutputBuffer.Bind(Initializer.ParameterMap, TEXT("OutputBuffer"));							        \
	}																								        \
																									        \
	void BindShaderBuffers(																			        \
		FRHICommandList&           RHICmdList,														        \
		FUnorderedAccessViewRHIRef OutputBufferUAV,													        \
		FShaderResourceViewRHIRef  InputBufferSRV,															\
		FShaderResourceViewRHIRef  WeightBufferSRV)													        \
	{																								        \
		FRHIComputeShader* ComputeShaderRHI = RHICmdList.GetBoundComputeShader();					        \
																									        \
		SetUAVParameter(RHICmdList, ComputeShaderRHI, OutputBuffer, OutputBufferUAV);				        \
		SetSRVParameter(RHICmdList, ComputeShaderRHI, InputBuffer, InputBufferSRV);					        \
		SetSRVParameter(RHICmdList, ComputeShaderRHI, WeightBuffer, WeightBufferSRV);					    \
	}																								        \
																									        \
	void UnbindShaderBuffers(FRHICommandList& RHICmdList)											        \
	{																								        \
		FRHIComputeShader* ComputeShaderRHI = RHICmdList.GetBoundComputeShader();					        \
																									        \
		SetUAVParameter(RHICmdList, ComputeShaderRHI, OutputBuffer, FUnorderedAccessViewRHIRef());	        \
		SetSRVParameter(RHICmdList, ComputeShaderRHI, InputBuffer, FShaderResourceViewRHIRef());	        \
		SetSRVParameter(RHICmdList, ComputeShaderRHI, WeightBuffer, FShaderResourceViewRHIRef());	        \
	}																								        \
																									        \
private:																							        \
																									        \
	LAYOUT_FIELD(FShaderResourceParameter, InputBuffer);											        \
	LAYOUT_FIELD(FShaderResourceParameter, WeightBuffer);											        \
	LAYOUT_FIELD(FShaderResourceParameter, OutputBuffer);											        \
};																										    \
IMPLEMENT_GLOBAL_SHADER(FConvLayer##FilterSize##ComputeShader,												\
STRINGIZE(PPCAT(/Plugin/NNPP/ConvLayer, FilterSize).usf),													\
STRINGIZE(PPCAT(ConvLayer, FilterSize)), SF_Compute);

#define DECLARE_CONV_LAYER_SHADER(FilterSize) \
DECLARE_CONV_LAYER_SHADER_PARAM(FilterSize);  \
DECLARE_CONV_LAYER_SHADER_CLASS(FilterSize);

DECLARE_CONV_LAYER_SHADER(8);
DECLARE_CONV_LAYER_SHADER(12);
DECLARE_CONV_LAYER_SHADER(16);
DECLARE_CONV_LAYER_SHADER(20);
DECLARE_CONV_LAYER_SHADER(32);
DECLARE_CONV_LAYER_SHADER(64);
DECLARE_CONV_LAYER_SHADER(128);
DECLARE_CONV_LAYER_SHADER(256);

FConvLayer::FConvLayer() :
	FNNLayerBase()
{

}

FConvLayer::~FConvLayer()
{

}

void FConvLayer::SetupLayer(FIntVector InInputDim)
{
	FNNLayerBase::SetupLayer(InInputDim);

	OutputDim = FIntVector(InputDim.X / Stride.X, InputDim.Y / Stride.Y, Filters);

	ReleaseResource();
}

void FConvLayer::ReleaseResource()
{

}

void FConvLayer::RunLayer_RenderThread(
	FRHICommandList&          RHICmdList,
	FShaderResourceViewRHIRef InputBufferSRV,
	FShaderResourceViewRHIRef OptionalInputBufferSRV /*= nullptr*/)
{
	DispatchConvShader_RenderThread<FConvLayer128ComputeShader>(RHICmdList, InputBufferSRV);

	// Make UAV safe for read
	RHICmdList.TransitionResource(
		EResourceTransitionAccess::EReadable,
		EResourceTransitionPipeline::EComputeToCompute,
		OutputBufferUAV,
		nullptr);
}

template <class ShaderClass>
void FConvLayer::DispatchConvShader_RenderThread(FRHICommandList& RHICmdList, FShaderResourceViewRHIRef InputBufferSRV)
{
	TShaderMapRef<ShaderClass> ConvLayerCS(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
	RHICmdList.SetComputeShader(ConvLayerCS.GetComputeShader());

	ConvLayerCS->BindShaderBuffers(RHICmdList, OutputBufferUAV, InputBufferSRV, WeightBufferSRV);

	ShaderClass::FParameters UniformParam;
	UniformParam.InputDim           = InputDim;
	UniformParam.OutputDim          = OutputDim;
	UniformParam.InputDimIndexMult  = FIntVector(InputDim.Y * InputDim.Z, InputDim.Z, 1);
	UniformParam.OutputDimIndexMult = FIntVector(OutputDim.Y * OutputDim.Z, OutputDim.Z, 1);
	UniformParam.WeightDim          = FIntVector4(KernelSize.X, KernelSize.Y, InputDim.Z, Filters);
	UniformParam.WeightDimIndexMult = FIntVector4(KernelSize.Y * InputDim.Z * Filters, InputDim.Z * Filters, Filters, 1);
	UniformParam.Stride             = Stride;
	ConvLayerCS->SetShaderParameters(RHICmdList, UniformParam);

	const int32 ThreadGroupCountX = FMath::CeilToInt(OutputDim.Y * OutputDim.Z / 32.0f);
	const int32 ThreadGroupCountY = OutputDim.X;
	const int32 ThreadGroupCountZ = 1;
	DispatchComputeShader(RHICmdList, ConvLayerCS, ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);

	ConvLayerCS->UnbindShaderBuffers(RHICmdList);
}

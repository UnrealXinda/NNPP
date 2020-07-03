// Fill out your copyright notice in the Description page of Project Settings.

#include "Layers/ConvLayer.h"
#include "Layers/NNLayerUtils.h"


#define PPCAT_NX(A, B) A ## B
#define PPCAT(A, B) PPCAT_NX(A, B)
#define STRINGIZE_NX(A) #A
#define STRINGIZE(A) STRINGIZE_NX(A)

#define DECLARE_CONV_LAYER_SHADER_PARAM(FilterSize)                                                      \
BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FConvLayer##FilterSize##ComputeShaderParameters, )                  \
	SHADER_PARAMETER(int32, InputDimX)                                                                   \
	SHADER_PARAMETER(int32, InputDimY)                                                                   \
	SHADER_PARAMETER(int32, InputDimZ)                                                                   \
	SHADER_PARAMETER(int32, OutputDimX)                                                                  \
	SHADER_PARAMETER(int32, OutputDimY)                                                                  \
	SHADER_PARAMETER(int32, OutputDimZ)                                                                  \
	SHADER_PARAMETER(int32, InputDimIndexMultX)                                                          \
	SHADER_PARAMETER(int32, InputDimIndexMultY)                                                          \
	SHADER_PARAMETER(int32, InputDimIndexMultZ)                                                          \
	SHADER_PARAMETER(int32, OutputDimIndexMultX)                                                         \
	SHADER_PARAMETER(int32, OutputDimIndexMultY)                                                         \
	SHADER_PARAMETER(int32, OutputDimIndexMultZ)                                                         \
	SHADER_PARAMETER(int32, WeightDimX)                                                                  \
	SHADER_PARAMETER(int32, WeightDimY)                                                                  \
	SHADER_PARAMETER(int32, WeightDimZ)                                                                  \
	SHADER_PARAMETER(int32, WeightDimW)                                                                  \
	SHADER_PARAMETER(int32, WeightDimIndexMultX)                                                         \
	SHADER_PARAMETER(int32, WeightDimIndexMultY)                                                         \
	SHADER_PARAMETER(int32, WeightDimIndexMultZ)                                                         \
	SHADER_PARAMETER(int32, WeightDimIndexMultW)                                                         \
	SHADER_PARAMETER(int32, StrideX)                                                                     \
	SHADER_PARAMETER(int32, StrideY)                                                                     \
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

#define RUN_CONV_LAYER_IMPLEMENTATION(FilterSize)															  \
																											  \
TShaderMapRef<FConvLayer##FilterSize##ComputeShader> ConvLayerCS(GetGlobalShaderMap(ERHIFeatureLevel::SM5));  \
RHICmdList.SetComputeShader(ConvLayerCS.GetComputeShader());												  \
																											  \
ConvLayerCS->BindShaderBuffers(RHICmdList, OutputBufferUAV, InputBufferSRV, WeightBufferSRV);	      		  \
																											  \
FConvLayer##FilterSize##ComputeShader::FParameters UniformParam;											  \
UniformParam.InputDimX = ConvLayer.InputDim.X;																  \
UniformParam.InputDimY = ConvLayer.InputDim.Y;																  \
UniformParam.InputDimZ = ConvLayer.InputDim.Z;																  \
UniformParam.OutputDimX = ConvLayer.OutputDim.X;														      \
UniformParam.OutputDimY = ConvLayer.OutputDim.Y;														      \
UniformParam.OutputDimZ = ConvLayer.OutputDim.Z;														      \
UniformParam.InputDimIndexMultX = ConvLayer.InputDim.Y * ConvLayer.InputDim.Z;								  \
UniformParam.InputDimIndexMultY = ConvLayer.InputDim.Z;														  \
UniformParam.InputDimIndexMultZ = 1;																		  \
UniformParam.OutputDimIndexMultX = ConvLayer.OutputDim.Y * ConvLayer.OutputDim.Z;				 			  \
UniformParam.OutputDimIndexMultY = ConvLayer.OutputDim.Z;													  \
UniformParam.OutputDimIndexMultZ = 1;																		  \
UniformParam.WeightDimX = ConvLayer.KernelSize.X;															  \
UniformParam.WeightDimY = ConvLayer.KernelSize.Y;															  \
UniformParam.WeightDimZ = ConvLayer.InputDim.Z;																  \
UniformParam.WeightDimW = ConvLayer.Filters;																  \
UniformParam.WeightDimIndexMultX = ConvLayer.KernelSize.Y * ConvLayer.InputDim.Z * ConvLayer.Filters;		  \
UniformParam.WeightDimIndexMultY = ConvLayer.InputDim.Z * ConvLayer.Filters;							 	  \
UniformParam.WeightDimIndexMultZ = ConvLayer.Filters;														  \
UniformParam.WeightDimIndexMultW = 1;																		  \
UniformParam.StrideX = ConvLayer.Stride.X;																	  \
UniformParam.StrideY = ConvLayer.Stride.Y;																	  \
ConvLayerCS->SetShaderParameters(RHICmdList, UniformParam);													  \
																											  \
const int32 ThreadGroupCountX = FMath::CeilToInt(ConvLayer.OutputDim.Y * ConvLayer.OutputDim.Z / 32.0f);	  \
const int32 ThreadGroupCountY = ConvLayer.OutputDim.X;														  \
const int32 ThreadGroupCountZ = 1;																			  \
DispatchComputeShader(RHICmdList, ConvLayerCS, ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);	  \
																											  \
ConvLayerCS->UnbindShaderBuffers(RHICmdList);

#define DECLARE_CONV_LAYER_HELPER_FUNCTOR(FilterSize)   \
template <>											    \
struct FConvLayerFunctor<FilterSize>				    \
{													    \
public:												    \
	static void Run(								    \
		const FConvLayer&          ConvLayer,		    \
		FRHICommandList&           RHICmdList,		    \
		FUnorderedAccessViewRHIRef OutputBufferUAV,	    \
		FShaderResourceViewRHIRef  InputBufferSRV,	    \
		FShaderResourceViewRHIRef  WeightBufferSRV)     \
	{												    \
		RUN_CONV_LAYER_IMPLEMENTATION(FilterSize);	    \
	}												    \
};

#define DECLARE_CONV_LAYER_SHADER(FilterSize) \
DECLARE_CONV_LAYER_SHADER_PARAM(FilterSize);  \
DECLARE_CONV_LAYER_SHADER_CLASS(FilterSize);  \
DECLARE_CONV_LAYER_HELPER_FUNCTOR(FilterSize);

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
}

void FConvLayer::ReleaseResource()
{

}

void FConvLayer::RunLayer_RenderThread(
	FRHICommandList&          RHICmdList, 
	FShaderResourceViewRHIRef InputBufferSRV,
	FShaderResourceViewRHIRef OptionalInputBufferSRV /*= nullptr*/)
{
	FConvLayerFunctor<256>::Run(*this, RHICmdList, OutputBufferUAV, InputBufferSRV, WeightBufferSRV);

	// Make UAV safe for read
	RHICmdList.TransitionResource(
		EResourceTransitionAccess::EReadable,
		EResourceTransitionPipeline::EComputeToCompute,
		OutputBufferUAV,
		nullptr);
}
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

namespace
{
	inline int GetSupportedConvChannel(int32 Channels)
	{
		auto Predicate = [Channels](const int32& Channel) -> bool
		{
			return Channel >= Channels;
		};

		if (const int32* SupportedChannel = FConvLayer::kChannels.FindByPredicate(Predicate))
		{
			return *SupportedChannel;
		}

		return -1;
	}
}

FConvLayer::FConvLayer() :
	FNNLayerBase(ENNLayerType::Conv)
{

}

FConvLayer::~FConvLayer()
{
	ReleaseWeightBuffers();
}

void FConvLayer::SetupLayer(FIntVector InInputDim)
{
	FNNLayerBase::SetupLayer(InInputDim);

	OutputDim = FIntVector(InputDim.X / Stride.X, InputDim.Y / Stride.Y, Filters);

	ConvChannels = GetSupportedConvChannel(FMath::Max(InputDim.Z, Filters));
}

void FConvLayer::RunLayer_RenderThread(
	FRHICommandList&           RHICmdList,
	FUnorderedAccessViewRHIRef OutputBufferUAV,
	FShaderResourceViewRHIRef  InputBufferSRV,
	FShaderResourceViewRHIRef  OptionalInputBufferSRV /*= nullptr*/)
{
	switch (ConvChannels)
	{
	case 8:
		DispatchConvShader_RenderThread<FConvLayer8ComputeShader>(RHICmdList, OutputBufferUAV, InputBufferSRV);
		break;
	case 12:
		DispatchConvShader_RenderThread<FConvLayer12ComputeShader>(RHICmdList, OutputBufferUAV, InputBufferSRV);
		break;
	case 16:
		DispatchConvShader_RenderThread<FConvLayer16ComputeShader>(RHICmdList, OutputBufferUAV, InputBufferSRV);
		break;
	case 20:
		DispatchConvShader_RenderThread<FConvLayer20ComputeShader>(RHICmdList, OutputBufferUAV, InputBufferSRV);
		break;
	case 32:
		DispatchConvShader_RenderThread<FConvLayer32ComputeShader>(RHICmdList, OutputBufferUAV, InputBufferSRV);
		break;
	case 64:
		DispatchConvShader_RenderThread<FConvLayer64ComputeShader>(RHICmdList, OutputBufferUAV, InputBufferSRV);
		break;
	case 128:
		DispatchConvShader_RenderThread<FConvLayer128ComputeShader>(RHICmdList, OutputBufferUAV, InputBufferSRV);
		break;
	case 256:
		DispatchConvShader_RenderThread<FConvLayer256ComputeShader>(RHICmdList, OutputBufferUAV, InputBufferSRV);
		break;
	}	

	// Make UAV safe for read
	RHICmdList.TransitionResource(
		EResourceTransitionAccess::EReadable,
		EResourceTransitionPipeline::EComputeToCompute,
		OutputBufferUAV,
		nullptr);
}

template <class ShaderClass>
void FConvLayer::DispatchConvShader_RenderThread(
	FRHICommandList&           RHICmdList,
	FUnorderedAccessViewRHIRef OutputBufferUAV,
	FShaderResourceViewRHIRef  InputBufferSRV)
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

	const int32 ThreadGroupCountX = 1;
	const int32 ThreadGroupCountY = OutputDim.Y;
	const int32 ThreadGroupCountZ = FMath::CeilToInt(OutputDim.X / 4.0f);;
	DispatchComputeShader(RHICmdList, ConvLayerCS, ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);

	ConvLayerCS->UnbindShaderBuffers(RHICmdList);
}

void FConvLayer::SetupWeightBuffer(const float* WeightData, int32 Size)
{
	// Always release before reallocating new buffers
	ReleaseWeightBuffers();

	TResourceArray<float> ResourceArray;
	ResourceArray.AddUninitialized(Size);
	FMemory::Memcpy(ResourceArray.GetData(), WeightData, sizeof(float) * Size);

	FRHIResourceCreateInfo CreateInfo(&ResourceArray);

	WeightBuffer = RHICreateStructuredBuffer(
		sizeof(float),          // Stride
		sizeof(float) * Size,   // Size
		BUF_ShaderResource,     // Usage
		CreateInfo              // Create info
	);
	WeightBufferSRV = RHICreateShaderResourceView(WeightBuffer);
}

void FConvLayer::ReleaseWeightBuffers()
{
	ReleaseRenderResource<FStructuredBufferRHIRef>(WeightBuffer);
	ReleaseRenderResource<FShaderResourceViewRHIRef>(WeightBufferSRV);
}

// Copyright Epic Games, Inc. All Rights Reserved.

#include "NNModel.h"
#include <iostream>
#include <fstream>
#include "Misc/FileHelper.h"
#include "Layers/AddLayer.h"
#include "Layers/BatchNormLayer.h"
#include "Layers/ConvLayer.h"
#include "Layers/InputLayer.h"
#include "Layers/OutputLayer.h"
#include "Layers/ReLULayer.h"
#include "Layers/TanhLayer.h"
#include "Layers/UpSamplingLayer.h"

THIRD_PARTY_INCLUDES_START
#pragma push_macro("CONSTEXPR")
#undef CONSTEXPR
#pragma push_macro("dynamic_cast")
#undef dynamic_cast
#pragma push_macro("check")
#undef check
#pragma push_macro("PI")
#undef PI

#include <json.hpp>

#pragma pop_macro("PI")
#pragma pop_macro("check")
#pragma pop_macro("dynamic_cast")
#pragma pop_macro("CONSTEXPR")
THIRD_PARTY_INCLUDES_END

using json = nlohmann::json;

NLOHMANN_JSON_SERIALIZE_ENUM(ENNLayerType, {
	{ENNLayerType::Input,        "NNPP.InputLayer"},
	{ENNLayerType::Output,       "NNPP.OutputLayer"},
	{ENNLayerType::Conv,         "NNPP.Conv2D"},
	{ENNLayerType::BatchNorm,    "NNPP.BatchNormalization"},
	{ENNLayerType::ReLU,         "NNPP.ReLU"},
	{ENNLayerType::Add,          "NNPP.Add"},
	{ENNLayerType::UpSampling,   "NNPP.UpSampling2D"},
	{ENNLayerType::Tanh,         "NNPP.Tanh"},
});

namespace
{
	inline FVector GetVector(const json& j)
	{
		float X = j["x"].get<float>();
		float Y = j["y"].get<float>();
		float Z = j["z"].get<float>();
		return FVector(X, Y, Z);
	}

	inline FIntVector GetIntVector(const json& j)
	{
		int X = j["x"].get<int>();
		int Y = j["y"].get<int>();
		int Z = j["z"].get<int>();
		return FIntVector(X, Y, Z);
	}

	inline FIntPoint GetIntPoint(const json& j)
	{
		int X = j["x"].get<int>();
		int Y = j["y"].get<int>();
		return FIntPoint(X, Y);
	}

	inline FVector4 GetVector4(const json& j)
	{
		float X = j["x"].get<float>();
		float Y = j["y"].get<float>();
		float Z = j["z"].get<float>();
		float W = j["w"].get<float>();
		return FVector4(X, Y, Z, W);
	}

	TUniquePtr<FNNLayerBase> ParseInputLayer(const json& layer)
	{
		auto NNLayer = MakeUnique<FInputLayer>();

		const auto& name          = layer["Name"]; // FName
		const auto& inputShape    = layer["InputShape"]; // FIntVector
		const auto& outputShape   = layer["OutputShape"]; // FIntVector
		const auto& weightShape   = layer["WeightShape"]; // FVector4
		const auto& inputChannels = layer["InputChannels"]; // int

		NNLayer->SetName(FName(*FString(name.get<std::string>().c_str())));
		NNLayer->InputChannels = inputChannels.get<int>();

		return NNLayer;
	}

	TUniquePtr<FNNLayerBase> ParseConvLayer(const json& layer)
	{
		auto NNLayer = MakeUnique<FConvLayer>();

		const auto& name        = layer["Name"]; // FName
		const auto& inputShape  = layer["InputShape"]; // FIntVector
		const auto& outputShape = layer["OutputShape"]; // FIntVector
		const auto& weightShape = layer["WeightShape"]; // FVector4
		const auto& kernelSize  = layer["KernelSize"]; // FIntPoint
		const auto& filters     = layer["Filters"]; // int
		const auto& stride      = layer["Stride"]; // FIntPoint
		const auto& weights     = layer["Weights"]; // float array

		auto weightArray = weights.get<std::vector<float>>();

		NNLayer->SetName(FName(*FString(name.get<std::string>().c_str())));
		NNLayer->SetupWeightBuffer(weightArray.data(), weightArray.size());
		NNLayer->KernelSize = GetIntPoint(kernelSize);
		NNLayer->Filters    = filters.get<int>();
		NNLayer->Stride     = GetIntPoint(stride);

		return NNLayer;
	}

	TUniquePtr<FNNLayerBase> ParseBatchNormLayer(const json& layer)
	{
		auto NNLayer = MakeUnique<FBatchNormLayer>();

		const auto& name        = layer["Name"]; // FName
		const auto& inputShape  = layer["InputShape"]; // FIntVector
		const auto& outputShape = layer["OutputShape"]; // FIntVector
		const auto& weightShape = layer["WeightShape"]; // FVector4
		const auto& weights     = layer["Weights"]; // float array

		auto weightArray = weights.get<std::vector<float>>();

		NNLayer->SetName(FName(*FString(name.get<std::string>().c_str())));
		NNLayer->SetupWeightBuffer(weightArray.data(), weightArray.size());

		return NNLayer;		
	}

	TUniquePtr<FNNLayerBase> ParseReLULayer(const json& layer)
	{
		auto NNLayer = MakeUnique<FReLULayer>();

		const auto& name        = layer["Name"]; // FName
		const auto& inputShape  = layer["InputShape"]; // FIntVector
		const auto& outputShape = layer["OutputShape"]; // FIntVector
		const auto& weightShape = layer["WeightShape"]; // FVector4

		NNLayer->SetName(FName(*FString(name.get<std::string>().c_str())));

		return NNLayer;
	}

	TUniquePtr<FNNLayerBase> ParseAddLayer(const json& layer)
	{
		auto NNLayer = MakeUnique<FAddLayer>();

		const auto& name = layer["Name"]; // FName
		const auto& inputShape = layer["InputShape"]; // FIntVector
		const auto& outputShape = layer["OutputShape"]; // FIntVector
		const auto& weightShape = layer["WeightShape"]; // FVector4
		const auto& alternativeInputId = layer["AlternativeInputId"]; // int

		NNLayer->SetName(FName(*FString(name.get<std::string>().c_str())));
		NNLayer->OtherInputLayerIndex = alternativeInputId.get<int>();

		return NNLayer;
	}

	TUniquePtr<FNNLayerBase> ParseUpSamplingLayer(const json& layer)
	{
		auto NNLayer = MakeUnique<FUpSamplingLayer>();

		const auto& name = layer["Name"]; // FName
		const auto& inputShape = layer["InputShape"]; // FIntVector
		const auto& outputShape = layer["OutputShape"]; // FIntVector
		const auto& weightShape = layer["WeightShape"]; // FVector4
		const auto& size = layer["Size"]; // FIntPoint

		NNLayer->SetName(FName(*FString(name.get<std::string>().c_str())));
		NNLayer->Size = GetIntPoint(size);

		return NNLayer;
	}

	TUniquePtr<FNNLayerBase> ParseTanhLayer(const json& layer, bool print = false)
	{
		auto NNLayer = MakeUnique<FTanhLayer>();

		const auto& name = layer["Name"]; // FName
		const auto& inputShape = layer["InputShape"]; // FIntVector
		const auto& outputShape = layer["OutputShape"]; // FIntVector
		const auto& weightShape = layer["WeightShape"]; // FVect

		NNLayer->SetName(FName(*FString(name.get<std::string>().c_str())));

		return NNLayer;
	}

	FORCEINLINE FNNBufferSize SizeFromVector(FIntVector Dim)
	{
		return Dim.X * Dim.Y * Dim.Z;
	}
}

FNNBuffer::FNNBuffer(FNNBufferSize InSize) :
	Size(InSize)
{
	FRHIResourceCreateInfo CreateInfo;

	Buffer = RHICreateStructuredBuffer(
		sizeof(float),                            // Stride
		sizeof(float) * Size,                     // Size
		BUF_UnorderedAccess | BUF_ShaderResource, // Usage
		CreateInfo                                // Create info
	);
	BufferSRV = RHICreateShaderResourceView(Buffer);
	BufferUAV = RHICreateUnorderedAccessView(Buffer, true, false);
}

FNNBuffer::~FNNBuffer()
{
	ReleaseRenderResource<FStructuredBufferRHIRef>(Buffer);
	ReleaseRenderResource<FShaderResourceViewRHIRef>(BufferSRV);
	ReleaseRenderResource<FUnorderedAccessViewRHIRef>(BufferUAV);
}

FNNModel::FNNModel()
{

}

FNNModel::~FNNModel()
{
	ResetModel();
}

void FNNModel::Predict(FRHITexture* TargetTexture, FShaderResourceViewRHIRef SrcSRV, FIntPoint ImageDim)
{
	// Swap width and height
	SetupLayers(ImageDim);

	ENQUEUE_RENDER_COMMAND(KaleidoComputeCommand)
	(
		[TargetTexture, SrcSRV, this](FRHICommandListImmediate& RHICmdList)
		{
			Predict_RenderThread(RHICmdList, TargetTexture, SrcSRV);
		}
	);
}

void FNNModel::LoadModel(const TCHAR* ModelFile)
{
	FString FileContent;

	if (FFileHelper::LoadFileToString(FileContent, ModelFile))
	{
		try
		{
			Layers.Empty();

			auto fileLength = FileContent.Len();
			auto jsonContent = json::parse(*FileContent, *FileContent + fileLength);
			auto layerTypes = jsonContent["LayerTypes"];
			auto layerJsonStrings = jsonContent["LayerJson"];

			auto layerTypeEnums = std::vector<ENNLayerType>{};

			for (const auto& layer : layerTypes)
			{
				layerTypeEnums.push_back(layer.get<ENNLayerType>());
			}

			auto enumIt = layerTypeEnums.begin();

			for (const auto& layer : layerJsonStrings.items())
			{
				auto layerJson = json::parse(layer.value().get<std::string>());

				switch (*enumIt)
				{
				case ENNLayerType::Input:
					Layers.Add(ParseInputLayer(layerJson));
					break;
				case ENNLayerType::Conv:
					Layers.Add(ParseConvLayer(layerJson));
					break;
				case ENNLayerType::BatchNorm:
					Layers.Add(ParseBatchNormLayer(layerJson));
					break;
				case ENNLayerType::ReLU:
					Layers.Add(ParseReLULayer(layerJson));
					break;
				case ENNLayerType::Add:
					Layers.Add(ParseAddLayer(layerJson));
					break;
				case ENNLayerType::UpSampling:
					Layers.Add(ParseUpSamplingLayer(layerJson));
					break;
				case ENNLayerType::Tanh:
					Layers.Add(ParseTanhLayer(layerJson, true));
					break;
				}

				++enumIt;
			}

			// Append output layer to the end
			Layers.Add(MakeUnique<FOutputLayer>());
		}
		catch (std::exception& e)
		{
			std::cerr << e.what() << std::endl;
		}
	}
}

void FNNModel::SetupLayers(FIntPoint ImageDim)
{
	if (CachedImageDim != ImageDim)
	{
		FIntVector InputDim;

		// Release allocated buffers and textures before reallocating
		ResetModel();

		FInputLayer* InputLayer = StaticCast<FInputLayer*>(Layers[0].Get());

		// Swap width and height
		InputDim = FIntVector(ImageDim.Y, ImageDim.X, InputLayer->InputChannels);
		InputLayer->SetupLayer(InputDim);

		for (int32 Idx = 1; Idx < Layers.Num(); ++Idx)
		{
			FNNLayerBase* Layer = Layers[Idx].Get();
			FNNLayerBase* PrevLayer = Layers[Idx - 1].Get();
			InputDim = PrevLayer->GetOutputDim();

			Layer->SetupLayer(InputDim);

			if (Layer->GetLayerType() == ENNLayerType::Add)
			{
				FAddLayer* AddLayer = StaticCast<FAddLayer*>(Layer);
				LayersToCacheOutput.Add(AddLayer->OtherInputLayerIndex);
			}
		}

		CreateOutputTexture(FIntVector(ImageDim.X, ImageDim.Y, InputLayer->InputChannels));

		CachedImageDim = ImageDim;
	}
}

void FNNModel::Predict_RenderThread(
	FRHICommandListImmediate& RHICmdList,
	FRHITexture*              TargetTexture,
	FShaderResourceViewRHIRef SrcSRV)
{
	check(IsInRenderingThread());

	auto RunLayerImpl = [this](
		FRHICommandList& RHICmdList,
		FNNLayerIndex    LayerIdx,
		FNNBufferWeakPtr InputBufferPtr,
		FNNBufferWeakPtr OptionalInputBufferPtr,
		bool             bShouldCache) -> FNNBufferWeakPtr
	{
		FNNLayerBase* Layer = Layers[LayerIdx].Get();
		FIntVector OutputDim = Layer->GetOutputDim();
		FNNBufferWeakPtr OutputBufferPtr = DequeueAvailableBuffer(SizeFromVector(OutputDim));

		FNNBufferSharedPtr InputBuffer = InputBufferPtr.Pin();
		FNNBufferSharedPtr OutputBuffer = OutputBufferPtr.Pin();
		FNNBufferSharedPtr OptionalInputBuffer = OptionalInputBufferPtr.Pin();

		check(InputBuffer);
		check(OutputBuffer);

		FUnorderedAccessViewRHIRef OutputUAV = OutputBuffer->GetUAV();
		FShaderResourceViewRHIRef InputSRV = InputBuffer->GetSRV();
		FShaderResourceViewRHIRef OptionalInputSRV = OptionalInputBuffer ? OptionalInputBuffer->GetSRV() : nullptr;

		Layer->RunLayer_RenderThread(RHICmdList, OutputUAV, InputSRV, OptionalInputSRV);

		if (bShouldCache)
		{
			check(!CachedLookUpTable.Contains(LayerIdx));
			CachedLookUpTable.Add(LayerIdx, OutputBuffer);
		}

		// Finished using the input buffer, return to available buffer queue
		EnqueueAvailableBuffer(InputBuffer);

		// Finished using cached output. Remove from the cached look up table
		if (Layer->GetLayerType() == ENNLayerType::Add)
		{
			FAddLayer* AddLayer = StaticCast<FAddLayer*>(Layer);
			int32 OtherInputLayerIdx = AddLayer->OtherInputLayerIndex;

			check(CachedLookUpTable.Contains(OtherInputLayerIdx));
			CachedLookUpTable.Remove(OtherInputLayerIdx);
		}

		return OutputBuffer;
	};

	FNNBufferWeakPtr InputBuffer;
	FNNBufferWeakPtr OutputBuffer;
	FIntVector OutputDim;

	FNNLayerBase* InputLayer = Layers[0].Get();
	OutputDim = InputLayer->GetOutputDim();

	OutputBuffer = DequeueAvailableBuffer(SizeFromVector(OutputDim));
	FNNBufferSharedPtr OutputBufferPtr = OutputBuffer.Pin();
	check(OutputBufferPtr);
	InputLayer->RunLayer_RenderThread(RHICmdList, OutputBufferPtr->GetUAV(), SrcSRV);

	// Previous output buffer becomes new input buffer
	InputBuffer = OutputBuffer;

	for (int32 Idx = 1; Idx < Layers.Num() - 1; ++Idx)
	{
		FNNLayerBase* Layer = Layers[Idx].Get();
		FNNBufferWeakPtr OptionalInputBuffer = nullptr;
		bool bShouldCacheOutput = LayersToCacheOutput.Contains(Idx);

		if (Layer->GetLayerType() == ENNLayerType::Add)
		{
			FAddLayer* AddLayer = StaticCast<FAddLayer*>(Layer);
			int32 OtherInputLayerIdx = AddLayer->OtherInputLayerIndex;

			check(LayersToCacheOutput.Contains(OtherInputLayerIdx));

			OptionalInputBuffer = CachedLookUpTable[OtherInputLayerIdx];
		}

		OutputBuffer = RunLayerImpl(RHICmdList, Idx, InputBuffer, OptionalInputBuffer, bShouldCacheOutput);

		// Previous output buffer becomes new input buffer
		InputBuffer = OutputBuffer;
	}

	FNNLayerBase* LastLayer = Layers[Layers.Num() - 1].Get();
	FOutputLayer* OutputLayer = StaticCast<FOutputLayer*>(LastLayer);
	OutputBufferPtr = OutputBuffer.Pin();
	check(OutputBufferPtr);
	OutputLayer->RunLayer_RenderThread(RHICmdList, OutputTextureUAV, OutputBufferPtr->GetSRV());
	EnqueueAvailableBuffer(InputBuffer);

	RHICmdList.CopyToResolveTarget(OutputTexture, TargetTexture, FResolveParams());
}

void FNNModel::ResetModel()
{
	ReleaseNNBuffers();
	ReleaseOutputTexture();
}

FNNBufferWeakPtr FNNModel::CreateNNBuffer(FNNBufferSize Size)
{
	if (!NNBufferLookUpTable.Contains(Size))
	{
		NNBufferLookUpTable.Add(Size, FNNBufferQueue());
	}

	TSharedRef<FNNBuffer> Buffer = MakeShared<FNNBuffer>(Size);

	FNNBufferQueue& Queue = NNBufferLookUpTable[Size];
	NNBuffers.Add(Buffer);

	return NNBuffers[NNBuffers.Num() - 1];
}

void FNNModel::ReleaseNNBuffers()
{
	NNBuffers.Empty();
	NNBufferLookUpTable.Empty();
	LayersToCacheOutput.Empty();
	CachedLookUpTable.Empty();
}

void FNNModel::CreateOutputTexture(FIntVector ImageDim)
{
	FRHIResourceCreateInfo CreateInfo;

	OutputTexture = RHICreateTexture2D(ImageDim.X, ImageDim.Y, PF_FloatRGBA, 1, 1, TexCreate_UAV, CreateInfo);
	OutputTextureUAV = RHICreateUnorderedAccessView(OutputTexture);
}

void FNNModel::ReleaseOutputTexture()
{
	ReleaseRenderResource<FTexture2DRHIRef>(OutputTexture);
	ReleaseRenderResource<FUnorderedAccessViewRHIRef>(OutputTextureUAV);
}

void FNNModel::EnqueueAvailableBuffer(FNNBufferWeakPtr Buffer)
{
	FNNBufferSharedPtr BufferPtr = Buffer.Pin();

	check(BufferPtr);
	check(NNBufferLookUpTable.Contains(BufferPtr->GetSize()));

	FNNBufferQueue& Queue = NNBufferLookUpTable[BufferPtr->GetSize()];
	Queue.Add(BufferPtr);
}

FNNBufferWeakPtr FNNModel::DequeueAvailableBuffer(FNNBufferSize Size)
{
	FNNBufferWeakPtr Buffer = nullptr;

	if (NNBufferLookUpTable.Contains(Size))
	{
		FNNBufferQueue& Queue = NNBufferLookUpTable[Size];

		// Dequeue the first buffer that doesn't have cached result
		for (int32 Idx = 0; Idx < Queue.Num(); ++Idx)
		{
			bool IsCachedOutput = false;

			for (auto It = CachedLookUpTable.CreateConstIterator(); It; ++It)
			{
				FNNBufferSharedPtr SharedPtr = (It->Value).Pin();
				check(SharedPtr);

				IsCachedOutput |= SharedPtr == Queue[Idx];
			}

			if (!IsCachedOutput)
			{
				Buffer = Queue[Idx];
				Queue.RemoveAtSwap(Idx);
				break;
			}
		}
	}

	// Create a buffer if not existed
	if (!Buffer.Pin())
	{
		Buffer = CreateNNBuffer(Size);
	}

	return Buffer;
}
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
}

FNNModel::FNNModel()
{

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
		FInputLayer* InputLayer = StaticCast<FInputLayer*>(Layers[0].Get());

		// Swap width and height
		InputLayer->SetupLayer(FIntVector(ImageDim.Y, ImageDim.X, InputLayer->InputChannels));

		for (int32 Idx = 1; Idx < Layers.Num(); ++Idx)
		{
			FNNLayerBase* Layer = Layers[Idx].Get();
			FNNLayerBase* PrevLayer = Layers[Idx - 1].Get();

			Layer->SetupLayer(PrevLayer->GetOutputDim());
		}

		FNNLayerBase* LastLayer = Layers[Layers.Num() - 1].Get();
		FOutputLayer* OutputLayer = StaticCast<FOutputLayer*>(LastLayer);
		OutputLayer->SetupOutputDimension(FIntVector(ImageDim.X, ImageDim.Y, InputLayer->InputChannels));

		CachedImageDim = ImageDim;

		for (const auto& layer : Layers)
		{
			UE_LOG(LogTemp, Warning, TEXT("%s: %s"), *(layer->GetName().ToString()), *(layer->GetOutputDim().ToString()));
		}
	}
}

void FNNModel::Predict_RenderThread(
	FRHICommandListImmediate& RHICmdList,
	FRHITexture*              TargetTexture,
	FShaderResourceViewRHIRef SrcSRV)
{
	check(IsInRenderingThread());

	FNNLayerBase* InputLayer = Layers[0].Get();
	InputLayer->RunLayer_RenderThread(RHICmdList, SrcSRV);

	for (int32 Idx = 1; Idx < Layers.Num(); ++Idx)
	{
		FNNLayerBase* Layer = Layers[Idx].Get();
		FNNLayerBase* PrevLayer = Layers[Idx - 1].Get();

		FShaderResourceViewRHIRef InputSRV = PrevLayer->GetOutputBufferSRV();
		FShaderResourceViewRHIRef OptionalInputSRV = nullptr;

		if (Layer->GetLayerType() == ENNLayerType::Add)
		{
			FAddLayer* AddLayer = StaticCast<FAddLayer*>(Layer);
			int32 OtherInputLayerIdx = AddLayer->OtherInputLayerIndex;
			FNNLayerBase* OtherInputLayer = Layers[OtherInputLayerIdx].Get();

			OptionalInputSRV = OtherInputLayer->GetOutputBufferSRV();
		}

		Layer->RunLayer_RenderThread(RHICmdList, InputSRV, OptionalInputSRV);
	}

	FNNLayerBase* LastLayer = Layers[Layers.Num() - 1].Get();
	FOutputLayer* OutputLayer = StaticCast<FOutputLayer*>(LastLayer);
	OutputLayer->CopyToTargetTexture_RenderThread(RHICmdList, TargetTexture);
}

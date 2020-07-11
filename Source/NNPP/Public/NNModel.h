// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NNLayerBase.h"

using FNNBufferSize = int32;
using FNNLayerIndex = int32;
using FNNBufferQueue = TArray<const struct FNNBuffer*>;
using FNNBufferLookUpTable = TMap<FNNBufferSize, FNNBufferQueue>;
using FNNCachedLookUpTable = TMap<FNNLayerIndex, const struct FNNBuffer*>;

struct FNNBuffer
{
	FNNBufferSize              Size;
	FStructuredBufferRHIRef    Buffer;
	FShaderResourceViewRHIRef  BufferSRV;
	FUnorderedAccessViewRHIRef BufferUAV;
};

class FNNModel
{
public:

	FNNModel();

	void Predict(FRHITexture* TargetTexture, FShaderResourceViewRHIRef SrcSRV, FIntPoint ImageDim);
	void LoadModel(const TCHAR* ModelFile);

protected:

	TArray<TUniquePtr<FNNLayerBase>> Layers;

	FIntPoint CachedImageDim;

	TArray<FNNBuffer>    NNBuffers;
	TSet<FNNLayerIndex>  LayersToCacheOutput;
	FNNCachedLookUpTable CachedLookUpTable;
	FNNBufferLookUpTable NNBufferLookUpTable;

	FTexture2DRHIRef           OutputTexture;
	FUnorderedAccessViewRHIRef OutputTextureUAV;

protected:

	void SetupLayers(FIntPoint ImageDim);
	void Predict_RenderThread(
		FRHICommandListImmediate& RHICmdList,
		FRHITexture*              TargetTexture,
		FShaderResourceViewRHIRef SrcSRV);

	void ResetModel();

	const FNNBuffer* CreateNNBuffer(FNNBufferSize Size);
	void ReleaseNNBuffers();

	void CreateOutputTexture(FIntVector ImageDim);
	void ReleaseOutputTexture();

	void EnqueueAvailableBuffer(const FNNBuffer* Buffer);
	const FNNBuffer* DequeueAvailableBuffer(FNNBufferSize Size);
};

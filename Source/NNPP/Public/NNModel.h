// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NNLayerBase.h"

using FNNBufferSize = int32;
using FNNLayerIndex = int32;
using FNNBufferWeakPtr = TWeakPtr<const struct FNNBuffer>;
using FNNBufferSharedPtr = TSharedPtr<const struct FNNBuffer>;
using FNNBufferQueue = TArray<FNNBufferWeakPtr>;
using FNNBufferLookUpTable = TMap<FNNBufferSize, FNNBufferQueue>;
using FNNCachedLookUpTable = TMap<FNNLayerIndex, FNNBufferWeakPtr>;

struct FNNBuffer
{
	explicit FNNBuffer(FNNBufferSize InSize);
	~FNNBuffer();

	FNNBuffer(const FNNBuffer&) = delete;
	FNNBuffer& operator=(const FNNBuffer&) = delete;

	FORCEINLINE FNNBufferSize GetSize() const
	{
		return Size;
	}

	FORCEINLINE FStructuredBufferRHIRef GetBuffer() const
	{
		return Buffer;
	}

	FORCEINLINE FUnorderedAccessViewRHIRef GetUAV() const
	{
		return BufferUAV;
	}

	FORCEINLINE FShaderResourceViewRHIRef GetSRV() const
	{
		return BufferSRV;
	}

private:

	FNNBufferSize              Size;
	FStructuredBufferRHIRef    Buffer;
	FShaderResourceViewRHIRef  BufferSRV;
	FUnorderedAccessViewRHIRef BufferUAV;
};

class FNNModel
{
public:

	FNNModel();
	~FNNModel();

	void Predict(FRHITexture* TargetTexture, FShaderResourceViewRHIRef SrcSRV, FIntPoint ImageDim);
	void LoadModel(const TCHAR* ModelFile);

protected:

	TArray<TUniquePtr<FNNLayerBase>> Layers;

	FIntPoint CachedImageDim;

	TArray<TSharedRef<FNNBuffer>> NNBuffers;
	TSet<FNNLayerIndex>           LayersToCacheOutput;
	FNNCachedLookUpTable          CachedLookUpTable;
	FNNBufferLookUpTable          NNBufferLookUpTable;

	FTexture2DRHIRef           OutputTexture;
	FUnorderedAccessViewRHIRef OutputTextureUAV;

protected:

	void SetupLayers(FIntPoint ImageDim);
	void Predict_RenderThread(
		FRHICommandListImmediate& RHICmdList,
		FRHITexture*              TargetTexture,
		FShaderResourceViewRHIRef SrcSRV);

	void ResetModel();

	FNNBufferWeakPtr CreateNNBuffer(FNNBufferSize Size);
	void ReleaseNNBuffers();

	void CreateOutputTexture(FIntVector ImageDim);
	void ReleaseOutputTexture();

	void EnqueueAvailableBuffer(FNNBufferWeakPtr Buffer);
	FNNBufferWeakPtr DequeueAvailableBuffer(FNNBufferSize Size);
};

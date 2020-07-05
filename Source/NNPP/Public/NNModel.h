// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NNLayerBase.h"

class FNNModel
{
public:

	FNNModel();

	void Predict(FRHITexture* TargetTexture, FShaderResourceViewRHIRef SrcSRV, FIntPoint ImageDim);
	void LoadModel(const TCHAR* ModelFile);

protected:

	TArray<TUniquePtr<FNNLayerBase>> Layers;

	FIntPoint CachedImageDim;

protected:

	void SetupLayers(FIntPoint ImageDim);
	void Predict_RenderThread(
		FRHICommandListImmediate& RHICmdList,
		FRHITexture*              TargetTexture,
		FShaderResourceViewRHIRef SrcSRV);
};

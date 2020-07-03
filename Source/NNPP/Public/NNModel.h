// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NNLayerBase.h"

class FNNModel
{
public:

	FNNModel();

	void Predict(FUnorderedAccessViewRHIRef DstUAV, FShaderResourceViewRHIRef SrcSRV);

protected:

	TArray<TUniquePtr<FNNLayerBase>> Layers;

	int32 CachedWidth, CachedHeight;

protected:

	void Predict_RenderThread(FRHICommandListImmediate& RHICmdList, FUnorderedAccessViewRHIRef DstUAV, FShaderResourceViewRHIRef SrcSRV);
};

// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NNLayerBase.h"

class FConvLayer final : public FNNLayerBase
{

public:

	int32     Filters;
	FIntPoint KernelSize;
	FIntPoint Stride;

public:

	FConvLayer();

	virtual ~FConvLayer() override;

	virtual void SetupLayer(FIntVector InInputDim) override;
	virtual void ReleaseResource() override;
	virtual void RunLayer_RenderThread(
		FRHICommandList&          RHICmdList,
		FShaderResourceViewRHIRef InputBufferSRV,
		FShaderResourceViewRHIRef OptionalInputBufferSRV = nullptr) override;

private:

	inline static const TArray<int32> kChannels = { 8, 12, 16, 20, 32, 64, 128, 256 };

	FStructuredBufferRHIRef   WeightBuffer;
	FShaderResourceViewRHIRef WeightBufferSRV;

private:

	template <class ShaderClass>
	void DispatchConvShader_RenderThread(FRHICommandList& RHICmdList, FShaderResourceViewRHIRef InputBufferSRV);

};
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

	FStructuredBufferRHIRef   WeightBuffer;
	FShaderResourceViewRHIRef WeightBufferSRV;

};

template <int32 FilterSize>
struct FConvLayerFunctor
{
public:
	static void Run(
		const FConvLayer&          ConvLayer,
		FRHICommandList&           RHICmdList,
		FUnorderedAccessViewRHIRef OutputBufferUAV,
		FShaderResourceViewRHIRef  InputBufferSRV,
		FShaderResourceViewRHIRef  WeightBufferSRV);
};
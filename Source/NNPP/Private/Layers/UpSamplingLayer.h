// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NNLayerBase.h"

class FUpSamplingLayer final : public FNNLayerBase
{
public:

	FIntPoint Size;

public:

	FUpSamplingLayer();

	virtual ~FUpSamplingLayer() override;

	virtual void SetupLayer(FIntVector InInputDim) override;
	virtual void ReleaseResource() override;
	virtual void RunLayer_RenderThread(
		FRHICommandList&          RHICmdList,
		FShaderResourceViewRHIRef InputBufferSRV,
		FShaderResourceViewRHIRef OptionalInputBufferSRV = nullptr) override;

};

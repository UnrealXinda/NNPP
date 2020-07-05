// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NNLayerBase.h"

class FReLULayer final : public FNNLayerBase
{

public:

	FReLULayer();

	virtual ~FReLULayer() override;

	virtual void SetupLayer(FIntVector InInputDim) override;
	virtual void ReleaseRenderResources() override;
	virtual void RunLayer_RenderThread(
		FRHICommandList&          RHICmdList,
		FShaderResourceViewRHIRef InputBufferSRV,
		FShaderResourceViewRHIRef OptionalInputBufferSRV = nullptr) override;

};
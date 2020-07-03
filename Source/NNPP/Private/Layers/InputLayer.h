// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NNLayerBase.h"

class FInputLayer final : public FNNLayerBase
{

public:

	FInputLayer();

	virtual ~FInputLayer() override;

	virtual void SetupLayer(FIntVector InInputDim) override;
	virtual void ReleaseResource() override;
	virtual void RunLayer_RenderThread(
		FRHICommandList&          RHICmdList,
		FShaderResourceViewRHIRef InputBufferSRV,
		FShaderResourceViewRHIRef OptionalInputBufferSRV = nullptr) override;

};
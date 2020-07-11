// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NNLayerBase.h"

class FInputLayer final : public FNNLayerBase
{
public:

	int32 InputChannels;

public:

	FInputLayer();

	virtual ~FInputLayer() override;

	virtual void RunLayer_RenderThread(
		FRHICommandList&           RHICmdList,
		FUnorderedAccessViewRHIRef OutputBufferUAV,
		FShaderResourceViewRHIRef  InputBufferSRV,
		FShaderResourceViewRHIRef  OptionalInputBufferSRV = nullptr) override;
};
// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NNLayerBase.h"

class FAddLayer final : public FNNLayerBase
{
public:

	int32 OtherInputLayerIndex;

public:

	FAddLayer();

	virtual ~FAddLayer() override;

	virtual void RunLayer_RenderThread(
		FRHICommandList&           RHICmdList,
		FUnorderedAccessViewRHIRef OutputBufferUAV,
		FShaderResourceViewRHIRef  InputBufferSRV,
		FShaderResourceViewRHIRef  OptionalInputBufferSRV = nullptr) override;
};

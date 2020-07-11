// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NNLayerBase.h"

class FTanhLayer final : public FNNLayerBase
{

public:

	FTanhLayer();

	virtual ~FTanhLayer() override;

	virtual void RunLayer_RenderThread(
		FRHICommandList&           RHICmdList,
		FUnorderedAccessViewRHIRef OutputBufferUAV,
		FShaderResourceViewRHIRef  InputBufferSRV,
		FShaderResourceViewRHIRef  OptionalInputBufferSRV = nullptr) override;

};
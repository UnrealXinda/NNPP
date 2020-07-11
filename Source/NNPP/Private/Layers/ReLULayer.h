// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NNLayerBase.h"

class FReLULayer final : public FNNLayerBase
{

public:

	FReLULayer();

	virtual ~FReLULayer() override;

	virtual void RunLayer_RenderThread(
		FRHICommandList&           RHICmdList,
		FUnorderedAccessViewRHIRef OutputBufferUAV,
		FShaderResourceViewRHIRef  InputBufferSRV,
		FShaderResourceViewRHIRef  OptionalInputBufferSRV = nullptr) override;

};
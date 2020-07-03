// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NNLayerBase.h"

class FOutputLayer final : public FNNLayerBase
{

public:

	FOutputLayer();

	virtual ~FOutputLayer() override;

	virtual void SetupLayer(FIntVector InInputDim) override;
	virtual void ReleaseResource() override;
	virtual void RunLayer_RenderThread(
		FRHICommandList&          RHICmdList,
		FShaderResourceViewRHIRef InputBufferSRV,
		FShaderResourceViewRHIRef OptionalInputBufferSRV = nullptr) override;

};

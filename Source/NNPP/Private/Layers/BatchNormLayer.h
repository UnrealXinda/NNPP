// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NNLayerBase.h"

class FBatchNormLayer final : public FNNLayerBase
{

public:

	FBatchNormLayer();

	virtual ~FBatchNormLayer() override;

	virtual void RunLayer_RenderThread(
		FRHICommandList&           RHICmdList,
		FUnorderedAccessViewRHIRef OutputBufferUAV,
		FShaderResourceViewRHIRef  InputBufferSRV,
		FShaderResourceViewRHIRef  OptionalInputBufferSRV = nullptr) override;

	void SetupWeightBuffer(const float* WeightData, int32 Size);

private:

	FStructuredBufferRHIRef   WeightBuffer;
	FShaderResourceViewRHIRef WeightBufferSRV;

private:

	void ReleaseWeightBuffers();

};

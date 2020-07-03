// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FNNLayerBase
{
public:

	FIntVector InputDim;
	FIntVector OutputDim;

public:

	FNNLayerBase();
	virtual ~FNNLayerBase();

	FNNLayerBase(const FNNLayerBase&) = delete;
	FNNLayerBase& operator=(const FNNLayerBase&) = delete;

	virtual void SetupLayer(FIntVector InInputDim);
	virtual void ReleaseResource() = 0;
	virtual void RunLayer_RenderThread(
		FRHICommandList&          RHICmdList,
		FShaderResourceViewRHIRef InputBufferSRV,
		FShaderResourceViewRHIRef OptionalInputBufferSRV = nullptr) = 0;

protected:

	FName                      Name;
	FStructuredBufferRHIRef    OutputBuffer;
	FShaderResourceViewRHIRef  OutputBufferSRV;
	FUnorderedAccessViewRHIRef OutputBufferUAV;

};

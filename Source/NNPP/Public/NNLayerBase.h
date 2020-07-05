// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

enum class ENNLayerType
{
	Input,
	Output,
	Conv,
	BatchNorm,
	ReLU,
	Add,
	UpSampling,
	Tanh,
};

class FNNLayerBase
{
public:


public:

	explicit FNNLayerBase(ENNLayerType InType);
	virtual ~FNNLayerBase();

	FNNLayerBase(const FNNLayerBase&) = delete;
	FNNLayerBase& operator=(const FNNLayerBase&) = delete;

	virtual void SetupLayer(FIntVector InInputDim);
	virtual void ReleaseRenderResources();
	virtual void RunLayer_RenderThread(
		FRHICommandList&          RHICmdList,
		FShaderResourceViewRHIRef InputBufferSRV,
		FShaderResourceViewRHIRef OptionalInputBufferSRV = nullptr) = 0;

	FORCEINLINE FShaderResourceViewRHIRef GetOutputBufferSRV() const
	{
		return OutputBufferSRV;
	}

	FORCEINLINE void SetName(FName InName)
	{
		Name = InName;
	}

	FORCEINLINE FName GetName() const
	{
		return Name;
	}

	FORCEINLINE ENNLayerType GetLayerType() const
	{
		return LayerType;
	}

	FORCEINLINE FIntVector GetInputDim() const
	{
		return InputDim;
	}

	FORCEINLINE FIntVector GetOutputDim() const
	{
		return OutputDim;
	}

protected:

	FName                      Name;
	const ENNLayerType         LayerType;
	FIntVector                 InputDim;
	FIntVector                 OutputDim;

	FStructuredBufferRHIRef    OutputBuffer;
	FShaderResourceViewRHIRef  OutputBufferSRV;
	FUnorderedAccessViewRHIRef OutputBufferUAV;
};

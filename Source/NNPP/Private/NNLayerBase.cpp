// Copyright Epic Games, Inc. All Rights Reserved.

#include "NNLayerBase.h"

FNNLayerBase::FNNLayerBase(ENNLayerType InType) :
	LayerType(InType)
{

}

FNNLayerBase::~FNNLayerBase()
{
	ReleaseRenderResources();
}

void FNNLayerBase::SetupLayer(FIntVector InInputDim)
{
	InputDim = InInputDim;
}

void FNNLayerBase::ReleaseRenderResources()
{
	ReleaseRenderResource<FStructuredBufferRHIRef>(OutputBuffer);
	ReleaseRenderResource<FShaderResourceViewRHIRef>(OutputBufferSRV);
	ReleaseRenderResource<FUnorderedAccessViewRHIRef>(OutputBufferUAV);
}

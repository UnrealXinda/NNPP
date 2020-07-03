// Copyright Epic Games, Inc. All Rights Reserved.

#include "NNLayerBase.h"

FNNLayerBase::FNNLayerBase()
{

}

FNNLayerBase::~FNNLayerBase()
{

}

void FNNLayerBase::SetupLayer(FIntVector InInputDim)
{
	InputDim  = InInputDim;
	OutputDim = InInputDim;
}

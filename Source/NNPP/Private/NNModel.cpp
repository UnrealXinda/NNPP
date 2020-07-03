// Copyright Epic Games, Inc. All Rights Reserved.

#include "NNModel.h"

FNNModel::FNNModel()
{

}

void FNNModel::Predict(FUnorderedAccessViewRHIRef DstUAV, FShaderResourceViewRHIRef SrcSRV)
{
	ENQUEUE_RENDER_COMMAND(KaleidoComputeCommand)
	(
		[DstUAV, SrcSRV, this](FRHICommandListImmediate& RHICmdList)
		{
			Predict_RenderThread(RHICmdList, DstUAV, SrcSRV);
		}
	);
}

void FNNModel::Predict_RenderThread(FRHICommandListImmediate& RHICmdList, FUnorderedAccessViewRHIRef DstUAV, FShaderResourceViewRHIRef SrcSRV)
{
	check(IsInRenderingThread());

}

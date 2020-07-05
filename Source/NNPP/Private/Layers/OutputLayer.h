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
	virtual void ReleaseRenderResources() override;
	virtual void RunLayer_RenderThread(
		FRHICommandList&          RHICmdList,
		FShaderResourceViewRHIRef InputBufferSRV,
		FShaderResourceViewRHIRef OptionalInputBufferSRV = nullptr) override;

	void SetupOutputDimension(FIntVector InOutputDim);

	void CopyToTargetTexture_RenderThread(FRHICommandList& RHICmdList, FRHITexture* TargetTexture);

private:

	FTexture2DRHIRef           OutputTexture;
	FUnorderedAccessViewRHIRef OutputTextureUAV;

private:
	
	void ReleaseTextureResources();
};

// Fill out your copyright notice in the Description page of Project Settings.


#include "NNPostProcessingComponent.h"
#include "NNPP.h"
#include "Layers/NNLayerUtils.h"
#include "Misc/FileHelper.h"
#include "Engine/TextureRenderTarget2D.h"

UNNPostProcessingComponent::UNNPostProcessingComponent(const FObjectInitializer& Initializer) :
	NNModel(MakeUnique<FNNModel>())
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
}


// Called when the game starts
void UNNPostProcessingComponent::BeginPlay()
{
	Super::BeginPlay();

	FString PluginDir = FNNPPModule::GetPluginDirectory();
	FString ModelFilePath = PluginDir + "/Models/" + ModelFileName;

	NNModel->LoadModel(*ModelFilePath);
}


// Called every frame
void UNNPostProcessingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	RunNeuralNetwork();
}

void UNNPostProcessingComponent::RunNeuralNetwork()
{
	FRHITexture2D* InputTextureRef = InputTexture->Resource->TextureRHI->GetTexture2D();
	FShaderResourceViewRHIRef InputTextureSRV = RHICreateShaderResourceView(InputTextureRef, 0);

	int32 TextureWidth = InputTexture->SizeX;
	int32 TextureHeight = InputTexture->SizeY;

	if (OutputTexture)
	{
		FTextureReferenceRHIRef OutputRenderTargetTextureRHI = OutputTexture->TextureReference.TextureReferenceRHI;
		FRHITexture* RenderTargetTextureRef = OutputRenderTargetTextureRHI->GetTextureReference()->GetReferencedTexture();

		NNModel->Predict(RenderTargetTextureRef, InputTextureSRV, FIntPoint(TextureWidth, TextureHeight));
	}
}

void UNNPostProcessingComponent::Render()
{
	RunNeuralNetwork();
}


// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NNModel.h"
#include "NNPostProcessingComponent.generated.h"


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent), ClassGroup = Rendering, DisplayName = "NNPostProcessingComponent")
class NNPP_API UNNPostProcessingComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UNNPostProcessingComponent(const FObjectInitializer& Initializer);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Neural Network Model")
	FString ModelFileName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Neural Network Model")
	class UTextureRenderTarget2D* InputTexture;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Neural Network Model")
	class UTextureRenderTarget2D* OutputTexture;

	TUniquePtr<FNNModel> NNModel;

protected:

	virtual void BeginPlay() override;
	void RunNeuralNetwork();
};

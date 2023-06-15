// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NNPPSettings.generated.h"

class UNNEModelData;

DECLARE_DELEGATE(FOnNNPPModelChanged)

UCLASS(Config=NNPPSettings, defaultconfig)
class UNNPPSettings : public UObject
{
	GENERATED_BODY()

public:
	virtual void PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent) override;

public:
	UPROPERTY(EditDefaultsOnly, Config)
	bool bEnabled;

	UPROPERTY(EditDefaultsOnly, Config, meta=(AllowedClasses="NNEModelData"))
	FSoftObjectPath NNPPModelPath;

	FOnNNPPModelChanged OnNnppModelChanged;
};
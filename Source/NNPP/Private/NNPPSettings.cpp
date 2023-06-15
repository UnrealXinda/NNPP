// Fill out your copyright notice in the Description page of Project Settings.


#include "NNPPSettings.h"

void UNNPPSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	UObject::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UNNPPSettings, NNPPModelPath))
	{
		OnNnppModelChanged.ExecuteIfBound();
	}
}
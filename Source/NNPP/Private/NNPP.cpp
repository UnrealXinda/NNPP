// Copyright Epic Games, Inc. All Rights Reserved.

#include "NNPP.h"

#include "ISettingsModule.h"
#include "NNPPSettings.h"
#include "NNPPViewExtension.h"
#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "FNNPPModule"

void FNNPPModule::StartupModule()
{
	[[maybe_unused]] FNNPPViewExtension& ViewExtension = FNNPPViewExtension::Get();

	UNNPPSettings* Settings = GetMutableDefault<UNNPPSettings>();
	Settings->OnNnppModelChanged.BindStatic(HandleOnNNPPModelChanged);

	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->RegisterSettings(
			"Project",
			"Plugins",
			"NNPP_Settings",
			LOCTEXT("RuntimeSettingsName", "NNPP Settings"),
			LOCTEXT("RuntimeSettingsDescription", "Configure NNPP setting"),
			GetMutableDefault<UNNPPSettings>());
	}
}

void FNNPPModule::ShutdownModule()
{
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings(
			"Project",
			"Plugins",
			"NNPP_Settings");
	}
}

void FNNPPModule::HandleOnNNPPModelChanged()
{
	FNNPPViewExtension& ViewExtension = FNNPPViewExtension::Get();
	ViewExtension.InitializeModel();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FNNPPModule, NNPP)
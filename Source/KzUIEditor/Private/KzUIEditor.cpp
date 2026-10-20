// Copyright 2026 kirzo

#include "KzUIEditor.h"
#include "KzUIEditorStyle.h"
#include "Customization/KzSwitchOnOptionCustomization.h"
#include "K2Node_SwitchOnKzOption.h"
#include "KzUIFlipbook.h"
#include "KzUIIconTheme.h"
#include "KzUISoundTheme.h"

void FKzUIEditorModule::OnStartupModule()
{
	FKzUIEditorStyle::Initialize();

	RegisterClassLayout<UK2Node_SwitchOnKzOption, FKzSwitchOnOptionCustomization>();

	RegisterAssetTypeAction<UKzUISoundTheme>(
		KzAssetCategoryBit,
		INVTEXT("UI Sound Theme"),
		FColor(64, 201, 255),
		{ INVTEXT("UI") });

	RegisterAssetTypeAction<UKzUIIconTheme>(
		KzAssetCategoryBit,
		INVTEXT("UI Icon Theme"),
		FColor(64, 201, 255),
		{ INVTEXT("UI") });

	RegisterAssetTypeAction<UKzUIFlipbook>(
		KzAssetCategoryBit,
		INVTEXT("UI Flipbook"),
		FColor(64, 201, 255),
		{ INVTEXT("UI") });
}

void FKzUIEditorModule::OnShutdownModule()
{
	FKzUIEditorStyle::Shutdown();
}

IMPLEMENT_MODULE(FKzUIEditorModule, KzUIEditor)
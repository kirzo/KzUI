// Copyright 2026 kirzo

#include "KzUIEditor.h"
#include "KzUIEditorStyle.h"
#include "Customization/KzSwitchOnOptionCustomization.h"
#include "K2Node_SwitchOnKzOption.h"
#include "KzUIFlipbook.h"
#include "KzUIIconTheme.h"
#include "KzUISoundTheme.h"

#include "Editor.h"
#include "Kismet2/BlueprintEditorUtils.h"

/** Switch on Option pins follow the widget tree, which the compiler never asks about: refreshed here, a renamed option with links fails to compile instead of falling through to Default. */
static void RefreshSwitchOnOptionNodes(UBlueprint* Blueprint)
{
	TArray<UK2Node_SwitchOnKzOption*> Nodes;
	FBlueprintEditorUtils::GetAllNodesOfClass(Blueprint, Nodes);
	for (UK2Node_SwitchOnKzOption* Node : Nodes)
	{
		Node->RefreshOptions();
	}
}

void FKzUIEditorModule::OnStartupModule()
{
	FKzUIEditorStyle::Initialize();

	RegisterClassLayout<UK2Node_SwitchOnKzOption, FKzSwitchOnOptionCustomization>();

	if (GEditor)
	{
		BlueprintPreCompileHandle = GEditor->OnBlueprintPreCompile().AddStatic(&RefreshSwitchOnOptionNodes);
	}

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
	if (GEditor)
	{
		GEditor->OnBlueprintPreCompile().Remove(BlueprintPreCompileHandle);
	}
	FKzUIEditorStyle::Shutdown();
}

IMPLEMENT_MODULE(FKzUIEditorModule, KzUIEditor)
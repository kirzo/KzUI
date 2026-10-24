// Copyright 2026 kirzo

#include "KzUIEditor.h"
#include "KzUIEditorStyle.h"
#include "Customization/KzSwitchOnOptionCustomization.h"
#include "K2Node_SwitchOnKzOption.h"

#include "Editor.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"

/** Switch on Option pins follow the widget tree, which the compiler never asks about: refreshed here, a renamed option keeps its links and a deleted one with links fails to compile instead of falling through to Default. */
static void RefreshSwitchOnOptionNodes(UBlueprint* Blueprint)
{
	TArray<UK2Node_SwitchOnKzOption*> Nodes;
	FBlueprintEditorUtils::GetAllNodesOfClass(Blueprint, Nodes);
	for (UK2Node_SwitchOnKzOption* Node : Nodes)
	{
		Node->RefreshOptions();
	}
}

void FKzUIEditorModule::StartupModule()
{
	FKzUIEditorStyle::Initialize();

	FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyEditor.RegisterCustomClassLayout(UK2Node_SwitchOnKzOption::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FKzSwitchOnOptionCustomization::MakeInstance));

	if (GEditor)
	{
		BlueprintPreCompileHandle = GEditor->OnBlueprintPreCompile().AddStatic(&RefreshSwitchOnOptionNodes);
	}
}

void FKzUIEditorModule::ShutdownModule()
{
	if (GEditor)
	{
		GEditor->OnBlueprintPreCompile().Remove(BlueprintPreCompileHandle);
	}

	if (FPropertyEditorModule* PropertyEditor = FModuleManager::GetModulePtr<FPropertyEditorModule>("PropertyEditor"))
	{
		PropertyEditor->UnregisterCustomClassLayout(UK2Node_SwitchOnKzOption::StaticClass()->GetFName());
	}

	FKzUIEditorStyle::Shutdown();
}

IMPLEMENT_MODULE(FKzUIEditorModule, KzUIEditor)
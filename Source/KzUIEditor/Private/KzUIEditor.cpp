// Copyright 2026 kirzo

#include "KzUIEditor.h"
#include "KzUIEditorStyle.h"
#include "KzUIFlipbook.h"
#include "KzUIFlipbookThumbnailRenderer.h"
#include "Customization/KzSwitchOnOptionCustomization.h"
#include "Customization/KzUIFlipbookCustomization.h"
#include "Factories/KzUIFlipbookFactory.h"
#include "K2Node_SwitchOnKzOption.h"

#include "AssetToolsModule.h"
#include "ContentBrowserMenuContexts.h"
#include "ContentBrowserModule.h"
#include "Editor.h"
#include "Engine/Texture2D.h"
#include "IContentBrowserSingleton.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "ThumbnailRendering/ThumbnailManager.h"
#include "ToolMenus.h"

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

/** One flipbook page per texture, in name order, sized to the first: right for frame sequences, a starting point to set FrameSize for a single sprite sheet. */
static void CreateFlipbookFromTextures(TArray<UTexture2D*> Textures)
{
	Textures.Sort([](const UTexture2D& A, const UTexture2D& B) { return A.GetName() < B.GetName(); });

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	FString PackageName, AssetName;
	AssetTools.CreateUniqueAssetName(Textures[0]->GetOutermost()->GetName(), TEXT("_Flipbook"), PackageName, AssetName);

	UKzUIFlipbook* Flipbook = Cast<UKzUIFlipbook>(AssetTools.CreateAsset(AssetName, FPackageName::GetLongPackagePath(PackageName), UKzUIFlipbook::StaticClass(), NewObject<UKzUIFlipbookFactory>()));
	if (!Flipbook)
	{
		return;
	}

	Flipbook->Modify();
	Flipbook->Textures = TArray<TObjectPtr<UTexture2D>>(Textures);
	Flipbook->FrameSize = FVector2D(Textures[0]->GetSizeX(), Textures[0]->GetSizeY());
	Flipbook->TotalFrames = Textures.Num();
	Flipbook->PostEditChange();

	FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser").Get().SyncBrowserToAssets(TArray<UObject*>({ Flipbook }));
}

/** "Create UI Flipbook" in the Content Browser menu of textures. */
static void RegisterFlipbookMenu()
{
	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("ContentBrowser.AssetContextMenu.Texture2D");
	if (!Menu)
	{
		return;
	}

	Menu->FindOrAddSection("GetAssetActions").AddDynamicEntry("KzUICreateFlipbook", FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& Section)
		{
			const UContentBrowserAssetContextMenuContext* Context = Section.FindContext<UContentBrowserAssetContextMenuContext>();
			if (!Context || Context->SelectedAssets.IsEmpty())
			{
				return;
			}

			// The textures load on click, not on right click
			const TArray<FAssetData> Assets = Context->SelectedAssets;
			Section.AddMenuEntry(
				"KzUICreateFlipbook",
				INVTEXT("Create UI Flipbook"),
				INVTEXT("Creates a KzUI flipbook with the selected textures as its frames, in name order."),
				FSlateIcon("KzUIEditorStyle", "ClassIcon.KzUIFlipbook"),
				FUIAction(FExecuteAction::CreateLambda([Assets]()
					{
						TArray<UTexture2D*> Textures;
						for (const FAssetData& Asset : Assets)
						{
							if (UTexture2D* Texture = Cast<UTexture2D>(Asset.GetAsset()))
							{
								Textures.Add(Texture);
							}
						}
						if (!Textures.IsEmpty())
						{
							CreateFlipbookFromTextures(MoveTemp(Textures));
						}
					}
				))
			);
		}
	));
}

void FKzUIEditorModule::StartupModule()
{
	FKzUIEditorStyle::Initialize();

	FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyEditor.RegisterCustomClassLayout(UK2Node_SwitchOnKzOption::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FKzSwitchOnOptionCustomization::MakeInstance));
	PropertyEditor.RegisterCustomClassLayout(UKzUIFlipbook::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FKzUIFlipbookCustomization::MakeInstance));

	UThumbnailManager::Get().RegisterCustomRenderer(UKzUIFlipbook::StaticClass(), UKzUIFlipbookThumbnailRenderer::StaticClass());
	ToolMenusHandle = UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateLambda([this]()
		{
			FToolMenuOwnerScoped OwnerScoped(this);
			RegisterFlipbookMenu();
		}
	));

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
		PropertyEditor->UnregisterCustomClassLayout(UKzUIFlipbook::StaticClass()->GetFName());
	}

	if (UObjectInitialized())
	{
		UThumbnailManager::Get().UnregisterCustomRenderer(UKzUIFlipbook::StaticClass());
	}
	UToolMenus::UnRegisterStartupCallback(ToolMenusHandle);
	UToolMenus::UnregisterOwner(this);

	FKzUIEditorStyle::Shutdown();
}

IMPLEMENT_MODULE(FKzUIEditorModule, KzUIEditor)
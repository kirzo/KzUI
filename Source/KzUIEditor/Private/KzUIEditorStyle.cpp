// Copyright 2026 kirzo

#include "KzUIEditorStyle.h"

#include "Brushes/SlateImageBrush.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleRegistry.h"

TSharedPtr<FKzUIEditorStyle> FKzUIEditorStyle::Instance;

void FKzUIEditorStyle::Initialize()
{
	if (!Instance)
	{
		Instance = MakeShared<FKzUIEditorStyle>();
		FSlateStyleRegistry::RegisterSlateStyle(*Instance);
	}
}

void FKzUIEditorStyle::Shutdown()
{
	if (Instance)
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*Instance);
		Instance.Reset();
	}
}

FKzUIEditorStyle::FKzUIEditorStyle()
	: FSlateStyleSet(FName("KzUIEditorStyle"))
{
	const FString ResourcesDir = IPluginManager::Get().FindPlugin(TEXT("KzUI"))->GetBaseDir() / TEXT("Resources");
	SetContentRoot(ResourcesDir);
	SetCoreContentRoot(ResourcesDir);

	// White line icon for lists, tabs and menus; colour figure for the Content Browser, which Blueprints of the text styles inherit
	const TPair<const TCHAR*, const TCHAR*> Classes[] = {
		{ TEXT("KzUIIconTheme"), TEXT("IconTheme") },
		{ TEXT("KzUISoundTheme"), TEXT("SoundTheme") },
		{ TEXT("KzUIFlipbook"), TEXT("Flipbook") },
		{ TEXT("KzTextStyle"), TEXT("TextStyle") },
		{ TEXT("KzTextBlockStyle"), TEXT("TextBlockStyle") }
	};
	for (const TPair<const TCHAR*, const TCHAR*>& Class : Classes)
	{
		Set(*FString::Printf(TEXT("ClassIcon.%s"), Class.Key), new FSlateVectorImageBrush(RootToContentDir(FString(TEXT("Icons/")) + Class.Value, TEXT(".svg")), FVector2D(16.0f, 16.0f)));
		Set(*FString::Printf(TEXT("ClassThumbnail.%s"), Class.Key), new FSlateVectorImageBrush(RootToContentDir(FString(TEXT("Icons/")) + Class.Value + TEXT("Color"), TEXT(".svg")), FVector2D(64.0f, 64.0f)));
	}

	// UMG palette and hierarchy, in the engine's 16 px white style with the focus yellow; Widget Blueprints show the icon of their native parent in the hierarchy
	const TCHAR* Widgets[] = {
		TEXT("KzImage"), TEXT("KzInputIcon"), TEXT("KzTextBlock"), TEXT("KzWidgetSwitcher"), TEXT("KzSwitcherPage"), TEXT("KzSplashPage"),
		TEXT("KzUserWidget"), TEXT("KzNavigableWidget"), TEXT("KzOptionWidget"), TEXT("KzSpinner"), TEXT("KzSlider"), TEXT("KzToggle"), TEXT("KzPromptWidget"), TEXT("KzSplashWidget")
	};
	for (const TCHAR* Widget : Widgets)
	{
		Set(*FString::Printf(TEXT("ClassIcon.%s"), Widget), new FSlateVectorImageBrush(RootToContentDir(FString(TEXT("Icons/Widgets/")) + Widget, TEXT(".svg")), FVector2D(16.0f, 16.0f)));
	}
}
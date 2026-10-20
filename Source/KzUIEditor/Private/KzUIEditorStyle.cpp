// Copyright 2026 kirzo

#include "KzUIEditorStyle.h"

FKzUIEditorStyle::FKzUIEditorStyle()
	: TKzEditorStyle_Base(FName("KzUIEditorStyle"))
{
	SetupPluginResources(TEXT("KzUI"));

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
}
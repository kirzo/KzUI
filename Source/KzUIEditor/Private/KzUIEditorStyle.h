// Copyright 2026 kirzo

#pragma once

#include "Styling/SlateStyle.h"

/** Class icons and thumbnails of the KzUI assets and widgets. */
class FKzUIEditorStyle : public FSlateStyleSet
{
public:
	FKzUIEditorStyle();

	static void Initialize();
	static void Shutdown();

private:
	static TSharedPtr<FKzUIEditorStyle> Instance;
};
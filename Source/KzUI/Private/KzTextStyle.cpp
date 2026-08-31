// Copyright 2026 kirzo

#include "KzTextStyle.h"
#include "Styling/CoreStyle.h"

UKzTextStyle::UKzTextStyle()
{
	Font = FCoreStyle::GetDefaultFontStyle("Regular", 24);
	Color = FSlateColor(FLinearColor::White);
	ShadowOffset = FVector2D(1.0, 1.0);
	ShadowColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
}
// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

/** Details panel of a UI flipbook: adds a looping playback preview on top of its properties. */
class FKzUIFlipbookCustomization : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
};
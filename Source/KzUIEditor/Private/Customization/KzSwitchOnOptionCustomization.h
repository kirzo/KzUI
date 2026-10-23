// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

/** Details panel for the Switch on Option node: one checkbox per option to toggle its exec pin. */
class FKzSwitchOnOptionCustomization : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	virtual void CustomizeDetails(const TSharedPtr<IDetailLayoutBuilder>& DetailBuilder) override;

private:
	/** Rebuilt after a toggle: reconstructing the node may have read a changed widget tree. */
	TWeakPtr<IDetailLayoutBuilder> LayoutBuilder;
};
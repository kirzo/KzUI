// Copyright 2026 kirzo

#pragma once

#include "Components/ContentWidget.h"
#include "KzSwitcherPage.generated.h"

class SBox;

/** One page of a UKzWidgetSwitcher: wraps any widget and carries the page identity. */
UCLASS(meta = (PrioritizeCategories = "KzUI"))
class KZUI_API UKzSwitcherPage : public UContentWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = "KzUI|Page", EditAnywhere)
	FText Title;

	//~ Begin UWidget Interface
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif
	//~ End UWidget Interface

protected:
	//~ Begin UPanelWidget Interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void OnSlotAdded(UPanelSlot* InSlot) override;
	virtual void OnSlotRemoved(UPanelSlot* InSlot) override;
	//~ End UPanelWidget Interface

private:
	TSharedPtr<SBox> MyBox;
};
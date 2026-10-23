// Copyright 2026 kirzo

#pragma once

#include "Components/WidgetSwitcher.h"
#include "KzWidgetSwitcher.generated.h"

class UKzUserWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FKzActivePageChangedSignature, int32, Index, UWidget*, Page, FText, Title);

/**
 * Widget switcher whose pages plug into the KzUI input model. Pages are ideally UKzSwitcherPage
 * containers, which carry the page title. Plain content belongs to the owning screen, which keeps
 * handling the input; the KzUserWidgets of a page (at any depth of this widget tree, nested
 * switchers aside) are registered on the player's widget stack while it is active, taking focus
 * and suspending the owning screen, and their RemoveFromStack returns the switcher to the previous
 * page instead of removing them, so the same widget works as a screen or as a page. The default
 * page should be plain content: a KzUserWidget page active from construction is not managed.
 */
UCLASS(meta = (PrioritizeCategories = "KzUI"))
class KZUI_API UKzWidgetSwitcher : public UWidgetSwitcher
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = "KzUI|Events", BlueprintAssignable)
	FKzActivePageChangedSignature OnActivePageChanged;

	//~ Begin UWidgetSwitcher Interface
	virtual void SetActiveWidgetIndex(int32 Index) override;
	virtual void SetActiveWidget(UWidget* Widget) override;
	//~ End UWidgetSwitcher Interface

	//~ Begin UWidget Interface
	virtual void OnWidgetRebuilt() override;
	//~ End UWidget Interface

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif

	/** Returns to the page that was active before the current one; the first page when there is no history. */
	UFUNCTION(Category = "KzUI|Switcher", BlueprintCallable)
	void GoBack();

	/** Title of the active KzSwitcherPage; empty for other pages. Handy for the initial title, before any page change. */
	UFUNCTION(Category = "KzUI|Switcher", BlueprintPure)
	FText GetActivePageTitle() const;

	/** Nearest KzUserWidget up the outer chain: the screen hosting this switcher. */
	UKzUserWidget* GetOwningScreen() const;

	/** Nearest ancestor KzWidgetSwitcher within the same widget tree, reporting whether Widget sits on its active page. */
	static UKzWidgetSwitcher* FindOwningSwitcher(const UWidget* Widget, bool& bOutOnActivePage);

private:
	void HandleActivePageChanged(int32 PreviousIndex);

	/** The KzUserWidgets this switcher manages on a page. */
	TArray<UKzUserWidget*> GetPageScreens(UWidget* Page) const;

	TArray<int32> PageHistory;

	bool bNavigatingBack = false;

	bool bOwnerInputDisabledBySwitcher = false;
};
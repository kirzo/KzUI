// Copyright 2026 kirzo

#pragma once

#include "Components/WidgetSwitcher.h"
#include "KzWidgetSwitcher.generated.h"

class UKzUserWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FKzActivePageChangedSignature, int32, Index, UWidget*, Page, FText, Title);

/**
 * Widget switcher whose pages plug into the KzUI input model. Pages are ideally UKzSwitcherPage
 * containers, which carry the page title. Plain content belongs to the owning screen, which keeps
 * handling the input; a KzUserWidget page (direct child or KzSwitcherPage content) is registered
 * on the player's widget stack while active, taking focus and suspending the owning screen, and
 * its RemoveFromStack returns the switcher to the previous page instead of removing it, so the
 * same widget works as a screen or as a page. The default page should be plain content: a
 * KzUserWidget page active from construction is not managed.
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

private:
	void HandleActivePageChanged(int32 PreviousIndex);

	/** Nearest KzUserWidget up the outer chain: the screen hosting this switcher. */
	UKzUserWidget* GetOwningScreen() const;

	/** The KzUserWidget a page brings in: the page itself or the content of its KzSwitcherPage wrapper. */
	static UKzUserWidget* GetPageScreen(UWidget* Page);

	TArray<int32> PageHistory;

	bool bNavigatingBack = false;

	bool bOwnerInputDisabledBySwitcher = false;
};
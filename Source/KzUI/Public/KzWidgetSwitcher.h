// Copyright 2026 kirzo

#pragma once

#include "Components/WidgetSwitcher.h"
#include "KzWidgetSwitcher.generated.h"

class UKzUserWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FKzActivePageChangedSignature, int32, Index, UWidget*, Page, FText, Title);

UENUM(BlueprintType)
enum class EKzUISwitcherMode : uint8
{
	/** Page changes are remembered, and a page's RemoveFromStack goes back to the previous page. */
	History,
	/** Pages are tabs: nothing is remembered, the Previous/Next a page leaves unhandled move between tabs (wrapping), and a page's RemoveFromStack removes the screen hosting the switcher. */
	Tabs
};

/**
 * Widget switcher whose pages plug into the KzUI input model. Pages are ideally UKzSwitcherPage
 * containers, which carry the page title. Plain content belongs to the owning screen, which keeps
 * handling the input; the KzUserWidgets of a page (at any depth of this widget tree, nested
 * switchers aside) are registered on the player's widget stack while it is active, taking focus
 * and suspending the owning screen, and their RemoveFromStack returns the switcher to the previous
 * page instead of removing them (Mode changes that for tabs), so the same widget works as a screen or as a page. The default
 * page should be plain content: a KzUserWidget page active from construction is not managed.
 */
UCLASS(meta = (PrioritizeCategories = "KzUI"))
class KZUI_API UKzWidgetSwitcher : public UWidgetSwitcher
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = "KzUI|Switcher", EditAnywhere, BlueprintReadOnly)
	EKzUISwitcherMode Mode = EKzUISwitcherMode::History;

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

	/** Activates the next page, wrapping to the first. */
	UFUNCTION(Category = "KzUI|Switcher", BlueprintCallable)
	void NextPage();

	/** Activates the previous page, wrapping to the last. */
	UFUNCTION(Category = "KzUI|Switcher", BlueprintCallable)
	void PreviousPage();

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
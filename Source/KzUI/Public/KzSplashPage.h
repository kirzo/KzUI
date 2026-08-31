// Copyright 2026 kirzo

#pragma once

#include "Components/ContentWidget.h"
#include "KzSplashPage.generated.h"

class SBox;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FKzSplashPageShownSignature);

/** One page of a UKzSplashWidget sequence: wraps any widget and carries the page timing. */
UCLASS(meta = (PrioritizeCategories = "KzUI"))
class KZUI_API UKzSplashPage : public UContentWidget
{
	GENERATED_BODY()

public:
	/** Time on screen between the fades. */
	UPROPERTY(Category = "KzUI|Splash", EditAnywhere, meta = (ClampMin = 0))
	float Duration = 3.0f;

	UPROPERTY(Category = "KzUI|Splash", EditAnywhere, meta = (ClampMin = 0))
	float FadeInTime = 0.5f;

	UPROPERTY(Category = "KzUI|Splash", EditAnywhere, meta = (ClampMin = 0))
	float FadeOutTime = 0.5f;

	UPROPERTY(Category = "KzUI|Splash", EditAnywhere)
	bool bSkippable = true;

	/** Played when the page is shown. */
	UPROPERTY(Category = "KzUI|Splash", EditAnywhere)
	TObjectPtr<USoundBase> Sound;

	/** The splash sequence activated this page. */
	UPROPERTY(Category = "KzUI|Events", BlueprintAssignable)
	FKzSplashPageShownSignature OnShown;

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
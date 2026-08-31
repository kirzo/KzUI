// Copyright 2026 kirzo

#pragma once

#include "KzUserWidget.h"
#include "KzSplashWidget.generated.h"

class UKzSplashPage;
class UWidgetSwitcher;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FKzSplashFinishedSignature);

/**
 * Plays the children of its Pages switcher as a splash sequence: fade in, hold, fade out, next.
 * Pages should be UKzSplashPage containers carrying their own timing; any key or click from any
 * device skips the current page when it allows it. Nothing happens after OnFinished: the game
 * decides what comes next.
 */
UCLASS(Abstract, meta = (PrioritizeCategories = "KzUI"))
class KZUI_API UKzSplashWidget : public UKzUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = "KzUI|Events", BlueprintAssignable)
	FKzSplashFinishedSignature OnFinished;

protected:
	/** Sequence pages, played in child order. */
	UPROPERTY(Category = "KzUI", BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> Pages;

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:
	/** Advances the current page to its fade out. Returns whether it acted: the page may not be skippable. */
	UFUNCTION(Category = "KzUI|Splash", BlueprintCallable)
	bool Skip();

	UFUNCTION(Category = "KzUI|Splash", BlueprintImplementableEvent, meta = (DisplayName = "On Page Changed"))
	void ReceiveOnPageChanged(int32 Index, UWidget* Page);

	UFUNCTION(Category = "KzUI|Splash", BlueprintImplementableEvent, meta = (DisplayName = "On Finished"))
	void ReceiveOnFinished();

private:
	enum class EKzSplashState : uint8 { Inactive, FadeIn, Hold, FadeOut, Finished };

	void StartPage(int32 Index);
	void BeginFadeOut();
	void Finish();
	UWidget* GetCurrentPage() const;
	UKzSplashPage* GetCurrentSplashPage() const;

	EKzSplashState State = EKzSplashState::Inactive;
	float StateTime = 0.0f;
	float FadeOutStartOpacity = 1.0f;
	int32 PageIndex = INDEX_NONE;

	TSharedPtr<class FKzSplashInputListener> InputListener;
};
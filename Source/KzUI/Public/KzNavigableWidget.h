// Copyright 2026 kirzo

#pragma once

#include "KzUserWidget.h"
#include "KzNavigableWidget.generated.h"

class UButton;

/**
 * Widget that navigates a set of option widgets with directional input, mouse or gamepad.
 * Navigation moves the hover; Accept commits the hovered option as the selection. Options
 * may implement IKzSelectableWidgetInterface to react to both.
 */
UCLASS(Abstract, meta = (PrioritizeCategories = "KzUI"))
class KZUI_API UKzNavigableWidget : public UKzUserWidget
{
	GENERATED_BODY()

public:
	/** Wrap the navigation around the edges. */
	UPROPERTY(Category = "KzUI|Navigation", EditAnywhere)
	bool bLoopNavigation = false;

	/** Previous/Next move the hover through the options in tree order. */
	UPROPERTY(Category = "KzUI|Navigation", EditAnywhere)
	bool bPreviousNextNavigation = false;

	/** Overrides the project-settings HoverSound for this widget. */
	UPROPERTY(Category = "KzUI|Sound", EditAnywhere)
	TObjectPtr<USoundBase> HoverSoundOverride;

	/** Overrides the project-settings SelectSound for this widget. */
	UPROPERTY(Category = "KzUI|Sound", EditAnywhere)
	TObjectPtr<USoundBase> SelectSoundOverride;

protected:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UWidget>> Options;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> HoveredOption;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> SelectedOption;

public:
	UKzNavigableWidget(const FObjectInitializer& ObjectInitializer);

	//~ Begin UUserWidget Interface
protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	//~ End UUserWidget Interface

	//~ Begin UKzUserWidget Interface
	virtual bool CanHandleAcceptInput_Implementation() override;
	virtual bool CanHandleDownInput_Implementation() override;
	virtual bool CanHandleUpInput_Implementation() override;
	virtual bool CanHandleLeftInput_Implementation() override;
	virtual bool CanHandleRightInput_Implementation() override;
	virtual bool CanHandlePreviousInput_Implementation() override;
	virtual bool CanHandleNextInput_Implementation() override;

	virtual void AcceptInputTriggered_Implementation(bool Handled) override;
	virtual void DownInputTriggered_Implementation(bool Handled) override;
	virtual void UpInputTriggered_Implementation(bool Handled) override;
	virtual void LeftInputTriggered_Implementation(bool Handled) override;
	virtual void RightInputTriggered_Implementation(bool Handled) override;
	virtual void PreviousInputTriggered_Implementation(bool Handled) override;
	virtual void NextInputTriggered_Implementation(bool Handled) override;
	//~ End UKzUserWidget Interface

	virtual void PlayInputSound(EKzUIInputType Input) override;

	virtual void OnActivated() override;

	virtual void RefreshHover();

	/** Explicit designer navigation rules first, then the nearest selectable option in the direction (Previous/Next go in tree order). */
	UWidget* GetTargetWidget(UWidget* Source, EKzUIInputType Input);

public:
	UFUNCTION(Category = "KzUI|Navigation", BlueprintCallable)
	bool HasAnyOption() const { return Options.Num() > 0; }

	UFUNCTION(Category = "KzUI|Navigation", BlueprintPure)
	UWidget* GetHoveredOption() const { return HoveredOption; }

	UFUNCTION(Category = "KzUI|Navigation", BlueprintPure)
	UWidget* GetSelectedOption() const { return SelectedOption; }

	/** Moves the navigation hover to the given option. */
	UFUNCTION(Category = "KzUI|Navigation", BlueprintCallable)
	virtual void HoverWidget(UWidget* Widget);

	/** Commits the given option as the selection, deselecting the previous one. */
	UFUNCTION(Category = "KzUI|Navigation", BlueprintCallable)
	virtual void SelectOption(UWidget* Widget);

	UFUNCTION(Category = "KzUI|Navigation", BlueprintCallable)
	void ClearSelection();

	/** The hovered option was accepted. OptionName is the widget's designer name, handy for a Switch on Name. */
	UFUNCTION(Category = "KzUI|Navigation", BlueprintImplementableEvent, meta = (DisplayName = "On Option Accepted"))
	void ReceiveOnOptionAccepted(UWidget* Option, FName OptionName);

	/** A supported direction had nowhere to move. Only fires when the navigation does not loop. */
	UFUNCTION(Category = "KzUI|Navigation", BlueprintImplementableEvent, meta = (DisplayName = "On Navigation Edge"))
	void ReceiveOnNavigationEdge(EKzUIInputType Input);

	/** Defaults to every widget of the tree implementing the selectable interface with IsSelectable on, in tree order. */
	UFUNCTION(Category = "KzUI|Navigation", BlueprintNativeEvent, BlueprintCosmetic, BlueprintCallable)
	TArray<UWidget*> GetOptions() const;
	virtual TArray<UWidget*> GetOptions_Implementation() const;

	UFUNCTION(Category = "KzUI|Navigation", BlueprintNativeEvent, BlueprintCosmetic, BlueprintCallable)
	UWidget* GetDefaultOption() const;
	virtual UWidget* GetDefaultOption_Implementation() const { return nullptr; }

	UFUNCTION(Category = "KzUI|Navigation", BlueprintCallable)
	void OverrideOptions(TArray<UWidget*> NewOptions);

	/** Collects GetOptions again, keeping the hover where it is while that is still an option. Runs when SetIsSelectable changes a KzOptionWidget or KzTextBlock; call it after changing the options any other way. */
	UFUNCTION(Category = "KzUI|Navigation", BlueprintCallable)
	void RefreshOptions();

	/** Collects the selectable widgets under Root (inclusive), in tree order. Handy to scope a GetOptions override to a single container. */
	UFUNCTION(Category = "KzUI|Navigation", BlueprintPure)
	TArray<UWidget*> GetOptionsUnder(UWidget* Root) const;

	/** Reverts every option row to its construction value. */
	UFUNCTION(Category = "KzUI|Navigation", BlueprintCallable)
	void RevertOptions();

	/** Whether any option row changed since its construction. */
	UFUNCTION(Category = "KzUI|Navigation", BlueprintCallable)
	bool HasChangedOptions() const;

protected:
	void PlayHoverSound();

	void PlaySelectSound();

private:
	/** OverrideOptions, optionally keeping the current hover when it is still an option. */
	void SetOptions(TArray<UWidget*> NewOptions, bool bKeepHover);

	void HandleNavigationInput(EKzUIInputType Input, bool Handled);

	bool CanHandleNavigationInput(EKzUIInputType Input);

	bool HoveredOptionWantsInput(EKzUIInputType Input) const;

	UWidget* FindGeometricTarget(UWidget* Source, EKzUIInputType Input) const;

	UWidget* FindOrdinalTarget(UWidget* Source, bool bForward) const;

	UButton* GetButton(UWidget* Widget);

	UFUNCTION()
	void OnButtonClicked();

	/** Whether the mouse may drive the hover: the player is on keyboard and mouse, so a still cursor under a pad player's menu changes nothing. */
	bool IsUsingMouse() const;

	UWidget* GetOptionUnderCursor(const FVector2D& ScreenPosition) const;

	/** Hovers the option and triggers Accept through the same gates as a key press. */
	void AcceptWithMouse(UWidget* Option);

	/** Option under the left button when it went down: only a release over the same option accepts. */
	TWeakObjectPtr<UWidget> MousePressedOption;

	/** Buttons of the options, bound once per OverrideOptions to accept their own clicks. */
	TArray<TWeakObjectPtr<UButton>> BoundButtons;
};
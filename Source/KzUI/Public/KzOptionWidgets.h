// Copyright 2026 kirzo

#pragma once

#include "Blueprint/UserWidget.h"
#include "KzSelectableWidgetInterface.h"
#include "KzSettingsLibrary.h"
#include "Styling/SlateBrush.h"
#include "KzOptionWidgets.generated.h"

class UImage;
class UProgressBar;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FKzSpinnerValueChangedSignature, int32, Index, FText, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKzSliderValueChangedSignature, float, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKzToggleChangedSignature, bool, bChecked);

/**
 * Base for interactive option rows of a navigable menu: selectable, propagates hover and
 * selection to its inner selectable widgets, and can claim inputs while hovered.
 */
UCLASS(Abstract, meta = (PrioritizeCategories = "KzUI"))
class KZUI_API UKzOptionWidget : public UUserWidget, public IKzSelectableWidgetInterface
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = "KzUI|Option", EditAnywhere)
	bool bSelectable = true;

	/** With a bound Setting: apply and save on every change. Off, changes are only staged until UKzSettingsLibrary::ApplySettings (confirm-on-exit flows); discard them by reverting the rows. */
	UPROPERTY(Category = "KzUI|Option", EditAnywhere)
	bool bApplyImmediately = true;

	/** Restores the value captured at construction, notifying through the usual change events. */
	UFUNCTION(Category = "KzUI|Option", BlueprintCallable, BlueprintNativeEvent)
	void Revert();
	virtual void Revert_Implementation() {}

	/** Whether the value differs from the one captured at construction. */
	UFUNCTION(Category = "KzUI|Option", BlueprintCallable, BlueprintNativeEvent)
	bool HasChanges() const;
	virtual bool HasChanges_Implementation() const { return false; }

	/** Re-reads the bound value and re-baselines the change snapshot. Runs on construct and every time the owning screen becomes active. */
	UFUNCTION(Category = "KzUI|Option", BlueprintCallable, BlueprintNativeEvent)
	void Resync();
	virtual void Resync_Implementation() {}

	//~ Begin IKzSelectableWidgetInterface
	virtual bool IsSelectable_Implementation() const override { return bSelectable; }
	virtual void SetIsSelectable_Implementation(bool bNewSelectable) override { bSelectable = bNewSelectable; }
	virtual bool CanBeSelected_Implementation() const override { return false; }
	virtual TArray<UWidget*> GetLinkedSelectables_Implementation() const override;
	//~ End IKzSelectableWidgetInterface
};

/** Option row that cycles a list of values with Left/Right while hovered. Bind ValueText to display the current value. */
UCLASS(Abstract, meta = (PrioritizeCategories = "KzUI"))
class KZUI_API UKzSpinner : public UKzOptionWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = "KzUI|Spinner", EditAnywhere, BlueprintReadOnly)
	TArray<FText> Values;

	UPROPERTY(Category = "KzUI|Spinner", EditAnywhere, BlueprintReadOnly)
	int32 SelectedIndex = 0;

	UPROPERTY(Category = "KzUI|Spinner", EditAnywhere)
	bool bWrap = false;

	/** Index-shaped engine setting to auto-populate from and apply to (Resolution, WindowMode, Quality). */
	UPROPERTY(Category = "KzUI|Spinner", EditAnywhere)
	EKzUIStandardSetting Setting = EKzUIStandardSetting::None;

	/** Scalability group this row drives. */
	UPROPERTY(Category = "KzUI|Spinner", EditAnywhere, meta = (EditCondition = "Setting == EKzUIStandardSetting::Quality", EditConditionHides))
	EKzUIQualitySetting QualitySetting = EKzUIQualitySetting::Overall;

	/** Shown while no value is selected, e.g. mixed quality levels. The next step lands on a real value. */
	UPROPERTY(Category = "KzUI|Spinner", EditAnywhere)
	FText CustomText;

	UPROPERTY(Category = "KzUI|Events", BlueprintAssignable)
	FKzSpinnerValueChangedSignature OnValueChanged;

protected:
	/** Displays the current value. */
	UPROPERTY(Category = "KzUI", BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ValueText;

	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;

	//~ Begin IKzSelectableWidgetInterface
	virtual bool WantsInput_Implementation(EKzUIInputType Input) const override;
	virtual bool HandleInput_Implementation(EKzUIInputType Input) override;
	//~ End IKzSelectableWidgetInterface

public:
	UKzSpinner(const FObjectInitializer& ObjectInitializer);

	virtual void Revert_Implementation() override { SetSelectedIndex(InitialIndex); }
	virtual bool HasChanges_Implementation() const override { return SelectedIndex != InitialIndex; }
	virtual void Resync_Implementation() override;

	/** An invalid NewIndex leaves the spinner unset, displaying CustomText until the first step. */
	UFUNCTION(Category = "KzUI|Spinner", BlueprintCallable)
	void SetValues(const TArray<FText>& NewValues, int32 NewIndex = 0);

	UFUNCTION(Category = "KzUI|Spinner", BlueprintCallable)
	void SetSelectedIndex(int32 NewIndex);

	UFUNCTION(Category = "KzUI|Spinner", BlueprintPure)
	FText GetSelectedValue() const;

	UFUNCTION(Category = "KzUI|Spinner", BlueprintCallable)
	void Next();

	UFUNCTION(Category = "KzUI|Spinner", BlueprintCallable)
	void Previous();

	UFUNCTION(Category = "KzUI|Spinner", BlueprintImplementableEvent, meta = (DisplayName = "On Value Changed"))
	void ReceiveOnValueChanged(int32 Index, const FText& Value);

	/** A step changed the value in the given direction. Handy for arrow feedback animations. */
	UFUNCTION(Category = "KzUI|Spinner", BlueprintImplementableEvent, meta = (DisplayName = "On Stepped"))
	void ReceiveOnStepped(bool bForward);

private:
	bool Step(int32 Delta);

	void RefreshValueText();

	int32 InitialIndex = 0;
};

/** Option row that adjusts a normalized value with Left/Right while hovered. Bind Bar and/or ValueText to display it. */
UCLASS(Abstract, meta = (PrioritizeCategories = "KzUI"))
class KZUI_API UKzSlider : public UKzOptionWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = "KzUI|Slider", EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0, ClampMax = 1))
	float Value = 1.0f;

	UPROPERTY(Category = "KzUI|Slider", EditAnywhere, meta = (ClampMin = 0.01, ClampMax = 1))
	float Step = 0.1f;

	/** Value-shaped engine setting to auto-populate from and apply to (ResolutionScale). */
	UPROPERTY(Category = "KzUI|Slider", EditAnywhere)
	EKzUIStandardSetting Setting = EKzUIStandardSetting::None;

	UPROPERTY(Category = "KzUI|Events", BlueprintAssignable)
	FKzSliderValueChangedSignature OnValueChanged;

protected:
	/** Displays the value as a filled bar. */
	UPROPERTY(Category = "KzUI", BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> Bar;

	/** Displays the value as a percentage. */
	UPROPERTY(Category = "KzUI", BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ValueText;

	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;

	//~ Begin IKzSelectableWidgetInterface
	virtual bool WantsInput_Implementation(EKzUIInputType Input) const override;
	virtual bool HandleInput_Implementation(EKzUIInputType Input) override;
	//~ End IKzSelectableWidgetInterface

public:
	virtual void Revert_Implementation() override { SetValue(InitialValue); }
	virtual bool HasChanges_Implementation() const override { return !FMath::IsNearlyEqual(Value, InitialValue); }
	virtual void Resync_Implementation() override;

	UFUNCTION(Category = "KzUI|Slider", BlueprintCallable)
	void SetValue(float NewValue);

	UFUNCTION(Category = "KzUI|Slider", BlueprintImplementableEvent, meta = (DisplayName = "On Value Changed"))
	void ReceiveOnValueChanged(float NewValue);

private:
	void RefreshValueVisuals();

	float InitialValue = 1.0f;
};

/** Option row that flips a boolean with Accept while hovered. For Left/Right adjustment use a two-option UKzSpinner instead. Bind ValueText and/or ValueImage to display the state. */
UCLASS(Abstract, meta = (PrioritizeCategories = "KzUI"))
class KZUI_API UKzToggle : public UKzOptionWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = "KzUI|Toggle", EditAnywhere, BlueprintReadOnly)
	bool bChecked = false;

	UPROPERTY(Category = "KzUI|Toggle", EditAnywhere)
	FText CheckedText;

	UPROPERTY(Category = "KzUI|Toggle", EditAnywhere)
	FText UncheckedText;

	UPROPERTY(Category = "KzUI|Toggle", EditAnywhere)
	FSlateBrush CheckedBrush;

	UPROPERTY(Category = "KzUI|Toggle", EditAnywhere)
	FSlateBrush UncheckedBrush;

	/** Bool-shaped engine setting to auto-populate from and apply to (VSync). */
	UPROPERTY(Category = "KzUI|Toggle", EditAnywhere)
	EKzUIStandardSetting Setting = EKzUIStandardSetting::None;

	UPROPERTY(Category = "KzUI|Events", BlueprintAssignable)
	FKzToggleChangedSignature OnCheckedChanged;

protected:
	/** Displays the current state as text. */
	UPROPERTY(Category = "KzUI", BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ValueText;

	/** Displays the current state as an image. */
	UPROPERTY(Category = "KzUI", BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> ValueImage;

	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;

	//~ Begin IKzSelectableWidgetInterface
	virtual bool WantsInput_Implementation(EKzUIInputType Input) const override;
	virtual bool HandleInput_Implementation(EKzUIInputType Input) override;
	//~ End IKzSelectableWidgetInterface

public:
	UKzToggle(const FObjectInitializer& ObjectInitializer);

	virtual void Revert_Implementation() override { SetChecked(bInitialChecked); }
	virtual bool HasChanges_Implementation() const override { return bChecked != bInitialChecked; }
	virtual void Resync_Implementation() override;

	UFUNCTION(Category = "KzUI|Toggle", BlueprintCallable)
	void SetChecked(bool bNewChecked);

	UFUNCTION(Category = "KzUI|Toggle", BlueprintImplementableEvent, meta = (DisplayName = "On Checked Changed"))
	void ReceiveOnCheckedChanged(bool bNewChecked);

private:
	void RefreshValueVisuals();

	bool bInitialChecked = false;
};
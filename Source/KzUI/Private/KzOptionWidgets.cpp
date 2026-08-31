// Copyright 2026 kirzo

#include "KzOptionWidgets.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

#define LOCTEXT_NAMESPACE "KzUI"

DEFINE_LOG_CATEGORY_STATIC(LogKzUI, Log, All);

TArray<UWidget*> UKzOptionWidget::GetLinkedSelectables_Implementation() const
{
	// Hover and selection reach the inner texts so their styles react with the row
	TArray<UWidget*> Result;
	if (WidgetTree)
	{
		WidgetTree->ForEachWidget([&Result](UWidget* Widget)
			{
				if (Widget->Implements<UKzSelectableWidgetInterface>())
				{
					Result.Add(Widget);
				}
			}
		);
	}
	return Result;
}

UKzSpinner::UKzSpinner(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CustomText = LOCTEXT("SpinnerCustom", "Custom");
}

void UKzSpinner::NativePreConstruct()
{
	Super::NativePreConstruct();
	RefreshValueText();
}

void UKzSpinner::NativeConstruct()
{
	Resync();
	Super::NativeConstruct();
}

void UKzSpinner::Resync_Implementation()
{
	if (Setting != EKzUIStandardSetting::None)
	{
		SetValues(UKzSettingsLibrary::GetSettingOptions(Setting, QualitySetting), UKzSettingsLibrary::GetSettingIndex(Setting, QualitySetting));
	}
	InitialIndex = SelectedIndex;
}

bool UKzSpinner::WantsInput_Implementation(EKzUIInputType Input) const
{
	return Input == EKzUIInputType::Left || Input == EKzUIInputType::Right;
}

bool UKzSpinner::HandleInput_Implementation(EKzUIInputType Input)
{
	return Step(Input == EKzUIInputType::Left ? -1 : 1);
}

void UKzSpinner::SetValues(const TArray<FText>& NewValues, int32 NewIndex)
{
	Values = NewValues;
	SelectedIndex = Values.IsValidIndex(NewIndex) ? NewIndex : INDEX_NONE;
	RefreshValueText();
}

void UKzSpinner::SetSelectedIndex(int32 NewIndex)
{
	if (!Values.IsValidIndex(NewIndex) || NewIndex == SelectedIndex)
	{
		return;
	}

	SelectedIndex = NewIndex;
	RefreshValueText();

	if (Setting != EKzUIStandardSetting::None)
	{
		UKzSettingsLibrary::SetSettingIndex(Setting, SelectedIndex, bApplyImmediately, QualitySetting);
	}
	OnValueChanged.Broadcast(SelectedIndex, GetSelectedValue());
	ReceiveOnValueChanged(SelectedIndex, GetSelectedValue());
}

FText UKzSpinner::GetSelectedValue() const
{
	return Values.IsValidIndex(SelectedIndex) ? Values[SelectedIndex] : FText::GetEmpty();
}

void UKzSpinner::Next()
{
	Step(1);
}

void UKzSpinner::Previous()
{
	Step(-1);
}

bool UKzSpinner::Step(int32 Delta)
{
	const int32 Count = Values.Num();
	if (Count == 0)
	{
		return false;
	}

	int32 NewIndex = SelectedIndex + Delta;
	if (bWrap)
	{
		NewIndex = (NewIndex % Count + Count) % Count;
	}

	const int32 Previous = SelectedIndex;
	SetSelectedIndex(FMath::Clamp(NewIndex, 0, Count - 1));
	if (SelectedIndex == Previous)
	{
		return false;
	}

	ReceiveOnStepped(Delta > 0);
	return true;
}

void UKzSpinner::RefreshValueText()
{
	if (ValueText)
	{
		ValueText->SetText(Values.IsValidIndex(SelectedIndex) ? Values[SelectedIndex] : (Values.Num() > 0 ? CustomText : FText::GetEmpty()));
	}
}

void UKzSlider::NativePreConstruct()
{
	Super::NativePreConstruct();
	RefreshValueVisuals();
}

void UKzSlider::NativeConstruct()
{
	Resync();
	Super::NativeConstruct();
}

void UKzSlider::Resync_Implementation()
{
	if (Setting != EKzUIStandardSetting::None)
	{
		Value = UKzSettingsLibrary::GetSettingValue(Setting);
		RefreshValueVisuals();
	}
	InitialValue = Value;
}

bool UKzSlider::WantsInput_Implementation(EKzUIInputType Input) const
{
	return Input == EKzUIInputType::Left || Input == EKzUIInputType::Right;
}

bool UKzSlider::HandleInput_Implementation(EKzUIInputType Input)
{
	const float Previous = Value;
	SetValue(Value + (Input == EKzUIInputType::Left ? -Step : Step));
	return !FMath::IsNearlyEqual(Previous, Value);
}

void UKzSlider::SetValue(float NewValue)
{
	NewValue = FMath::Clamp(NewValue, 0.0f, 1.0f);
	if (FMath::IsNearlyEqual(NewValue, Value))
	{
		return;
	}

	Value = NewValue;
	RefreshValueVisuals();

	if (Setting != EKzUIStandardSetting::None)
	{
		UKzSettingsLibrary::SetSettingValue(Setting, Value, bApplyImmediately);
	}
	OnValueChanged.Broadcast(Value);
	ReceiveOnValueChanged(Value);
}

void UKzSlider::RefreshValueVisuals()
{
	if (Bar)
	{
		Bar->SetPercent(Value);
	}
	if (ValueText)
	{
		ValueText->SetText(FText::AsPercent(Value));
	}
}

UKzToggle::UKzToggle(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CheckedText = LOCTEXT("ToggleOn", "On");
	UncheckedText = LOCTEXT("ToggleOff", "Off");
}

void UKzToggle::NativePreConstruct()
{
	Super::NativePreConstruct();
	RefreshValueVisuals();
}

void UKzToggle::NativeConstruct()
{
	Resync();
	Super::NativeConstruct();
}

void UKzToggle::Resync_Implementation()
{
	if (Setting != EKzUIStandardSetting::None)
	{
		bChecked = UKzSettingsLibrary::GetSettingEnabled(Setting);
		RefreshValueVisuals();
	}
	bInitialChecked = bChecked;
}

bool UKzToggle::WantsInput_Implementation(EKzUIInputType Input) const
{
	return Input == EKzUIInputType::Accept;
}

bool UKzToggle::HandleInput_Implementation(EKzUIInputType Input)
{
	SetChecked(!bChecked);
	return true;
}

void UKzToggle::SetChecked(bool bNewChecked)
{
	if (bNewChecked == bChecked)
	{
		return;
	}

	bChecked = bNewChecked;
	RefreshValueVisuals();

	if (Setting != EKzUIStandardSetting::None)
	{
		UKzSettingsLibrary::SetSettingEnabled(Setting, bChecked, bApplyImmediately);
	}
	OnCheckedChanged.Broadcast(bChecked);
	ReceiveOnCheckedChanged(bChecked);
}

void UKzToggle::RefreshValueVisuals()
{
	UE_LOG(LogKzUI, Verbose, TEXT("[%s] Toggle refresh: bChecked=%d Checked=%s Unchecked=%s ValueImage=%s"), *GetName(), bChecked, *GetNameSafe(CheckedBrush.GetResourceObject()), *GetNameSafe(UncheckedBrush.GetResourceObject()), *GetNameSafe(ValueImage));

	if (ValueText)
	{
		ValueText->SetText(bChecked ? CheckedText : UncheckedText);
	}
	if (ValueImage)
	{
		ValueImage->SetBrush(bChecked ? CheckedBrush : UncheckedBrush);
	}
}

#undef LOCTEXT_NAMESPACE
// Copyright 2026 kirzo

#include "KzNavigableWidget.h"
#include "KzOptionWidgets.h"
#include "KzSelectableWidgetInterface.h"
#include "KzUISoundTheme.h"

#include "Blueprint/WidgetNavigation.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

static bool IsWidgetVisible(UWidget* Widget)
{
	// A widget also counts as hidden when any ancestor is hidden or collapsed
	for (UWidget* Current = Widget; Current; Current = Current->GetParent())
	{
		if (!Current->IsVisible())
		{
			return false;
		}
	}
	return true;
}

static bool IsWidgetSelectable(UWidget* Widget)
{
	return Widget->GetIsEnabled() && IsWidgetVisible(Widget) && (!Widget->Implements<UKzSelectableWidgetInterface>() || IKzSelectableWidgetInterface::Execute_IsSelectable(Widget));
}

static bool CanSelectOption(UWidget* Widget)
{
	return IsValid(Widget) && (!Widget->Implements<UKzSelectableWidgetInterface>() || IKzSelectableWidgetInterface::Execute_CanBeSelected(Widget));
}

static const FWidgetNavigationData* GetNavigationData(UWidget* Source, EKzUIInputType Input)
{
	if (!IsValid(Source) || !Source->Navigation)
	{
		return nullptr;
	}

	switch (Input)
	{
		case EKzUIInputType::Down:		return &Source->Navigation->Down;
		case EKzUIInputType::Up:		return &Source->Navigation->Up;
		case EKzUIInputType::Left:		return &Source->Navigation->Left;
		case EKzUIInputType::Right:		return &Source->Navigation->Right;
		case EKzUIInputType::Previous:	return &Source->Navigation->Previous;
		case EKzUIInputType::Next:		return &Source->Navigation->Next;
		default: return nullptr;
	}
}

static UWidget* GetNavigationTarget(UWidget* Source, EKzUIInputType Input)
{
	const FWidgetNavigationData* Data = GetNavigationData(Source, Input);
	return Data && Data->Widget.IsValid() ? Data->Widget.Get() : nullptr;
}

static bool IsNavigationStopped(UWidget* Source, EKzUIInputType Input)
{
	const FWidgetNavigationData* Data = GetNavigationData(Source, Input);
	return Data && Data->Rule == EUINavigationRule::Stop;
}

UKzNavigableWidget::UKzNavigableWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetInputSupport(EKzUIInputType::Accept, true);
	SetInputSupport(EKzUIInputType::Down, true);
	SetInputSupport(EKzUIInputType::Up, true);
	SetInputSupport(EKzUIInputType::Left, true);
	SetInputSupport(EKzUIInputType::Right, true);
	SetInputSupport(EKzUIInputType::Previous, true);
	SetInputSupport(EKzUIInputType::Next, true);
}

//~ Begin UUserWidget Interface
void UKzNavigableWidget::NativeConstruct()
{
	OverrideOptions(GetOptions());

	Super::NativeConstruct();
}

void UKzNavigableWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	for (UWidget* Option : Options)
	{
		if (Option->GetCachedGeometry().IsUnderLocation(InMouseEvent.GetScreenSpacePosition()))
		{
			HoverWidget(Option);
			return;
		}
	}
}

FReply UKzNavigableWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (HoveredOption && HoveredOption->GetCachedGeometry().IsUnderLocation(InMouseEvent.GetScreenSpacePosition()))
	{
		PlayInputSound(EKzUIInputType::Accept);
		AcceptInputTriggered(true);
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}
//~ End UUserWidget Interface

//~ Begin UKzUserWidget Interface
bool UKzNavigableWidget::CanHandleAcceptInput_Implementation()
{
	return HoveredOption && (HoveredOptionWantsInput(EKzUIInputType::Accept) || CanSelectOption(HoveredOption));
}

bool UKzNavigableWidget::CanHandleNavigationInput(EKzUIInputType Input)
{
	if (HoveredOptionWantsInput(Input))
	{
		return true;
	}
	UWidget* Target = GetTargetWidget(HoveredOption, Input);
	return Target && Target != this;
}

bool UKzNavigableWidget::CanHandleDownInput_Implementation()
{
	return CanHandleNavigationInput(EKzUIInputType::Down);
}

bool UKzNavigableWidget::CanHandleUpInput_Implementation()
{
	return CanHandleNavigationInput(EKzUIInputType::Up);
}

bool UKzNavigableWidget::CanHandleLeftInput_Implementation()
{
	return CanHandleNavigationInput(EKzUIInputType::Left);
}

bool UKzNavigableWidget::CanHandleRightInput_Implementation()
{
	return CanHandleNavigationInput(EKzUIInputType::Right);
}

bool UKzNavigableWidget::CanHandlePreviousInput_Implementation()
{
	return CanHandleNavigationInput(EKzUIInputType::Previous);
}

bool UKzNavigableWidget::CanHandleNextInput_Implementation()
{
	return CanHandleNavigationInput(EKzUIInputType::Next);
}

void UKzNavigableWidget::AcceptInputTriggered_Implementation(bool Handled)
{
	if (Handled && HoveredOption)
	{
		// The hovered option can claim Accept for itself (e.g. a toggle)
		if (HoveredOptionWantsInput(EKzUIInputType::Accept))
		{
			if (IKzSelectableWidgetInterface::Execute_HandleInput(HoveredOption, EKzUIInputType::Accept))
			{
				PlaySelectSound();
			}
			return;
		}

		// Otherwise Accept commits the hovered option as the selection, unless it is hover-only
		if (CanSelectOption(HoveredOption))
		{
			// Re-accepting the current selection changes nothing but still deserves its feedback
			if (SelectedOption == HoveredOption)
			{
				PlaySelectSound();
			}
			SelectOption(HoveredOption);
			ReceiveOnOptionAccepted(HoveredOption, HoveredOption->GetFName());
		}
	}
}

void UKzNavigableWidget::HandleNavigationInput(EKzUIInputType Input, bool Handled)
{
	if (Handled)
	{
		// The hovered option can claim the input for itself (e.g. Left/Right on a spinner)
		if (HoveredOptionWantsInput(Input))
		{
			if (IKzSelectableWidgetInterface::Execute_HandleInput(HoveredOption, Input))
			{
				PlayHoverSound();
			}
			return;
		}
		HoverWidget(GetTargetWidget(HoveredOption, Input));
	}
	else if (HoveredOption && !GetTargetWidget(HoveredOption, Input))
	{
		ReceiveOnNavigationEdge(Input);
	}
}

bool UKzNavigableWidget::HoveredOptionWantsInput(EKzUIInputType Input) const
{
	return HoveredOption && IsWidgetSelectable(HoveredOption) && HoveredOption->Implements<UKzSelectableWidgetInterface>() && IKzSelectableWidgetInterface::Execute_WantsInput(HoveredOption, Input);
}

void UKzNavigableWidget::DownInputTriggered_Implementation(bool Handled)
{
	HandleNavigationInput(EKzUIInputType::Down, Handled);
}

void UKzNavigableWidget::UpInputTriggered_Implementation(bool Handled)
{
	HandleNavigationInput(EKzUIInputType::Up, Handled);
}

void UKzNavigableWidget::LeftInputTriggered_Implementation(bool Handled)
{
	HandleNavigationInput(EKzUIInputType::Left, Handled);
}

void UKzNavigableWidget::RightInputTriggered_Implementation(bool Handled)
{
	HandleNavigationInput(EKzUIInputType::Right, Handled);
}

void UKzNavigableWidget::PreviousInputTriggered_Implementation(bool Handled)
{
	HandleNavigationInput(EKzUIInputType::Previous, Handled);
}

void UKzNavigableWidget::NextInputTriggered_Implementation(bool Handled)
{
	HandleNavigationInput(EKzUIInputType::Next, Handled);
}
//~ End UKzUserWidget Interface

void UKzNavigableWidget::PlayInputSound(EKzUIInputType Input)
{
	switch (Input)
	{
		// Directional inputs sound as hover movement, played by HoverWidget
		case EKzUIInputType::Down:
		case EKzUIInputType::Up:
		case EKzUIInputType::Left:
		case EKzUIInputType::Right:
		case EKzUIInputType::Previous:
		case EKzUIInputType::Next:
			return;
		default:
			Super::PlayInputSound(Input);
	}
}

void UKzNavigableWidget::PlayHoverSound()
{
	USoundBase* Sound = HoverSoundOverride;
	if (!Sound)
	{
		if (const UKzUISoundTheme* Theme = GetSoundTheme())
		{
			Sound = Theme->HoverSound;
		}
	}
	if (Sound)
	{
		UGameplayStatics::PlaySound2D(this, Sound);
	}
}

void UKzNavigableWidget::PlaySelectSound()
{
	USoundBase* Sound = SelectSoundOverride;
	if (!Sound)
	{
		if (const UKzUISoundTheme* Theme = GetSoundTheme())
		{
			Sound = Theme->SelectSound;
		}
	}
	if (Sound)
	{
		UGameplayStatics::PlaySound2D(this, Sound);
	}
}

TArray<UWidget*> UKzNavigableWidget::GetOptions_Implementation() const
{
	return GetOptionsUnder(WidgetTree ? WidgetTree->RootWidget : nullptr);
}

TArray<UWidget*> UKzNavigableWidget::GetOptionsUnder(UWidget* Root) const
{
	TArray<UWidget*> Result;
	if (Root)
	{
		auto AddOption = [&Result](UWidget* Widget)
			{
				if (Widget->Implements<UKzSelectableWidgetInterface>() && IKzSelectableWidgetInterface::Execute_IsSelectable(Widget))
				{
					Result.Add(Widget);
				}
			};
		AddOption(Root);
		UWidgetTree::ForWidgetAndChildren(Root, AddOption);
	}
	return Result;
}

void UKzNavigableWidget::OnActivated()
{
	// A switcher page keeps living while inactive: rows re-read their bound values on every activation
	for (UWidget* Option : Options)
	{
		if (UKzOptionWidget* Row = Cast<UKzOptionWidget>(Option))
		{
			Row->Resync();
		}
	}
	Super::OnActivated();
}

void UKzNavigableWidget::RevertOptions()
{
	for (UWidget* Option : Options)
	{
		if (UKzOptionWidget* Row = Cast<UKzOptionWidget>(Option))
		{
			Row->Revert();
		}
	}
}

bool UKzNavigableWidget::HasChangedOptions() const
{
	for (UWidget* Option : Options)
	{
		const UKzOptionWidget* Row = Cast<UKzOptionWidget>(Option);
		if (Row && Row->HasChanges())
		{
			return true;
		}
	}
	return false;
}

void UKzNavigableWidget::RefreshHover()
{
	for (UWidget* Option : Options)
	{
		IKzSelectableWidgetInterface::Hover(Option, HoveredOption == Option);
	}
}

UWidget* UKzNavigableWidget::GetTargetWidget(UWidget* Source, EKzUIInputType Input)
{
	if (!IsValid(Source))
	{
		return nullptr;
	}

	// A designer Stop rule is a hard wall
	if (IsNavigationStopped(Source, Input))
	{
		return nullptr;
	}

	// Explicit designer rules win, following chains through non-selectable widgets (Visited guards cycles)
	TSet<UWidget*> Visited;
	UWidget* Current = Source;
	while (Current)
	{
		Visited.Add(Current);
		UWidget* Target = GetNavigationTarget(Current, Input);
		if (!Target || Visited.Contains(Target))
		{
			break;
		}
		if (IsWidgetSelectable(Target))
		{
			return Target;
		}
		Current = Target;
	}

	if (Input == EKzUIInputType::Previous || Input == EKzUIInputType::Next)
	{
		return bPreviousNextNavigation ? FindOrdinalTarget(Source, Input == EKzUIInputType::Next) : nullptr;
	}
	return FindGeometricTarget(Source, Input);
}

UWidget* UKzNavigableWidget::FindGeometricTarget(UWidget* Source, EKzUIInputType Input) const
{
	FVector2D Direction;
	switch (Input)
	{
		case EKzUIInputType::Down:	Direction = FVector2D(0.0f, 1.0f); break;
		case EKzUIInputType::Up:	Direction = FVector2D(0.0f, -1.0f); break;
		case EKzUIInputType::Left:	Direction = FVector2D(-1.0f, 0.0f); break;
		case EKzUIInputType::Right:	Direction = FVector2D(1.0f, 0.0f); break;
		default: return nullptr;
	}

	const FGeometry& SourceGeometry = Source->GetCachedGeometry();
	const FVector2D Center = SourceGeometry.GetAbsolutePosition() + SourceGeometry.GetAbsoluteSize() * 0.5f;

	// Nearest option ahead, penalizing perpendicular drift. With loop enabled, the farthest
	// option behind (same scoring, negative axis) is the wrap-around target.
	UWidget* BestAhead = nullptr;
	UWidget* BestBehind = nullptr;
	float BestAheadScore = UE_MAX_FLT;
	float BestBehindScore = UE_MAX_FLT;

	for (UWidget* Option : Options)
	{
		if (Option == Source || !IsWidgetSelectable(Option))
		{
			continue;
		}

		const FGeometry& Geometry = Option->GetCachedGeometry();
		const FVector2D Delta = Geometry.GetAbsolutePosition() + Geometry.GetAbsoluteSize() * 0.5f - Center;
		const float Along = FVector2D::DotProduct(Delta, Direction);
		const float Score = Along + FMath::Abs(FVector2D::CrossProduct(Delta, Direction)) * 2.0f;

		if (Along > UE_KINDA_SMALL_NUMBER && Score < BestAheadScore)
		{
			BestAhead = Option;
			BestAheadScore = Score;
		}
		else if (Along < -UE_KINDA_SMALL_NUMBER && bLoopNavigation && Score < BestBehindScore)
		{
			BestBehind = Option;
			BestBehindScore = Score;
		}
	}
	return BestAhead ? BestAhead : BestBehind;
}

UWidget* UKzNavigableWidget::FindOrdinalTarget(UWidget* Source, bool bForward) const
{
	const int32 Count = Options.Num();
	const int32 Start = Options.Find(Source);
	if (Start == INDEX_NONE)
	{
		return nullptr;
	}

	const int32 Step = bForward ? 1 : -1;
	for (int32 Offset = 1; Offset < Count; Offset++)
	{
		int32 Index = Start + Offset * Step;
		if (bLoopNavigation)
		{
			Index = (Index % Count + Count) % Count;
		}
		else if (Index < 0 || Index >= Count)
		{
			break;
		}

		if (IsWidgetSelectable(Options[Index]))
		{
			return Options[Index];
		}
	}
	return nullptr;
}

void UKzNavigableWidget::HoverWidget(UWidget* Widget)
{
	if (!Options.Contains(Widget) || HoveredOption == Widget || !IsWidgetSelectable(Widget))
	{
		return;
	}

	if (HoveredOption)
	{
		IKzSelectableWidgetInterface::Hover(HoveredOption, false);

		if (UButton* AsButton = GetButton(HoveredOption))
		{
			AsButton->OnClicked.RemoveDynamic(this, &UKzNavigableWidget::OnButtonClicked);
		}
	}

	HoveredOption = Widget;
	PlayHoverSound();

	IKzSelectableWidgetInterface::Hover(HoveredOption, true);

	if (UButton* AsButton = GetButton(HoveredOption))
	{
		AsButton->OnClicked.AddDynamic(this, &UKzNavigableWidget::OnButtonClicked);
	}
}

void UKzNavigableWidget::SelectOption(UWidget* Widget)
{
	if (SelectedOption == Widget || (Widget && !Options.Contains(Widget)))
	{
		return;
	}

	if (SelectedOption)
	{
		IKzSelectableWidgetInterface::Select(SelectedOption, false);
	}

	SelectedOption = Widget;

	if (SelectedOption)
	{
		IKzSelectableWidgetInterface::Select(SelectedOption, true);
		PlaySelectSound();
	}
}

void UKzNavigableWidget::ClearSelection()
{
	SelectOption(nullptr);
}

void UKzNavigableWidget::OverrideOptions(TArray<UWidget*> NewOptions)
{
	Options.Reset();
	for (UWidget* Option : NewOptions)
	{
		if (IsValid(Option))
		{
			Options.Add(Option);
		}
	}

	HoveredOption = GetDefaultOption();
	if (!Options.Contains(HoveredOption))
	{
		HoveredOption = nullptr;
		for (UWidget* Option : Options)
		{
			if (IsWidgetSelectable(Option))
			{
				HoveredOption = Option;
				break;
			}
		}
	}
	if (!Options.Contains(SelectedOption))
	{
		SelectedOption = nullptr;
	}
	RefreshHover();
}

UButton* UKzNavigableWidget::GetButton(UWidget* Widget)
{
	UButton* AsButton = Cast<UButton>(Widget);
	if (!AsButton)
	{
		if (UUserWidget* AsUserWidget = Cast<UUserWidget>(Widget))
		{
			AsUserWidget->WidgetTree->ForEachWidget([&AsButton](UWidget* Child)
				{
					if (!AsButton)
					{
						AsButton = Cast<UButton>(Child);
					}
				}
			);
		}
	}
	return AsButton;
}

void UKzNavigableWidget::OnButtonClicked()
{
	PlayInputSound(EKzUIInputType::Accept);
	AcceptInputTriggered(true);
}
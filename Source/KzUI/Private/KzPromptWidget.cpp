// Copyright 2026 kirzo

#include "KzPromptWidget.h"
#include "KzSelectableWidgetInterface.h"

#include "Components/TextBlock.h"

UKzPromptWidget::UKzPromptWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bStartWithInputEnabled = true;
	SetInputSupport(EKzUIInputType::Back, true);
	SetInputSupport(EKzUIInputType::Accept, true);
}

//~ Begin UUserWidget Interface
void UKzPromptWidget::NativeConstruct()
{
	bAnswered = false;
	RemainingTime = AutoCancelTime;
	RefreshMessage();
	if (ConfirmText)
	{
		ConfirmText->SetText(ConfirmMessage);
	}
	if (CancelText)
	{
		CancelText->SetText(CancelMessage);
	}

	// The option texts are selectable by contract, whatever their own default says
	for (UTextBlock* Option : { ConfirmText.Get(), CancelText.Get() })
	{
		if (Option && Option->Implements<UKzSelectableWidgetInterface>())
		{
			IKzSelectableWidgetInterface::Execute_SetIsSelectable(Option, true);
		}
	}

	Super::NativeConstruct();
}

void UKzPromptWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (AutoCancelTime > 0.0f && !bAnswered)
	{
		const int32 SecondsBefore = FMath::CeilToInt(RemainingTime);
		RemainingTime -= InDeltaTime;
		if (RemainingTime <= 0.0f)
		{
			Cancel();
		}
		else if (FMath::CeilToInt(RemainingTime) != SecondsBefore)
		{
			RefreshMessage();
		}
	}
}
//~ End UUserWidget Interface

void UKzPromptWidget::RefreshMessage()
{
	MessageText->SetText(AutoCancelTime > 0.0f ? FText::FormatNamed(Message, TEXT("Seconds"), FMath::CeilToInt(RemainingTime)) : Message);
}

//~ Begin UKzUserWidget Interface
bool UKzPromptWidget::CanHandleAcceptInput_Implementation()
{
	return !ConfirmMessage.IsEmpty() || HasAnyOption();
}

bool UKzPromptWidget::CanHandleBackInput_Implementation()
{
	return !CancelMessage.IsEmpty() || HasAnyOption();
}

void UKzPromptWidget::AcceptInputTriggered_Implementation(bool Handled)
{
	if (Handled)
	{
		if (IsValid(HoveredOption) && HoveredOption == CancelText)
		{
			Cancel();
		}
		else
		{
			Confirm();
		}
	}
}

void UKzPromptWidget::BackInputTriggered_Implementation(bool Handled)
{
	if (Handled)
	{
		// With the confirm option highlighted, Back moves the selection to cancel instead of dismissing
		if (IsValid(HoveredOption) && HoveredOption == ConfirmText && Options.Contains(CancelText.Get()))
		{
			HoverWidget(CancelText);
		}
		else
		{
			Cancel();
		}
	}
}
//~ End UKzUserWidget Interface

//~ Begin UKzNavigableWidget Interface
TArray<UWidget*> UKzPromptWidget::GetOptions_Implementation() const
{
	// An option without a message is not on offer
	TArray<UWidget*> Result;
	if (bIsNavigable)
	{
		if (!ConfirmMessage.IsEmpty())
		{
			Result.Add(ConfirmText);
		}
		if (!CancelMessage.IsEmpty())
		{
			Result.Add(CancelText);
		}
	}
	return Result;
}

UWidget* UKzPromptWidget::GetDefaultOption_Implementation() const
{
	if (bDefaultToCancel && Options.Contains(CancelText.Get()))
	{
		return CancelText;
	}
	return HasAnyOption() ? Options[0].Get() : nullptr;
}
//~ End UKzNavigableWidget Interface

void UKzPromptWidget::PlayInputSound(EKzUIInputType Input)
{
	switch (Input)
	{
		// Accept and Back sound as their outcome: Confirm plays the select sound, Cancel the Back one
		case EKzUIInputType::Accept:
		case EKzUIInputType::Back:
			return;
		default:
			Super::PlayInputSound(Input);
	}
}

void UKzPromptWidget::Confirm()
{
	// Callable from Blueprint too: a prompt answers once
	if (bAnswered)
	{
		return;
	}
	bAnswered = true;

	SetInputEnabled(false);
	PlaySelectSound();
	if (bCloseOnResult)
	{
		RemoveFromStack(false);
	}
	ReceiveOnConfirm();
	OnResult.Delegate.ExecuteIfBound(true);
}

void UKzPromptWidget::Cancel()
{
	if (bAnswered)
	{
		return;
	}
	bAnswered = true;

	SetInputEnabled(false);
	Super::PlayInputSound(EKzUIInputType::Back);
	if (bCloseOnResult)
	{
		RemoveFromStack(false);
	}
	ReceiveOnCancel();
	OnResult.Delegate.ExecuteIfBound(false);
}
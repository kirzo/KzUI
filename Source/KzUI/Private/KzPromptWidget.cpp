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
	MessageText->SetText(Message);
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
//~ End UUserWidget Interface

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
	SetInputEnabled(false);
	PlaySelectSound();
	if (bCloseOnResult)
	{
		RemoveFromStack(false);
	}
	ReceiveOnConfirm();
	OnConfirm.Delegate.ExecuteIfBound();
}

void UKzPromptWidget::Cancel()
{
	SetInputEnabled(false);
	Super::PlayInputSound(EKzUIInputType::Back);
	if (bCloseOnResult)
	{
		RemoveFromStack(false);
	}
	ReceiveOnCancel();
	OnCancel.Delegate.ExecuteIfBound();
}
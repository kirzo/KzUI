// Copyright 2026 kirzo

#include "KzWidgetSwitcher.h"

#include "KzSwitcherPage.h"
#include "KzUIInputSubsystem.h"
#include "KzUserWidget.h"

#include "Containers/Ticker.h"

void UKzWidgetSwitcher::SetActiveWidgetIndex(int32 Index)
{
	const int32 Previous = GetActiveWidgetIndex();
	Super::SetActiveWidgetIndex(Index);
	if (GetActiveWidgetIndex() != Previous)
	{
		HandleActivePageChanged(Previous);
	}
}

void UKzWidgetSwitcher::SetActiveWidget(UWidget* Widget)
{
	const int32 Previous = GetActiveWidgetIndex();
	Super::SetActiveWidget(Widget);
	if (GetActiveWidgetIndex() != Previous)
	{
		HandleActivePageChanged(Previous);
	}
}

#if WITH_EDITOR
const FText UKzWidgetSwitcher::GetPaletteCategory()
{
	return INVTEXT("Kz UI");
}
#endif

void UKzWidgetSwitcher::OnWidgetRebuilt()
{
	Super::OnWidgetRebuilt();

	// Announce the initial page, deferred one frame so bindings made during Construct receive it
	if (!IsDesignTime())
	{
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float)
			{
				OnActivePageChanged.Broadcast(GetActiveWidgetIndex(), GetActiveWidget(), GetActivePageTitle());
				return false;
			}
		));
	}
}

void UKzWidgetSwitcher::GoBack()
{
	const int32 Target = PageHistory.Num() > 0 ? PageHistory.Pop() : 0;
	TGuardValue<bool> Guard(bNavigatingBack, true);
	SetActiveWidgetIndex(Target);
}

void UKzWidgetSwitcher::HandleActivePageChanged(int32 PreviousIndex)
{
	if (!bNavigatingBack)
	{
		PageHistory.Push(PreviousIndex);
	}

	if (IsDesignTime())
	{
		return;
	}

	UKzUIInputSubsystem* Subsystem = UKzUIInputSubsystem::Get(GetOwningLocalPlayer());

	if (UKzUserWidget* OldPage = GetPageScreen(GetWidgetAtIndex(PreviousIndex)))
	{
		OldPage->ClearInput();
		if (Subsystem)
		{
			Subsystem->UnregisterWidget(OldPage);
		}
	}

	UKzUserWidget* NewPage = GetPageScreen(GetActiveWidget());
	if (NewPage)
	{
		if (Subsystem)
		{
			Subsystem->RegisterWidget(NewPage);
		}
		NewPage->OnActivated();
	}

	// The owning screen stays quiet while a managed page owns the input: without this, inputs
	// the page does not handle would bubble into the screen's own navigation
	if (UKzUserWidget* Screen = GetOwningScreen())
	{
		if (NewPage && !bOwnerInputDisabledBySwitcher)
		{
			Screen->SetInputEnabled(false);
			bOwnerInputDisabledBySwitcher = true;
		}
		else if (!NewPage && bOwnerInputDisabledBySwitcher)
		{
			Screen->SetInputEnabled(true);
			bOwnerInputDisabledBySwitcher = false;
		}
	}

	OnActivePageChanged.Broadcast(GetActiveWidgetIndex(), GetActiveWidget(), GetActivePageTitle());
}

FText UKzWidgetSwitcher::GetActivePageTitle() const
{
	const UKzSwitcherPage* Page = Cast<UKzSwitcherPage>(GetActiveWidget());
	return Page ? Page->Title : FText::GetEmpty();
}

UKzUserWidget* UKzWidgetSwitcher::GetOwningScreen() const
{
	return GetTypedOuter<UKzUserWidget>();
}

UKzUserWidget* UKzWidgetSwitcher::GetPageScreen(UWidget* Page)
{
	if (UKzSwitcherPage* Wrapper = Cast<UKzSwitcherPage>(Page))
	{
		return Cast<UKzUserWidget>(Wrapper->GetContent());
	}
	return Cast<UKzUserWidget>(Page);
}
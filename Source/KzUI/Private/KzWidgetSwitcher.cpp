// Copyright 2026 kirzo

#include "KzWidgetSwitcher.h"

#include "KzSwitcherPage.h"
#include "KzUIInputSubsystem.h"
#include "KzUserWidget.h"

#include "Blueprint/WidgetTree.h"
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

	for (UKzUserWidget* OldPage : GetPageScreens(GetWidgetAtIndex(PreviousIndex)))
	{
		OldPage->ClearInput();
		if (Subsystem)
		{
			Subsystem->UnregisterWidget(OldPage);
		}
	}

	const TArray<UKzUserWidget*> NewPages = GetPageScreens(GetActiveWidget());
	for (UKzUserWidget* NewPage : NewPages)
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
		if (NewPages.Num() > 0 && !bOwnerInputDisabledBySwitcher)
		{
			// A screen whose input was already off is left alone, so leaving the page does not switch it on
			if (Screen->IsInputSwitchedOn())
			{
				Screen->SetInputEnabled(false);
				bOwnerInputDisabledBySwitcher = true;
			}
		}
		else if (NewPages.IsEmpty() && bOwnerInputDisabledBySwitcher)
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

UKzWidgetSwitcher* UKzWidgetSwitcher::FindOwningSwitcher(const UWidget* Widget, bool& bOutOnActivePage)
{
	bOutOnActivePage = true;
	const UWidget* Child = Widget;
	for (UWidget* Parent = Widget->GetParent(); Parent; Child = Parent, Parent = Parent->GetParent())
	{
		if (UKzWidgetSwitcher* Switcher = Cast<UKzWidgetSwitcher>(Parent))
		{
			bOutOnActivePage = Switcher->GetActiveWidget() == Child;
			return Switcher;
		}
	}
	return nullptr;
}

TArray<UKzUserWidget*> UKzWidgetSwitcher::GetPageScreens(UWidget* Page) const
{
	TArray<UKzUserWidget*> Screens;
	auto AddScreen = [this, &Screens](UWidget* Widget)
		{
			// The ones under a nested switcher belong to it
			bool bOnActivePage = true;
			UKzUserWidget* Screen = Cast<UKzUserWidget>(Widget);
			if (Screen && FindOwningSwitcher(Screen, bOnActivePage) == this)
			{
				Screens.Add(Screen);
			}
		};

	if (Page)
	{
		AddScreen(Page);
		UWidgetTree::ForWidgetAndChildren(Page, AddScreen);
	}
	return Screens;
}
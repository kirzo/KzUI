// Copyright 2026 kirzo

#include "KzHUD.h"
#include "KzUIInputSettings.h"

#include "Algo/AnyOf.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	bool HasLiveSource(const TSet<TWeakObjectPtr<UObject>>& Sources)
	{
		return Algo::AnyOf(Sources, [](const TWeakObjectPtr<UObject>& Source) { return Source.IsValid(); });
	}
}

AKzHUD* AKzHUD::Get(const UObject* WorldContextObject)
{
	const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(WorldContextObject, 0);
	return PlayerController ? PlayerController->GetHUD<AKzHUD>() : nullptr;
}

void AKzHUD::BeginPlay()
{
	Super::BeginPlay();

	for (const FKzHUDWidget& Entry : Widgets)
	{
		if (Entry.Class)
		{
			AddToLayer(CreateWidget<UUserWidget>(GetOwningPlayerController(), Entry.Class), Entry.Layer);
		}
	}
}

void AKzHUD::AddToLayer(UUserWidget* Widget, FGameplayTag Layer)
{
	if (!Widget)
	{
		return;
	}

	// A widget created without an owning player can't go on a player's screen
	if (!Widget->GetOwningPlayer())
	{
		Widget->SetOwningPlayer(GetOwningPlayerController());
	}
	Widget->AddToPlayerScreen(UKzUIInputSettings::Get()->GetLayerZOrder(Layer));

	LayerWidgets.Add({ Widget, Layer });
	RefreshLayers();
}

void AKzHUD::PushLayerHidden(FGameplayTag Layer, UObject* Source)
{
	HiddenLayers.FindOrAdd(Layer).Add(Source);
	RefreshLayers();
}

void AKzHUD::RemoveLayerHidden(FGameplayTag Layer, UObject* Source)
{
	if (TSet<TWeakObjectPtr<UObject>>* Sources = HiddenLayers.Find(Layer))
	{
		Sources->Remove(Source);
	}
	RefreshLayers();
}

bool AKzHUD::IsLayerHidden(FGameplayTag Layer) const
{
	for (const TPair<FGameplayTag, TSet<TWeakObjectPtr<UObject>>>& Hidden : HiddenLayers)
	{
		if (Layer.MatchesTag(Hidden.Key) && HasLiveSource(Hidden.Value))
		{
			return true;
		}
	}
	return false;
}

void AKzHUD::PushPause(UObject* Source)
{
	PauseSources.Add(Source);
	UGameplayStatics::SetGamePaused(this, true);
}

void AKzHUD::RemovePause(UObject* Source)
{
	PauseSources.Remove(Source);
	UGameplayStatics::SetGamePaused(this, HasLiveSource(PauseSources));
}

UUserWidget* AKzHUD::GetWidget(TSubclassOf<UUserWidget> Class) const
{
	for (const FLayerWidget& Entry : LayerWidgets)
	{
		UUserWidget* Widget = Entry.Widget.Get();
		if (Widget && Widget->IsInViewport() && Class && Widget->IsA(Class))
		{
			return Widget;
		}
	}
	return nullptr;
}

void AKzHUD::RefreshLayers()
{
	// A widget removed from the screen leaves its layer
	LayerWidgets.RemoveAll([](const FLayerWidget& Entry) { return !Entry.Widget.IsValid() || !Entry.Widget->IsInViewport(); });

	for (FLayerWidget& Entry : LayerWidgets)
	{
		UUserWidget* Widget = Entry.Widget.Get();
		const bool bLayerHidden = IsLayerHidden(Entry.Layer);
		if (bLayerHidden && !Entry.ShownVisibility.IsSet())
		{
			Entry.ShownVisibility = Widget->GetVisibility();
			Widget->SetVisibility(ESlateVisibility::Collapsed);
		}
		else if (!bLayerHidden && Entry.ShownVisibility.IsSet())
		{
			Widget->SetVisibility(Entry.ShownVisibility.GetValue());
			Entry.ShownVisibility.Reset();
		}
	}
}

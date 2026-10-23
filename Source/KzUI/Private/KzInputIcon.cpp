// Copyright 2026 kirzo

#include "KzInputIcon.h"
#include "KzUIIconTheme.h"
#include "KzUIInputSettings.h"
#include "KzUIInputSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

DEFINE_LOG_CATEGORY_STATIC(LogKzUI, Log, All);

void UKzInputIcon::SetInput(EKzUIInputType NewInput)
{
	Input = NewInput;
	RefreshIcon();
}

void UKzInputIcon::SetCustomToken(FName NewToken)
{
	CustomToken = NewToken;
	RefreshIcon();
}

void UKzInputIcon::SetVariation(FName NewVariation)
{
	Variation = NewVariation;
	RefreshIcon();
}

void UKzInputIcon::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	RefreshIcon();
}

void UKzInputIcon::OnWidgetRebuilt()
{
	Super::OnWidgetRebuilt();
	RefreshIcon();
}

UKzUIInputSubsystem* UKzInputIcon::BindPlayerSubsystem()
{
	// Widgets created before the player is known have no owner yet: they follow the first player, like the engine's own default
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	if (!LocalPlayer)
	{
		const UWorld* World = GetWorld();
		const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		LocalPlayer = GameInstance ? GameInstance->GetFirstGamePlayer() : nullptr;
	}

	UKzUIInputSubsystem* Subsystem = UKzUIInputSubsystem::Get(LocalPlayer);
	if (Subsystem != BoundSubsystem.Get())
	{
		if (UKzUIInputSubsystem* Previous = BoundSubsystem.Get())
		{
			Previous->OnInputDeviceChanged.RemoveDynamic(this, &UKzInputIcon::OnInputDeviceChanged);
		}
		if (Subsystem)
		{
			Subsystem->OnInputDeviceChanged.AddUniqueDynamic(this, &UKzInputIcon::OnInputDeviceChanged);
		}
		BoundSubsystem = Subsystem;
	}
	return Subsystem;
}

void UKzInputIcon::RefreshIcon()
{
	// Without a player, the project theme and the platform's own pad, which is where a new player starts
	EKzUIInputDevice Device = UKzUIInputSubsystem::GetPlatformGamepadDevice();
	const UKzUIIconTheme* Theme = nullptr;

#if WITH_EDITOR
	if (IsDesignTime())
	{
		Device = PreviewDevice;
		Theme = UKzUIInputSettings::Get()->IconTheme.LoadSynchronous();
	}
	else
#endif
	if (const UKzUIInputSubsystem* Subsystem = BindPlayerSubsystem())
	{
		Device = Subsystem->GetCurrentInputDevice();
		Theme = Subsystem->GetIconTheme();
	}
	else
	{
		Theme = UKzUIInputSettings::Get()->IconTheme.LoadSynchronous();
	}

	UObject* Icon = !Theme ? nullptr : CustomToken.IsNone() ? Theme->GetIcon(Input, Device) : Theme->GetCustomIcon(CustomToken, Device);
	if (!Icon)
	{
		ClearIcon();
		return;
	}

	// The icon may follow a ClearIcon
	if (GetBrush().DrawAs == ESlateBrushDrawType::NoDrawType)
	{
		FSlateBrush NewBrush = GetBrush();
		NewBrush.DrawAs = ESlateBrushDrawType::Image;
		SetBrush(NewBrush);
	}

	if (!Variation.IsNone())
	{
		UTexture2D* Texture = Cast<UTexture2D>(Icon);
		UMaterialInterface* VariationMaterial = Theme->GetVariation(Variation);
		if (Texture && VariationMaterial)
		{
			// The dynamic instance persists across device changes: only the texture parameter
			// moves, so runtime parameters like a hold progress survive the swap
			SetFlipbook(nullptr);
			UMaterialInstanceDynamic* Material = Cast<UMaterialInstanceDynamic>(GetBrush().GetResourceObject());
			if (!Material || Material->Parent != VariationMaterial)
			{
				Material = UMaterialInstanceDynamic::Create(VariationMaterial, this);
				SetBrushFromMaterial(Material);
			}
			Material->SetTextureParameterValue(Theme->VariationTextureParameter, Texture);
			return;
		}
		UE_LOG(LogKzUI, Warning, TEXT("[%s] Icon variation '%s' not applied: %s"), *GetName(), *Variation.ToString(), VariationMaterial ? TEXT("the resolved icon is not a plain texture") : TEXT("the theme does not define it"));
	}
	SetImage(Icon);
}

void UKzInputIcon::ClearIcon()
{
	SetFlipbook(nullptr);
	FSlateBrush NewBrush = GetBrush();
	NewBrush.SetResourceObject(nullptr);
	NewBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
	SetBrush(NewBrush);
}

void UKzInputIcon::OnInputDeviceChanged(EKzUIInputDevice InputDevice)
{
	RefreshIcon();
}
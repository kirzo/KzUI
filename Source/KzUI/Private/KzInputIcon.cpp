// Copyright 2026 kirzo

#include "KzInputIcon.h"
#include "KzUIIconTheme.h"
#include "KzUIInputSettings.h"
#include "KzUIInputSubsystem.h"

#include "Engine/Texture2D.h"
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

	if (!IsDesignTime())
	{
		if (UKzUIInputSubsystem* Subsystem = UKzUIInputSubsystem::Get(GetOwningLocalPlayer()))
		{
			Subsystem->OnInputDeviceChanged.AddUniqueDynamic(this, &UKzInputIcon::OnInputDeviceChanged);
		}
	}
	RefreshIcon();
}

void UKzInputIcon::RefreshIcon()
{
	EKzUIInputDevice Device = EKzUIInputDevice::Keyboard;
	const UKzUIIconTheme* Theme = nullptr;

#if WITH_EDITOR
	if (IsDesignTime())
	{
		Device = PreviewDevice;
		Theme = UKzUIInputSettings::Get()->IconTheme.LoadSynchronous();
	}
	else
#endif
	if (const UKzUIInputSubsystem* Subsystem = UKzUIInputSubsystem::Get(GetOwningLocalPlayer()))
	{
		Device = Subsystem->GetCurrentInputDevice();
		Theme = Subsystem->GetIconTheme();
	}

	if (Theme)
	{
		UObject* Icon = CustomToken.IsNone() ? Theme->GetIcon(Input, Device) : Theme->GetCustomIcon(CustomToken, Device);
		if (!Icon)
		{
			return;
		}

		if (!Variation.IsNone())
		{
			UTexture2D* Texture = Cast<UTexture2D>(Icon);
			UMaterialInterface* VariationMaterial = Theme->GetVariation(Variation);
			if (Texture && VariationMaterial)
			{
				// The dynamic instance persists across device changes: only the texture parameter
				// moves, so runtime parameters like a hold progress survive the swap
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
}

void UKzInputIcon::OnInputDeviceChanged(EKzUIInputDevice InputDevice)
{
	RefreshIcon();
}
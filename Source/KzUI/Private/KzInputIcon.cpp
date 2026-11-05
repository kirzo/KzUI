// Copyright 2026 kirzo

#include "KzInputIcon.h"
#include "KzUIFlipbook.h"
#include "KzUIIconTheme.h"
#include "KzUIInputSettings.h"
#include "KzUIInputSubsystem.h"

#include "Engine/LocalPlayer.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Widgets/Images/SImage.h"

UKzInputIcon::UKzInputIcon(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Source.InitializeAs<FKzUIIconSource_Input>();
}

void UKzInputIcon::SetSource(TInstancedStruct<FKzUIIconSource> NewSource)
{
	Source = MoveTemp(NewSource);
	RefreshIcon();
}

void UKzInputIcon::SetInput(EKzUIInputType NewInput)
{
	FKzUIIconSource_Input NewSource;
	NewSource.Input = NewInput;
	SetSource(TInstancedStruct<FKzUIIconSource>::Make<FKzUIIconSource_Input>(NewSource));
}

void UKzInputIcon::SetCustomToken(FName NewToken)
{
	FKzUIIconSource_Custom NewSource;
	NewSource.Token = NewToken;
	SetSource(TInstancedStruct<FKzUIIconSource>::Make<FKzUIIconSource_Custom>(NewSource));
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

	// An active timer and not the widget tick, which only runs while a flipbook plays
	if (bShowHoldProgress && !IsDesignTime() && MyImage.IsValid())
	{
		LastHoldProgress = -1.0f;
		const TWeakObjectPtr<UKzInputIcon> WeakThis(this);
		MyImage->RegisterActiveTimer(0.0f, FWidgetActiveTimerDelegate::CreateLambda([WeakThis](double, float)
			{
				return WeakThis.IsValid() ? WeakThis->UpdateHoldProgress() : EActiveTimerReturnType::Stop;
			}
		));
	}
}

EActiveTimerReturnType UKzInputIcon::UpdateHoldProgress()
{
	const FKzUIIconContext Context = MakeContext();
	const FKzUIIconSource* IconSource = Source.GetPtr();
	if (!IconSource || !Context.Theme)
	{
		return EActiveTimerReturnType::Continue;
	}

	const float Progress = IconSource->GetHoldProgress(Context);
	if (Progress != LastHoldProgress)
	{
		LastHoldProgress = Progress;
		SetMaterialScalarParameter(Context.Theme->HoldProgressParameter, Progress);
	}
	return EActiveTimerReturnType::Continue;
}

UKzUIInputSubsystem* UKzInputIcon::BindPlayerSubsystem()
{
	UKzUIInputSubsystem* Subsystem = UKzUIInputSubsystem::GetForWidget(this);
	if (Subsystem != BoundSubsystem.Get())
	{
		if (UKzUIInputSubsystem* Previous = BoundSubsystem.Get())
		{
			Previous->OnInputDeviceChanged.RemoveDynamic(this, &UKzInputIcon::OnInputDeviceChanged);
			Previous->OnInputMappingsChanged.RemoveDynamic(this, &UKzInputIcon::OnInputMappingsChanged);
		}
		if (Subsystem)
		{
			Subsystem->OnInputDeviceChanged.AddUniqueDynamic(this, &UKzInputIcon::OnInputDeviceChanged);
			Subsystem->OnInputMappingsChanged.AddUniqueDynamic(this, &UKzInputIcon::OnInputMappingsChanged);
		}
		BoundSubsystem = Subsystem;
	}
	return Subsystem;
}

FKzUIIconContext UKzInputIcon::MakeContext()
{
	// Without a player, the project theme and the platform's own pad, which is where a new player starts
	FKzUIIconContext Context;
	Context.Icon = this;
	Context.Device = UKzUIInputSubsystem::GetPlatformGamepadDevice();
	Context.GamepadDevice = Context.Device;

#if WITH_EDITOR
	if (IsDesignTime())
	{
		Context.Device = PreviewDevice;
		Context.GamepadDevice = PreviewDevice != EKzUIInputDevice::Keyboard ? PreviewDevice : Context.GamepadDevice;
		Context.Theme = UKzUIInputSettings::Get()->IconTheme.LoadSynchronous();
	}
	else
#endif
	if (const UKzUIInputSubsystem* Subsystem = BindPlayerSubsystem())
	{
		Context.Player = Subsystem->GetLocalPlayer();
		Context.Device = Subsystem->GetCurrentInputDevice();
		Context.GamepadDevice = Subsystem->GetGamepadDevice();
		Context.Theme = Subsystem->GetIconTheme();
	}
	else
	{
		Context.Theme = UKzUIInputSettings::Get()->IconTheme.LoadSynchronous();
	}

	// Also in the designer, so the Keyboard preview shows what the game will
	if (Context.Device == EKzUIInputDevice::Keyboard && UKzUIInputSettings::Get()->bGamepadIconsOnKeyboard)
	{
		Context.Device = Context.GamepadDevice;
	}
	return Context;
}

void UKzInputIcon::RefreshIcon()
{
	const FKzUIIconContext Context = MakeContext();
	const FKzUIIconSource* IconSource = Source.GetPtr();
	const FKzUIIconResolution Resolution = IconSource ? IconSource->Resolve(Context) : FKzUIIconResolution();

	UObject* Icon = Resolution.Icon;
	if (!Icon && Resolution.Key.IsValid() && Context.Theme)
	{
		Icon = Context.Theme->GetKeyIcon(Resolution.Key, Context.Device);
	}

	// A new material starts from its defaults, so the hold progress is pushed again
	LastHoldProgress = -1.0f;

	// A key without an icon shows its name, for a KzTextBlock to display
	FallbackText = Icon ? FText::GetEmpty() : Resolution.Key.IsValid() ? Resolution.Key.GetDisplayName() : Resolution.Text;
	if (!Icon)
	{
		ClearIcon();
		return;
	}

	// A material has no size of its own, and a variation wraps a texture that does
	if (const UTexture2D* Texture = Cast<UTexture2D>(Icon))
	{
		NaturalSize = FVector2D(Texture->GetSizeX(), Texture->GetSizeY());
	}
	else if (const UKzUIFlipbook* AsFlipbook = Cast<UKzUIFlipbook>(Icon))
	{
		NaturalSize = AsFlipbook->FrameSize;
	}
	else
	{
		NaturalSize = FVector2D(1.0, 1.0);
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
		UMaterialInterface* VariationMaterial = Context.Theme ? Context.Theme->GetVariation(Variation) : nullptr;
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
			Material->SetTextureParameterValue(Context.Theme->VariationTextureParameter, Texture);
			return;
		}
		UE_LOG(LogKzUI, Warning, TEXT("[%s] Icon variation '%s' not applied: %s"), *GetName(), *Variation.ToString(), VariationMaterial ? TEXT("the resolved icon is not a plain texture") : TEXT("the theme does not define it"));
	}
	SetImage(Icon);
}

void UKzInputIcon::ClearIcon()
{
	NaturalSize = FVector2D::ZeroVector;
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

void UKzInputIcon::OnInputMappingsChanged()
{
	RefreshIcon();
}
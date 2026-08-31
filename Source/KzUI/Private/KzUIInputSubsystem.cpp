// Copyright 2026 kirzo

#include "KzUIInputSubsystem.h"
#include "KzUserWidget.h"
#include "KzInputDeviceListener.h"
#include "KzUIStickKeyProcessor.h"
#include "KzUIInputSettings.h"
#include "KzUISoundTheme.h"
#include "KzUIIconTheme.h"

#include "Blueprint/WidgetTree.h"
#include "Containers/Ticker.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "Widgets/SViewport.h"
#include "GameFramework/InputDeviceSubsystem.h"
#include "GameFramework/InputSettings.h"

DEFINE_LOG_CATEGORY_STATIC(LogKzUI, Log, All);

// Devices whose scope has no HardwareDevices declaration arrive with an Unspecified primary
// type (GameInput does not declare any), so the name mapping decides and the type is fallback
static EKzUIInputDevice ToInputDevice(const FHardwareDeviceIdentifier& Device)
{
	if (const EKzUIInputDevice* Mapped = UKzUIInputSettings::Get()->DeviceMappings.Find(Device.HardwareDeviceIdentifier))
	{
		return *Mapped;
	}
	return Device.PrimaryDeviceType == EHardwareDevicePrimaryType::KeyboardAndMouse ? EKzUIInputDevice::Keyboard : EKzUIInputDevice::Xbox;
}

UKzUIInputSubsystem* UKzUIInputSubsystem::Get(const ULocalPlayer* LocalPlayer)
{
	return LocalPlayer ? LocalPlayer->GetSubsystem<UKzUIInputSubsystem>() : nullptr;
}

namespace
{
	// One app-wide processor shared by every local player, alive while any subsystem exists
	TSharedPtr<FKzUIStickKeyProcessor> StickKeyProcessor;
	int32 StickKeyProcessorUsers = 0;
}

void UKzUIInputSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (FSlateApplication::IsInitialized() && StickKeyProcessorUsers++ == 0)
	{
		StickKeyProcessor = MakeShared<FKzUIStickKeyProcessor>();
		FSlateApplication::Get().RegisterInputPreProcessor(StickKeyProcessor);
	}

	if (UInputDeviceSubsystem* DeviceSubsystem = UInputDeviceSubsystem::Get())
	{
		DeviceSubsystem->OnInputHardwareDeviceChanged.AddDynamic(this, &UKzUIInputSubsystem::OnHardwareDeviceChanged);
	}

	// The device may already differ from the default when this player is created
	RefreshCurrentInputDevice();

	if (FSlateApplication::IsInitialized())
	{
		AppActivationHandle = FSlateApplication::Get().OnApplicationActivationStateChanged().AddUObject(this, &UKzUIInputSubsystem::OnAppActivationChanged);
		FocusChangingHandle = FSlateApplication::Get().OnFocusChanging().AddUObject(this, &UKzUIInputSubsystem::OnFocusChanging);
	}
}

void UKzUIInputSubsystem::Deinitialize()
{
	if (StickKeyProcessor && --StickKeyProcessorUsers == 0)
	{
		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().UnregisterInputPreProcessor(StickKeyProcessor);
		}
		StickKeyProcessor.Reset();
	}

	if (UInputDeviceSubsystem* DeviceSubsystem = UInputDeviceSubsystem::Get())
	{
		DeviceSubsystem->OnInputHardwareDeviceChanged.RemoveDynamic(this, &UKzUIInputSubsystem::OnHardwareDeviceChanged);
	}

	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().OnApplicationActivationStateChanged().Remove(AppActivationHandle);
		FSlateApplication::Get().OnFocusChanging().Remove(FocusChangingHandle);
	}
	AppActivationHandle.Reset();
	FocusChangingHandle.Reset();

	WidgetStack.Empty();
	Super::Deinitialize();
}

void UKzUIInputSubsystem::RegisterWidget(UKzUserWidget* Widget)
{
	if (IsValid(Widget))
	{
		WidgetStack.AddUnique(Widget);
		if (Widget->bSoloInput)
		{
			SuspendOtherPlayers(Widget, true);
		}
		if (Widget->IsFocusable() && !IsEditorSimulating())
		{
			Widget->SetFocus();
		}
	}
}

void UKzUIInputSubsystem::UnregisterWidget(UKzUserWidget* Widget)
{
	if (WidgetStack.Remove(Widget) > 0 && Widget)
	{
		if (Widget->bSoloInput)
		{
			SuspendOtherPlayers(Widget, false);
		}
		if (Widget->IsFocusable())
		{
			FocusTopWidget();
		}
	}
}

void UKzUIInputSubsystem::SuspendInput(UObject* Source)
{
	if (!Source)
	{
		return;
	}

	const bool bWasSuspended = IsInputSuspended();
	InputSuspensionSources.Add(Source);

	// Reset in-flight presses so no widget is left half way through a press or a hold
	if (!bWasSuspended)
	{
		for (UKzUserWidget* Widget : WidgetStack)
		{
			if (IsValid(Widget))
			{
				Widget->ClearInput();
			}
		}
	}
}

void UKzUIInputSubsystem::ResumeInput(UObject* Source)
{
	InputSuspensionSources.Remove(Source);

	for (auto It = InputSuspensionSources.CreateIterator(); It; ++It)
	{
		if (!It->IsValid())
		{
			It.RemoveCurrent();
		}
	}
}

bool UKzUIInputSubsystem::IsInputSuspended() const
{
	for (const TWeakObjectPtr<UObject>& Source : InputSuspensionSources)
	{
		if (Source.IsValid())
		{
			return true;
		}
	}
	return false;
}

void UKzUIInputSubsystem::SuspendOtherPlayers(UKzUserWidget* Widget, bool bSuspend)
{
	const ULocalPlayer* Self = GetLocalPlayer<ULocalPlayer>();
	const UGameInstance* GameInstance = Self ? Self->GetGameInstance() : nullptr;
	if (!GameInstance)
	{
		return;
	}

	for (ULocalPlayer* Player : GameInstance->GetLocalPlayers())
	{
		if (Player && Player != Self)
		{
			if (UKzUIInputSubsystem* Other = Get(Player))
			{
				bSuspend ? Other->SuspendInput(Widget) : Other->ResumeInput(Widget);
			}
		}
	}
}

void UKzUIInputSubsystem::FocusTopWidget()
{
	// While ejected in PIE (F8), the editor camera owns the viewport focus
	if (IsEditorSimulating())
	{
		return;
	}

	for (int32 i = WidgetStack.Num(); i--;)
	{
		if (IsValid(WidgetStack[i]) && WidgetStack[i]->IsFocusable())
		{
			WidgetStack[i]->SetFocus();
			return;
		}
	}
}

bool UKzUIInputSubsystem::IsEditorSimulating() const
{
#if WITH_EDITOR
	const ULocalPlayer* LocalPlayer = GetLocalPlayer<ULocalPlayer>();
	return LocalPlayer && LocalPlayer->ViewportClient && LocalPlayer->ViewportClient->IsSimulateInEditorViewport();
#else
	return false;
#endif
}

void UKzUIInputSubsystem::SetCurrentInputDevice(EKzUIInputDevice NewInputDevice)
{
	if (CurrentInputDevice == NewInputDevice)
	{
		return;
	}
	CurrentInputDevice = NewInputDevice;
	ApplyCursorPolicy();

	for (UKzUserWidget* Widget : WidgetStack)
	{
		if (!IsValid(Widget))
		{
			continue;
		}

		if (Widget->Implements<UKzInputDeviceListener>())
		{
			IKzInputDeviceListener::Execute_OnInputDeviceChanged(Widget, CurrentInputDevice);
		}

		if (Widget->WidgetTree)
		{
			Widget->WidgetTree->ForEachWidget([NewInputDevice](UWidget* Child)
				{
					if (Child->Implements<UKzInputDeviceListener>())
					{
						IKzInputDeviceListener::Execute_OnInputDeviceChanged(Child, NewInputDevice);
					}
				}
			);
		}
	}

	OnInputDeviceChanged.Broadcast(CurrentInputDevice);
}

UKzUISoundTheme* UKzUIInputSubsystem::GetSoundTheme() const
{
	return SoundThemeOverride ? SoundThemeOverride.Get() : UKzUIInputSettings::Get()->SoundTheme.LoadSynchronous();
}

void UKzUIInputSubsystem::SetIconTheme(UKzUIIconTheme* Theme)
{
	IconThemeOverride = Theme;
	// Icons listen to device changes; re-broadcast so they refresh with the new theme
	OnInputDeviceChanged.Broadcast(CurrentInputDevice);
}

UKzUIIconTheme* UKzUIInputSubsystem::GetIconTheme() const
{
	return IconThemeOverride ? IconThemeOverride.Get() : UKzUIInputSettings::Get()->IconTheme.LoadSynchronous();
}

void UKzUIInputSubsystem::OnHardwareDeviceChanged(const FPlatformUserId UserId, const FInputDeviceId DeviceId)
{
	const ULocalPlayer* LocalPlayer = GetLocalPlayer<ULocalPlayer>();
	if (LocalPlayer && LocalPlayer->GetPlatformUserId() == UserId)
	{
		RefreshCurrentInputDevice();
	}
}

void UKzUIInputSubsystem::RefreshCurrentInputDevice()
{
	const ULocalPlayer* LocalPlayer = GetLocalPlayer<ULocalPlayer>();
	const UInputDeviceSubsystem* DeviceSubsystem = UInputDeviceSubsystem::Get();
	if (!LocalPlayer || !DeviceSubsystem)
	{
		return;
	}

	const FHardwareDeviceIdentifier Device = DeviceSubsystem->GetMostRecentlyUsedHardwareDevice(LocalPlayer->GetPlatformUserId());
	if (Device.HardwareDeviceIdentifier.IsNone())
	{
		return;
	}

	const EKzUIInputDevice InputDevice = ToInputDevice(Device);
	UE_LOG(LogKzUI, Log, TEXT("Player %d hardware device: '%s' (class '%s', type %d) -> %s"), LocalPlayer->GetControllerId(), *Device.HardwareDeviceIdentifier.ToString(), *Device.InputClassName.ToString(), (int32)Device.PrimaryDeviceType, *UEnum::GetValueAsString(InputDevice));
	SetCurrentInputDevice(InputDevice);
}

void UKzUIInputSubsystem::OnAppActivationChanged(bool bActivated)
{
	// Give focus back to the topmost focusable widget when the application regains it
	if (bActivated)
	{
		FocusTopWidget();
	}
}

void UKzUIInputSubsystem::OnFocusChanging(const FFocusEvent& FocusEvent, const FWeakWidgetPath& OldPath, const TSharedPtr<SWidget>& OldWidget, const FWidgetPath& NewPath, const TSharedPtr<SWidget>& NewWidget)
{
	const ULocalPlayer* LocalPlayer = GetLocalPlayer<ULocalPlayer>();
	const TSharedPtr<const FSlateUser> SlateUser = LocalPlayer ? LocalPlayer->GetSlateUser() : nullptr;
	if (!SlateUser || (int32)FocusEvent.GetUser() != SlateUser->GetUserIndex())
	{
		return;
	}

	// Focus moving to another widget is respected; only the game viewport (or nothing) gets overruled
	const TSharedPtr<SWidget> ViewportWidget = LocalPlayer->ViewportClient ? LocalPlayer->ViewportClient->GetGameViewportWidget() : nullptr;
	if (NewWidget && NewWidget != ViewportWidget)
	{
		return;
	}

	// Deferred to the next engine tick: mutating the focus while it is changing is unsafe,
	// and world timers stall while the game is paused
	TWeakObjectPtr<UKzUIInputSubsystem> WeakThis(this);
	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakThis](float)
		{
			if (UKzUIInputSubsystem* Subsystem = WeakThis.Get())
			{
				Subsystem->RestoreStackFocus();
			}
			return false;
		}));
}

void UKzUIInputSubsystem::ApplyCursorPolicy()
{
	if (UKzUIInputSettings::Get()->CursorPolicy != EKzUICursorPolicy::ByDevice)
	{
		return;
	}

	const ULocalPlayer* LocalPlayer = GetLocalPlayer<ULocalPlayer>();
	if (APlayerController* PlayerController = LocalPlayer ? LocalPlayer->PlayerController.Get() : nullptr)
	{
		PlayerController->SetShowMouseCursor(CurrentInputDevice == EKzUIInputDevice::Keyboard);
	}
}

void UKzUIInputSubsystem::RestoreStackFocus()
{
	const ULocalPlayer* LocalPlayer = GetLocalPlayer<ULocalPlayer>();
	const TSharedPtr<const FSlateUser> SlateUser = LocalPlayer ? LocalPlayer->GetSlateUser() : nullptr;
	if (!SlateUser)
	{
		return;
	}

	// Re-check after the deferral: act only if the viewport (or nothing) still holds the focus
	const TSharedPtr<SWidget> Focused = SlateUser->GetFocusedWidget();
	const TSharedPtr<SWidget> ViewportWidget = LocalPlayer->ViewportClient ? LocalPlayer->ViewportClient->GetGameViewportWidget() : nullptr;
	if (!Focused || Focused == ViewportWidget)
	{
		FocusTopWidget();
	}
}
// Copyright 2026 kirzo

#include "KzUIInputSubsystem.h"
#include "KzUserWidget.h"
#include "KzInputDeviceListener.h"
#include "KzUIInputPreprocessor.h"
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

UKzUIInputSubsystem* UKzUIInputSubsystem::Get(const ULocalPlayer* LocalPlayer)
{
	return LocalPlayer ? LocalPlayer->GetSubsystem<UKzUIInputSubsystem>() : nullptr;
}

namespace
{
	// One app-wide preprocessor shared by every local player, alive while any subsystem exists
	TSharedPtr<FKzUIInputPreprocessor> InputPreprocessor;
	int32 InputPreprocessorUsers = 0;
}

void UKzUIInputSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (FSlateApplication::IsInitialized() && InputPreprocessorUsers++ == 0)
	{
		InputPreprocessor = MakeShared<FKzUIInputPreprocessor>();
		FSlateApplication::Get().RegisterInputPreProcessor(InputPreprocessor);
	}

	// A player usually joins by pressing something, so adopt the device that input came from.
	// Deferred because the local player is not registered with Slate yet at this point.
	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float)
		{
			const ULocalPlayer* LocalPlayer = GetLocalPlayer<ULocalPlayer>();
			const TSharedPtr<const FSlateUser> SlateUser = LocalPlayer ? LocalPlayer->GetSlateUser() : nullptr;
			EKzUIInputDevice Device = EKzUIInputDevice::Keyboard;
			if (SlateUser && InputPreprocessor && InputPreprocessor->TryGetDeviceForSlateUser(SlateUser->GetUserIndex(), Device))
			{
				SetCurrentInputDevice(Device);
			}
			return false;
		}
	));

	if (FSlateApplication::IsInitialized())
	{
		AppActivationHandle = FSlateApplication::Get().OnApplicationActivationStateChanged().AddUObject(this, &UKzUIInputSubsystem::OnAppActivationChanged);
		FocusChangingHandle = FSlateApplication::Get().OnFocusChanging().AddUObject(this, &UKzUIInputSubsystem::OnFocusChanging);
	}
}

void UKzUIInputSubsystem::Deinitialize()
{
	if (InputPreprocessor && --InputPreprocessorUsers == 0)
	{
		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().UnregisterInputPreProcessor(InputPreprocessor);
		}
		InputPreprocessor.Reset();
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
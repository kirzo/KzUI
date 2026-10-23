// Copyright 2026 kirzo

#include "KzUIInputSubsystem.h"
#include "KzUserWidget.h"
#include "KzInputDeviceListener.h"
#include "KzUIInputPreprocessor.h"
#include "KzUIInputSettings.h"
#include "KzUISoundTheme.h"
#include "KzUIIconTheme.h"
#include "KzPromptWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Containers/Ticker.h"
#include "Engine/AssetManager.h"
#include "Engine/GameInstance.h"
#include "Engine/Console.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GenericPlatform/GenericApplicationMessageHandler.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "HAL/PlatformProperties.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "Slate/SceneViewport.h"
#include "Widgets/SViewport.h"

UKzUIInputSubsystem* UKzUIInputSubsystem::Get(const ULocalPlayer* LocalPlayer)
{
	return LocalPlayer ? LocalPlayer->GetSubsystem<UKzUIInputSubsystem>() : nullptr;
}

UKzUIInputSubsystem* UKzUIInputSubsystem::FindForSlateUser(int32 UserIndex)
{
	if (!GEngine)
	{
		return nullptr;
	}

	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		const UGameInstance* GameInstance = Context.OwningGameInstance;
		if (!GameInstance)
		{
			continue;
		}

		for (ULocalPlayer* Player : GameInstance->GetLocalPlayers())
		{
			const TSharedPtr<const FSlateUser> SlateUser = Player ? Player->GetSlateUser() : nullptr;
			if (SlateUser && SlateUser->GetUserIndex() == UserIndex)
			{
				return Get(Player);
			}
		}
	}
	return nullptr;
}

UKzUIInputSubsystem* UKzUIInputSubsystem::GetForWidget(const UWidget* Widget)
{
	if (!Widget)
	{
		return nullptr;
	}

	const ULocalPlayer* LocalPlayer = Widget->GetOwningLocalPlayer();
	if (!LocalPlayer)
	{
		const UWorld* World = Widget->GetWorld();
		const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		LocalPlayer = GameInstance ? GameInstance->GetFirstGamePlayer() : nullptr;
	}
	return Get(LocalPlayer);
}

EKzUIInputDevice UKzUIInputSubsystem::GetPlatformGamepadDevice()
{
	const FString Platform = FPlatformProperties::IniPlatformName();
	if (Platform == TEXT("PS4"))
	{
		return EKzUIInputDevice::DualShock;
	}
	if (Platform == TEXT("PS5"))
	{
		return EKzUIInputDevice::DualSense;
	}
	if (Platform == TEXT("Switch"))
	{
		return EKzUIInputDevice::Switch;
	}
	return EKzUIInputDevice::Xbox;
}

namespace
{
	// One app-wide preprocessor shared by every local player, alive while any subsystem exists
	TSharedPtr<FKzUIInputPreprocessor> InputPreprocessor;
	int32 InputPreprocessorUsers = 0;

	// Pad family of every connected device whose backend told us its hardware
	TMap<FInputDeviceId, EKzUIInputDevice> ConnectedPads;

	const EKzUIInputDevice* FindConnectedPad(FPlatformUserId UserId)
	{
		TArray<FInputDeviceId> Devices;
		IPlatformInputDeviceMapper::Get().GetAllInputDevicesForUser(UserId, Devices);
		for (const FInputDeviceId Device : Devices)
		{
			if (const EKzUIInputDevice* PadDevice = ConnectedPads.Find(Device))
			{
				return PadDevice;
			}
		}
		return nullptr;
	}
}

void UKzUIInputSubsystem::HandleInputDeviceConnectionChange(EInputDeviceConnectionState NewConnectionState, FPlatformUserId PlatformUserId, FInputDeviceId InputDeviceId)
{
	if (NewConnectionState != EInputDeviceConnectionState::Connected)
	{
		ConnectedPads.Remove(InputDeviceId);
		return;
	}

	const FInputDeviceScope* Scope = FInputDeviceScope::GetCurrent();
	if (Scope && !Scope->HardwareDeviceIdentifier.IsEmpty())
	{
		const EKzUIInputDevice* Mapped = UKzUIInputSettings::Get()->DeviceMappings.Find(FName(*Scope->HardwareDeviceIdentifier));
		ConnectedPads.Add(InputDeviceId, Mapped ? *Mapped : GetPlatformGamepadDevice());
	}
}

void UKzUIInputSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (FSlateApplication::IsInitialized() && InputPreprocessorUsers++ == 0)
	{
		InputPreprocessor = MakeShared<FKzUIInputPreprocessor>();
		FSlateApplication::Get().RegisterInputPreProcessor(InputPreprocessor);
	}

	// Start on the pad the platform assigned to this player; a player who joined by pressing something
	// adopts the device that input came from instead. Deferred because the local player is not registered with Slate yet.
	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float)
		{
			const ULocalPlayer* LocalPlayer = GetLocalPlayer<ULocalPlayer>();
			if (const EKzUIInputDevice* PadDevice = LocalPlayer ? FindConnectedPad(LocalPlayer->GetPlatformUserId()) : nullptr)
			{
				SetCurrentInputDevice(*PadDevice);
			}

			const TSharedPtr<const FSlateUser> SlateUser = LocalPlayer ? LocalPlayer->GetSlateUser() : nullptr;
			EKzUIInputDevice Device = EKzUIInputDevice::Keyboard;
			if (SlateUser && InputPreprocessor && InputPreprocessor->TryGetDeviceForSlateUser(SlateUser->GetUserIndex(), Device))
			{
				SetCurrentInputDevice(Device);
			}
			ApplyCursorPolicy();
			return false;
		}
	));

	const TSoftClassPtr<UKzPromptWidget>& PromptClass = UKzUIInputSettings::Get()->PromptClass;
	if (!PromptClass.IsNull())
	{
		PromptClassHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(PromptClass.ToSoftObjectPath());
	}

	if (FSlateApplication::IsInitialized())
	{
		AppActivationHandle = FSlateApplication::Get().OnApplicationActivationStateChanged().AddUObject(this, &UKzUIInputSubsystem::OnAppActivationChanged);
		FocusChangingHandle = FSlateApplication::Get().OnFocusChanging().AddUObject(this, &UKzUIInputSubsystem::OnFocusChanging);
	}

	// The player controller usually arrives after the local player, and the cursor is its setting
	if (ULocalPlayer* LocalPlayer = GetLocalPlayer<ULocalPlayer>())
	{
		PlayerControllerHandle = LocalPlayer->OnPlayerControllerChanged().AddWeakLambda(this, [this](APlayerController*)
			{
				ApplyCursorPolicy();
			});
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

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer<ULocalPlayer>())
	{
		LocalPlayer->OnPlayerControllerChanged().Remove(PlayerControllerHandle);
	}
	PlayerControllerHandle.Reset();

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
		if (Widget->IsFocusable())
		{
			FocusTopWidget();
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

bool UKzUIInputSubsystem::IsConsoleActive()
{
	// The console reads its keys from the game viewport, at the end of the Slate bubble chain, so
	// any widget that consumes them first leaves it deaf
	const UGameViewportClient* ViewportClient = GEngine ? GEngine->GameViewport : nullptr;
	return ViewportClient && ViewportClient->ViewportConsole && ViewportClient->ViewportConsole->ConsoleActive();
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
	if (!CanTakeFocus())
	{
		return;
	}

	UKzUserWidget* Target = nullptr;
	for (int32 i = WidgetStack.Num(); i--;)
	{
		UKzUserWidget* Widget = WidgetStack[i];
		if (IsValid(Widget) && Widget->IsFocusable())
		{
			if (Widget->WantsFocus())
			{
				Target = Widget;
				break;
			}
			Target = Target ? Target : Widget;
		}
	}

	if (Target)
	{
		Target->SetFocus();
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

bool UKzUIInputSubsystem::CanTakeFocus() const
{
	// Something outside the game deliberately owns the input: the editor camera while ejected in
	// PIE (F8), or the console, which focuses the viewport for itself
	if (IsEditorSimulating() || IsConsoleActive())
	{
		return false;
	}

	const ULocalPlayer* LocalPlayer = GetLocalPlayer<ULocalPlayer>();
	const TSharedPtr<const FSlateUser> SlateUser = LocalPlayer ? LocalPlayer->GetSlateUser() : nullptr;
	const TSharedPtr<SWidget> Focused = SlateUser ? SlateUser->GetFocusedWidget() : nullptr;
	if (!Focused)
	{
		return true;
	}

	// Focus held outside the game viewport means the game is not the active input target, which in
	// the editor is any panel being worked on. Game widgets all live under the viewport, so this
	// only ever refuses in editor situations.
	const TSharedPtr<SWidget> ViewportWidget = LocalPlayer->ViewportClient ? LocalPlayer->ViewportClient->GetGameViewportWidget() : nullptr;
	for (TSharedPtr<SWidget> Widget = Focused; Widget.IsValid(); Widget = Widget->GetParentWidget())
	{
		if (Widget == ViewportWidget)
		{
			return true;
		}
	}
	return false;
}

void UKzUIInputSubsystem::SetCurrentInputDevice(EKzUIInputDevice NewInputDevice)
{
	if (NewInputDevice != EKzUIInputDevice::Keyboard)
	{
		GamepadDevice = NewInputDevice;
	}

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
	if (bActivated)
	{
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float)
			{
				RestoreAfterActivation();
				return false;
			}));
		return;
	}

	// A key released in another app never sends its key up and Slate keeps the focus, so every press is dropped now
	for (UKzUserWidget* Widget : WidgetStack)
	{
		if (IsValid(Widget))
		{
			Widget->ClearInput();
		}
	}
	if (InputPreprocessor && FSlateApplication::IsInitialized())
	{
		InputPreprocessor->ReleaseStickKeys(FSlateApplication::Get());
	}

	const ULocalPlayer* LocalPlayer = GetLocalPlayer<ULocalPlayer>();
	const TSharedPtr<const FSlateUser> SlateUser = LocalPlayer ? LocalPlayer->GetSlateUser() : nullptr;
	FocusOnDeactivate = SlateUser ? SlateUser->GetFocusedWidget() : nullptr;
}

void UKzUIInputSubsystem::RestoreAfterActivation()
{
	const ULocalPlayer* LocalPlayer = GetLocalPlayer<ULocalPlayer>();
	const TSharedPtr<const FSlateUser> SlateUser = LocalPlayer ? LocalPlayer->GetSlateUser() : nullptr;
	const TSharedPtr<SWidget> PreviousFocus = FocusOnDeactivate.Pin();
	FocusOnDeactivate.Reset();
	if (!SlateUser || !FSlateApplication::Get().IsActive())
	{
		return;
	}

	// SWindow gives every user the keyboard user's focus on activation: another player gets its own back
	FSlateApplication& SlateApp = FSlateApplication::Get();
	const bool bKeyboardUser = SlateUser->GetUserIndex() == SlateApp.GetUserIndexForKeyboard();
	if (!bKeyboardUser && PreviousFocus && SlateUser->GetFocusedWidget() != PreviousFocus && SlateUser->GetFocusedWidget() == SlateApp.GetKeyboardFocusedWidget())
	{
		SlateApp.SetUserFocus(SlateUser->GetUserIndex(), PreviousFocus, EFocusCause::WindowActivate);
	}

	// Restore rather than force: on activation the focus may legitimately belong elsewhere
	RestoreStackFocus();

	if (bKeyboardUser)
	{
		RecaptureMouse();
	}
}

void UKzUIInputSubsystem::RecaptureMouse()
{
	// The game viewport only hides the cursor while it has the focus or the mouse. KzUI keeps the focus on its widgets,
	// and the engine recaptures on activation only outside the editor, so the cursor would stay visible over the game
	ULocalPlayer* LocalPlayer = GetLocalPlayer<ULocalPlayer>();
	UGameViewportClient* ViewportClient = LocalPlayer ? LocalPlayer->ViewportClient.Get() : nullptr;
	const APlayerController* PlayerController = LocalPlayer ? LocalPlayer->PlayerController.Get() : nullptr;
	if (!ViewportClient || !PlayerController || PlayerController->ShouldShowMouseCursor() || !CanTakeFocus())
	{
		return;
	}

	const EMouseCaptureMode CaptureMode = ViewportClient->GetMouseCaptureMode();
	if (CaptureMode != EMouseCaptureMode::CapturePermanently && CaptureMode != EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown)
	{
		return;
	}

	const FSceneViewport* Viewport = ViewportClient->GetGameViewport();
	const TSharedPtr<SViewport> ViewportWidget = ViewportClient->GetGameViewportWidget();
	if (!Viewport || !ViewportWidget || Viewport->HasMouseCapture())
	{
		return;
	}

	// Applied by the engine loop on the game viewport next frame, like the player controller's input modes
	FReply& SlateOperations = LocalPlayer->GetSlateOperations();
	SlateOperations.CaptureMouse(ViewportWidget.ToSharedRef());
	if (ViewportClient->LockDuringCapture())
	{
		SlateOperations.LockMouseToWidget(ViewportWidget.ToSharedRef());
	}
	SlateOperations.UseHighPrecisionMouseMovement(ViewportWidget.ToSharedRef());
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
	// Without mouse support the cursor stays hidden whatever the policy says
	const UKzUIInputSettings* Settings = UKzUIInputSettings::Get();
	if (Settings->bSupportsMouse && Settings->CursorPolicy != EKzUICursorPolicy::ByDevice)
	{
		return;
	}

	const ULocalPlayer* LocalPlayer = GetLocalPlayer<ULocalPlayer>();
	if (APlayerController* PlayerController = LocalPlayer ? LocalPlayer->PlayerController.Get() : nullptr)
	{
		PlayerController->SetShowMouseCursor(Settings->bSupportsMouse && CurrentInputDevice == EKzUIInputDevice::Keyboard);
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
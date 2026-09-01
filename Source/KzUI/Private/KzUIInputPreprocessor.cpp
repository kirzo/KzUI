// Copyright 2026 kirzo

#include "KzUIInputPreprocessor.h"

#include "KzUIInputSettings.h"
#include "KzUIInputSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "GenericPlatform/GenericApplicationMessageHandler.h"
#include "InputCoreTypes.h"
#include "Misc/ConfigCacheIni.h"

namespace
{
	// Press matches the Slate analog navigation threshold; the release gap is hysteresis so a
	// value hovering around the press threshold does not chatter press/release
	constexpr float PressThreshold = 0.5f;
	constexpr float ReleaseThreshold = 0.4f;

	struct FStickDirectionKeys { FKey Positive; FKey Negative; };

	const TMap<FKey, FStickDirectionKeys>& GetAxisMap()
	{
		static const TMap<FKey, FStickDirectionKeys> AxisMap = {
			{ EKeys::Gamepad_LeftX, { EKeys::Gamepad_LeftStick_Right, EKeys::Gamepad_LeftStick_Left } },
			{ EKeys::Gamepad_LeftY, { EKeys::Gamepad_LeftStick_Up, EKeys::Gamepad_LeftStick_Down } },
			{ EKeys::Gamepad_RightX, { EKeys::Gamepad_RightStick_Right, EKeys::Gamepad_RightStick_Left } },
			{ EKeys::Gamepad_RightY, { EKeys::Gamepad_RightStick_Up, EKeys::Gamepad_RightStick_Down } } };
		return AxisMap;
	}

	bool IsStickDirectionKey(const FKey& Key)
	{
		return Key == EKeys::Gamepad_LeftStick_Up || Key == EKeys::Gamepad_LeftStick_Down
			|| Key == EKeys::Gamepad_LeftStick_Left || Key == EKeys::Gamepad_LeftStick_Right
			|| Key == EKeys::Gamepad_RightStick_Up || Key == EKeys::Gamepad_RightStick_Down
			|| Key == EKeys::Gamepad_RightStick_Left || Key == EKeys::Gamepad_RightStick_Right;
	}

	/** Same repeat cadence the platform layer uses for key repeats, including its InputSettings ini overrides. */
	struct FButtonRepeatDelays
	{
		float Initial = 0.2f;
		float Interval = 0.1f;

		FButtonRepeatDelays()
		{
			GConfig->GetFloat(TEXT("/Script/Engine.InputSettings"), TEXT("InitialButtonRepeatDelay"), Initial, GInputIni);
			GConfig->GetFloat(TEXT("/Script/Engine.InputSettings"), TEXT("ButtonRepeatDelay"), Interval, GInputIni);
		}
	};

	const FButtonRepeatDelays& GetButtonRepeatDelays()
	{
		static const FButtonRepeatDelays Delays;
		return Delays;
	}

	UKzUIInputSubsystem* FindSubsystemForSlateUser(int32 UserIndex)
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
					return UKzUIInputSubsystem::Get(Player);
				}
			}
		}
		return nullptr;
	}
}

void FKzUIInputPreprocessor::Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor)
{
	// Analog events only fire on value changes, so held directions repeat from here
	const double Now = SlateApp.GetCurrentTime();
	for (auto& Pair : AxisStates)
	{
		FAxisState& State = Pair.Value;
		if (State.Key.IsValid() && Now >= State.NextRepeatTime)
		{
			State.NextRepeatTime = Now + GetButtonRepeatDelays().Interval;
			InjectKeyEvent(SlateApp, State.Key, Pair.Key.Key, true, true);
		}
	}
}

bool FKzUIInputPreprocessor::HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent)
{
	// Injected keys carry no device of their own: the analog event that produced them already tracked it
	if (!bInjecting)
	{
		TrackDevice(InKeyEvent.GetUserIndex(), InKeyEvent.GetKey());
		return IsStickDirectionKey(InKeyEvent.GetKey());
	}
	return false;
}

bool FKzUIInputPreprocessor::HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent)
{
	return !bInjecting && IsStickDirectionKey(InKeyEvent.GetKey());
}

bool FKzUIInputPreprocessor::HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	TrackDevice(MouseEvent.GetUserIndex(), MouseEvent.GetEffectingButton());
	return false;
}

bool FKzUIInputPreprocessor::HandleAnalogInputEvent(FSlateApplication& SlateApp, const FAnalogInputEvent& InAnalogInputEvent)
{
	const FStickDirectionKeys* Directions = GetAxisMap().Find(InAnalogInputEvent.GetKey());
	if (!Directions)
	{
		return false;
	}

	FAxisState& State = AxisStates.FindOrAdd({ InAnalogInputEvent.GetUserIndex(), InAnalogInputEvent.GetKey() });
	const float Value = InAnalogInputEvent.GetAnalogValue();

	FKey NewKey;
	if (Value >= PressThreshold)
	{
		NewKey = Directions->Positive;
	}
	else if (Value <= -PressThreshold)
	{
		NewKey = Directions->Negative;
	}
	else if ((State.Key == Directions->Positive && Value >= ReleaseThreshold) || (State.Key == Directions->Negative && Value <= -ReleaseThreshold))
	{
		NewKey = State.Key;
	}

	if (NewKey != State.Key)
	{
		if (State.Key.IsValid())
		{
			InjectKeyEvent(SlateApp, State.Key, InAnalogInputEvent.GetUserIndex(), false, false);
		}
		if (NewKey.IsValid())
		{
			// Tracked here and not on the injected key, which is dispatched outside the device scope
			TrackDevice(InAnalogInputEvent.GetUserIndex(), InAnalogInputEvent.GetKey());
			State.NextRepeatTime = SlateApp.GetCurrentTime() + GetButtonRepeatDelays().Initial;
			InjectKeyEvent(SlateApp, NewKey, InAnalogInputEvent.GetUserIndex(), true, false);
		}
		State.Key = NewKey;
	}

	// The axis value itself keeps flowing to widgets and the game
	return false;
}

bool FKzUIInputPreprocessor::TryGetDeviceForSlateUser(int32 UserIndex, EKzUIInputDevice& OutDevice) const
{
	if (const EKzUIInputDevice* Device = DevicesByUser.Find(UserIndex))
	{
		OutDevice = *Device;
		return true;
	}
	return false;
}

void FKzUIInputPreprocessor::TrackDevice(int32 UserIndex, const FKey& Key)
{
	EKzUIInputDevice Device = EKzUIInputDevice::Keyboard;
	if (Key.IsGamepadKey())
	{
		// Read while the backend is still dispatching the event: device ids are recycled between
		// physical devices, so resolving one afterwards can name the wrong hardware
		FName DeviceName;
		if (const FInputDeviceScope* Scope = FInputDeviceScope::GetCurrent())
		{
			DeviceName = FName(*Scope->HardwareDeviceIdentifier);
		}

		const EKzUIInputDevice* Mapped = UKzUIInputSettings::Get()->DeviceMappings.Find(DeviceName);
		Device = Mapped ? *Mapped : EKzUIInputDevice::Xbox;
	}

	DevicesByUser.Add(UserIndex, Device);

	if (UKzUIInputSubsystem* Subsystem = FindSubsystemForSlateUser(UserIndex))
	{
		Subsystem->SetCurrentInputDevice(Device);
	}
}

void FKzUIInputPreprocessor::InjectKeyEvent(FSlateApplication& SlateApp, const FKey& Key, int32 UserIndex, bool bPress, bool bRepeat)
{
	const FKeyEvent Event(Key, FModifierKeysState(), UserIndex, bRepeat, 0, 0);
	TGuardValue<bool> Guard(bInjecting, true);
	if (bPress)
	{
		SlateApp.ProcessKeyDownEvent(Event);
	}
	else
	{
		SlateApp.ProcessKeyUpEvent(Event);
	}
}
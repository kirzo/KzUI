// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Framework/Application/IInputProcessor.h"
#include "KzUITypes.h"

/**
 * App-wide input plumbing for KzUI, with two jobs.
 *
 * Stick keys: pads GameInput treats as generic controllers never send the digital Gamepad_*Stick_*
 * keys that native gamepads do, so UI navigation would silently lose the sticks on them. Instead of
 * detecting which backend does what, every platform-sent stick-direction key is consumed and
 * re-synthesized from the analog axes: single source, same threshold, hysteresis and repeat cadence
 * on every pad.
 *
 * Device tracking: the input device of each local player is read from the events themselves. The
 * platform user cannot be used, because several devices (and therefore several local players) can
 * share one, and input device ids are recycled between physical devices; the slate user carried by
 * the event is the identity Slate actually routed the input with.
 */
class FKzUIInputPreprocessor : public IInputProcessor
{
public:
	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override;
	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override;
	virtual bool HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override;
	virtual bool HandleAnalogInputEvent(FSlateApplication& SlateApp, const FAnalogInputEvent& InAnalogInputEvent) override;
	virtual bool HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override;

	/** Device last used by the given slate user. Lets a subsystem created by an input event start on the device that caused it. */
	bool TryGetDeviceForSlateUser(int32 UserIndex, EKzUIInputDevice& OutDevice) const;

private:
	struct FAxisState
	{
		FKey Key;
		double NextRepeatTime = 0.0;
	};

	/** Active synthetic stick-direction key per (slate user, analog axis). */
	TMap<TPair<int32, FKey>, FAxisState> AxisStates;

	/** Last device seen per slate user, kept even while that player has no subsystem yet. */
	TMap<int32, EKzUIInputDevice> DevicesByUser;

	/** Reentrancy guard: lets our own injected key events pass through HandleKeyDown/UpEvent. */
	bool bInjecting = false;

	void TrackDevice(int32 UserIndex, const FKey& Key);

	void InjectKeyEvent(FSlateApplication& SlateApp, const FKey& Key, int32 UserIndex, bool bPress, bool bRepeat);
};
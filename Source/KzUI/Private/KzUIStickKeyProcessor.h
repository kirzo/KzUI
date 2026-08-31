// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Framework/Application/IInputProcessor.h"

/**
 * Normalizes stick-to-digital behavior across gamepad backends. Pads GameInput treats as generic
 * controllers (Shield, off-brand pads) never send the digital Gamepad_*Stick_* keys that native
 * gamepads (XInput, DualSense) do, so UI navigation silently loses the sticks on them. Instead of
 * detecting which backend does what, this processor consumes every platform-sent stick-direction
 * key and re-synthesizes them from the analog axes for all devices: single source, same threshold,
 * same hysteresis, same repeat cadence on every pad. Injected events route through the regular
 * per-user focus path, so UI consumption and game bubbling behave exactly like real keys.
 */
class FKzUIStickKeyProcessor : public IInputProcessor
{
public:
	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override;
	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override;
	virtual bool HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override;
	virtual bool HandleAnalogInputEvent(FSlateApplication& SlateApp, const FAnalogInputEvent& InAnalogInputEvent) override;

private:
	struct FAxisState
	{
		FKey Key;
		double NextRepeatTime = 0.0;
	};

	/** Active synthetic stick-direction key per (slate user, analog axis). */
	TMap<TPair<int32, FKey>, FAxisState> AxisStates;

	/** Reentrancy guard: lets our own injected key events pass through HandleKeyDown/UpEvent. */
	bool bInjecting = false;

	void InjectKeyEvent(FSlateApplication& SlateApp, const FKey& Key, int32 UserIndex, bool bPress, bool bRepeat);
};
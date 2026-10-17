// Copyright 2026 kirzo

#include "KzUITypes.h"

static TArray<FKey> ApplyOverride(TArray<FKey> Keys, const FKzUIInputDefinitionOverride& Override)
{
	switch (Override.Mode)
	{
		case EKzUIInputOverrideMode::Override:
		{
			return Override.GetKeys();
		}
		case EKzUIInputOverrideMode::Additive:
		{
			Keys.Append(Override.GetKeys());
			return Keys;
		}
		case EKzUIInputOverrideMode::Subtractive:
		{
			Keys.RemoveAll([ToRemove = Override.GetKeys()](const FKey& Key) { return ToRemove.Contains(Key); });
			return Keys;
		}
	}
	return Keys;
}

FKzUIInputDefinition FKzUIInputData::GetInputDefinition(EKzUIInputDevice GamepadDevice) const
{
	// Each override only rewrites its own device's keys, so an Override-mode pad entry never drops the keyboard
	TArray<FKey> KeyboardKeys;
	TArray<FKey> GamepadKeys;
	for (const FKey& Key : Default.GetKeys())
	{
		(Key.IsGamepadKey() ? GamepadKeys : KeyboardKeys).Add(Key);
	}

	if (bOverrideWindows)
	{
		KeyboardKeys = ApplyOverride(MoveTemp(KeyboardKeys), Windows);
	}

	const FKzUIInputDefinitionOverride* GamepadOverride = nullptr;
	switch (GamepadDevice)
	{
		case EKzUIInputDevice::Xbox:
			GamepadOverride = bOverrideXbox ? &Xbox : nullptr;
			break;
		case EKzUIInputDevice::DualShock:
		case EKzUIInputDevice::DualSense:
			GamepadOverride = bOverridePlayStation ? &PlayStation : nullptr;
			break;
		case EKzUIInputDevice::Switch:
			GamepadOverride = bOverrideSwitch ? &Switch : nullptr;
			break;
		default:
			break;
	}

	if (GamepadOverride)
	{
		GamepadKeys = ApplyOverride(MoveTemp(GamepadKeys), *GamepadOverride);
	}

	KeyboardKeys.Append(GamepadKeys);
	return FKzUIInputDefinition(KeyboardKeys);
}
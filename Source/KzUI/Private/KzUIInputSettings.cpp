// Copyright 2026 kirzo

#include "KzUIInputSettings.h"

UKzUIInputSettings::UKzUIInputSettings()
{
	// WinDualShock names, plus the GameInput family names and common GameInput id overrides
	DeviceMappings.Add("DualSense", EKzUIInputDevice::DualSense);
	DeviceMappings.Add("DualShock", EKzUIInputDevice::DualShock);
	DeviceMappings.Add("DualShock4", EKzUIInputDevice::DualShock);
	DeviceMappings.Add("XboxOne", EKzUIInputDevice::Xbox);
	DeviceMappings.Add("Xbox360", EKzUIInputDevice::Xbox);
	DeviceMappings.Add("XInputController", EKzUIInputDevice::Xbox);
	DeviceMappings.Add("KBM", EKzUIInputDevice::Keyboard);
	DeviceMappings.Add("I8042", EKzUIInputDevice::Keyboard);

	FKzUIInputData Back;
	Back.Default = FKzUIInputDefinition({ EKeys::Gamepad_FaceButton_Right });
	Back.bOverrideWindows = true;
	Back.Windows = FKzUIInputDefinitionOverride(EKzUIInputOverrideMode::Additive, { EKeys::BackSpace });
	Back.bOverrideSwitch = true;
	Back.Switch = FKzUIInputDefinitionOverride(EKzUIInputOverrideMode::Override, { EKeys::Gamepad_FaceButton_Bottom });
	InputMap.Add(EKzUIInputType::Back, Back);

	FKzUIInputData Accept;
	Accept.Default = FKzUIInputDefinition({ EKeys::Gamepad_FaceButton_Bottom });
	Accept.bOverrideWindows = true;
	Accept.Windows = FKzUIInputDefinitionOverride(EKzUIInputOverrideMode::Additive, { EKeys::SpaceBar });
	Accept.bOverrideSwitch = true;
	Accept.Switch = FKzUIInputDefinitionOverride(EKzUIInputOverrideMode::Override, { EKeys::Gamepad_FaceButton_Right });
	InputMap.Add(EKzUIInputType::Accept, Accept);

	FKzUIInputData Down;
	Down.Default = FKzUIInputDefinition({ EKeys::Gamepad_LeftStick_Down, EKeys::Gamepad_DPad_Down });
	Down.bOverrideWindows = true;
	Down.Windows = FKzUIInputDefinitionOverride(EKzUIInputOverrideMode::Additive, { EKeys::S, EKeys::Down });
	InputMap.Add(EKzUIInputType::Down, Down);

	FKzUIInputData Up;
	Up.Default = FKzUIInputDefinition({ EKeys::Gamepad_LeftStick_Up, EKeys::Gamepad_DPad_Up });
	Up.bOverrideWindows = true;
	Up.Windows = FKzUIInputDefinitionOverride(EKzUIInputOverrideMode::Additive, { EKeys::W, EKeys::Up });
	InputMap.Add(EKzUIInputType::Up, Up);

	FKzUIInputData Left;
	Left.Default = FKzUIInputDefinition({ EKeys::Gamepad_LeftStick_Left, EKeys::Gamepad_DPad_Left });
	Left.bOverrideWindows = true;
	Left.Windows = FKzUIInputDefinitionOverride(EKzUIInputOverrideMode::Additive, { EKeys::A, EKeys::Left });
	InputMap.Add(EKzUIInputType::Left, Left);

	FKzUIInputData Right;
	Right.Default = FKzUIInputDefinition({ EKeys::Gamepad_LeftStick_Right, EKeys::Gamepad_DPad_Right });
	Right.bOverrideWindows = true;
	Right.Windows = FKzUIInputDefinitionOverride(EKzUIInputOverrideMode::Additive, { EKeys::D, EKeys::Right });
	InputMap.Add(EKzUIInputType::Right, Right);

	FKzUIInputData Previous;
	Previous.Default = FKzUIInputDefinition({ EKeys::Gamepad_LeftShoulder });
	Previous.bOverrideWindows = true;
	Previous.Windows = FKzUIInputDefinitionOverride(EKzUIInputOverrideMode::Additive, { EKeys::Q });
	InputMap.Add(EKzUIInputType::Previous, Previous);

	FKzUIInputData Next;
	Next.Default = FKzUIInputDefinition({ EKeys::Gamepad_RightShoulder });
	Next.bOverrideWindows = true;
	Next.Windows = FKzUIInputDefinitionOverride(EKzUIInputOverrideMode::Additive, { EKeys::E });
	InputMap.Add(EKzUIInputType::Next, Next);

	FKzUIInputData Start;
	Start.Default = FKzUIInputDefinition({ EKeys::Gamepad_Special_Right });
	Start.bOverrideWindows = true;
	Start.Windows = FKzUIInputDefinitionOverride(EKzUIInputOverrideMode::Additive, { EKeys::Enter });
	InputMap.Add(EKzUIInputType::Start, Start);

	FKzUIInputData Select;
	Select.Default = FKzUIInputDefinition({ EKeys::Gamepad_Special_Left });
	Select.bOverrideWindows = true;
	Select.Windows = FKzUIInputDefinitionOverride(EKzUIInputOverrideMode::Additive, { EKeys::Tab });
	InputMap.Add(EKzUIInputType::Select, Select);
}
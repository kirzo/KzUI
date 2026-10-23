// Copyright 2026 kirzo

#include "KzUIInputSettings.h"

/** Keys and triggers a project starts with, also used to fill entries missing from an older saved config. */
static TMap<EKzUIInputType, FKzUIInputData> MakeDefaultInputMap()
{
	TMap<EKzUIInputType, FKzUIInputData> Map;

	FKzUIInputData Back;
	Back.Default = FKzUIInputDefinition({ EKeys::Gamepad_FaceButton_Right });
	Back.bOverrideWindows = true;
	Back.Windows = FKzUIInputDefinitionOverride(EKzUIInputOverrideMode::Additive, { EKeys::BackSpace });
	Back.bOverrideSwitch = true;
	Back.Switch = FKzUIInputDefinitionOverride(EKzUIInputOverrideMode::Override, { EKeys::Gamepad_FaceButton_Bottom });
	Map.Add(EKzUIInputType::Back, Back);

	FKzUIInputData Accept;
	Accept.Default = FKzUIInputDefinition({ EKeys::Gamepad_FaceButton_Bottom });
	Accept.bOverrideWindows = true;
	Accept.Windows = FKzUIInputDefinitionOverride(EKzUIInputOverrideMode::Additive, { EKeys::SpaceBar });
	Accept.bOverrideSwitch = true;
	Accept.Switch = FKzUIInputDefinitionOverride(EKzUIInputOverrideMode::Override, { EKeys::Gamepad_FaceButton_Right });
	Map.Add(EKzUIInputType::Accept, Accept);

	FKzUIInputData Down;
	Down.Default = FKzUIInputDefinition({ EKeys::Gamepad_LeftStick_Down, EKeys::Gamepad_DPad_Down });
	Down.bOverrideWindows = true;
	Down.Windows = FKzUIInputDefinitionOverride(EKzUIInputOverrideMode::Additive, { EKeys::S, EKeys::Down });
	Map.Add(EKzUIInputType::Down, Down);

	FKzUIInputData Up;
	Up.Default = FKzUIInputDefinition({ EKeys::Gamepad_LeftStick_Up, EKeys::Gamepad_DPad_Up });
	Up.bOverrideWindows = true;
	Up.Windows = FKzUIInputDefinitionOverride(EKzUIInputOverrideMode::Additive, { EKeys::W, EKeys::Up });
	Map.Add(EKzUIInputType::Up, Up);

	FKzUIInputData Left;
	Left.Default = FKzUIInputDefinition({ EKeys::Gamepad_LeftStick_Left, EKeys::Gamepad_DPad_Left });
	Left.bOverrideWindows = true;
	Left.Windows = FKzUIInputDefinitionOverride(EKzUIInputOverrideMode::Additive, { EKeys::A, EKeys::Left });
	Map.Add(EKzUIInputType::Left, Left);

	FKzUIInputData Right;
	Right.Default = FKzUIInputDefinition({ EKeys::Gamepad_LeftStick_Right, EKeys::Gamepad_DPad_Right });
	Right.bOverrideWindows = true;
	Right.Windows = FKzUIInputDefinitionOverride(EKzUIInputOverrideMode::Additive, { EKeys::D, EKeys::Right });
	Map.Add(EKzUIInputType::Right, Right);

	FKzUIInputData Previous;
	Previous.Default = FKzUIInputDefinition({ EKeys::Gamepad_LeftShoulder });
	Previous.bOverrideWindows = true;
	Previous.Windows = FKzUIInputDefinitionOverride(EKzUIInputOverrideMode::Additive, { EKeys::Q });
	Map.Add(EKzUIInputType::Previous, Previous);

	FKzUIInputData Next;
	Next.Default = FKzUIInputDefinition({ EKeys::Gamepad_RightShoulder });
	Next.bOverrideWindows = true;
	Next.Windows = FKzUIInputDefinitionOverride(EKzUIInputOverrideMode::Additive, { EKeys::E });
	Map.Add(EKzUIInputType::Next, Next);

	FKzUIInputData Start;
	Start.Default = FKzUIInputDefinition({ EKeys::Gamepad_Special_Right });
	Start.bOverrideWindows = true;
	Start.Windows = FKzUIInputDefinitionOverride(EKzUIInputOverrideMode::Additive, { EKeys::Enter });
	Map.Add(EKzUIInputType::Start, Start);

	FKzUIInputData Select;
	Select.Default = FKzUIInputDefinition({ EKeys::Gamepad_Special_Left });
	Select.bOverrideWindows = true;
	Select.Windows = FKzUIInputDefinitionOverride(EKzUIInputOverrideMode::Additive, { EKeys::Tab });
	Map.Add(EKzUIInputType::Select, Select);

	return Map;
}

UKzUIInputSettings::UKzUIInputSettings()
{
	// WinDualShock names, the GameInput family names and common id overrides, and SDLGamepad ids
	DeviceMappings.Add("DualSense", EKzUIInputDevice::DualSense);
	DeviceMappings.Add("DualShock", EKzUIInputDevice::DualShock);
	DeviceMappings.Add("DualShock4", EKzUIInputDevice::DualShock);
	DeviceMappings.Add("SwitchPro", EKzUIInputDevice::Switch);
	DeviceMappings.Add("XboxOne", EKzUIInputDevice::Xbox);
	DeviceMappings.Add("Xbox360", EKzUIInputDevice::Xbox);
	DeviceMappings.Add("XInputController", EKzUIInputDevice::Xbox);
	DeviceMappings.Add("KBM", EKzUIInputDevice::Keyboard);
	DeviceMappings.Add("I8042", EKzUIInputDevice::Keyboard);

	InputMap = MakeDefaultInputMap();
}

void UKzUIInputSettings::PostInitProperties()
{
	Super::PostInitProperties();
	AddMissingInputs();
}

void UKzUIInputSettings::AddMissingInputs()
{
	// A config saved before an input type existed replaces the whole map without it: that input gets its default keys instead of none
	for (auto& Pair : MakeDefaultInputMap())
	{
		if (!InputMap.Contains(Pair.Key))
		{
			InputMap.Add(Pair.Key, MoveTemp(Pair.Value));
		}
	}
}

const FKzUIInputDefinition* UKzUIInputSettings::FindInputDefinition(EKzUIInputType Input, EKzUIInputDevice GamepadDevice) const
{
	TMap<EKzUIInputType, FKzUIInputDefinition>* Resolved = ResolvedInputs.Find(GamepadDevice);
	if (!Resolved)
	{
		Resolved = &ResolvedInputs.Add(GamepadDevice);
		for (const auto& Pair : InputMap)
		{
			Resolved->Add(Pair.Key, Pair.Value.GetInputDefinition(GamepadDevice));
		}
	}
	return Resolved->Find(Input);
}

void UKzUIInputSettings::PostReloadConfig(FProperty* PropertyThatWasLoaded)
{
	Super::PostReloadConfig(PropertyThatWasLoaded);
	AddMissingInputs();
	ResolvedInputs.Reset();
}

#if WITH_EDITOR
void UKzUIInputSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	ResolvedInputs.Reset();
}
#endif
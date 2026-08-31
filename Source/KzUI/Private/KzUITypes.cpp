// Copyright 2026 kirzo

#include "KzUITypes.h"
#include "HAL/PlatformProperties.h"

static FKzUIInputDefinition ApplyOverride(const FKzUIInputDefinition& Base, const FKzUIInputDefinitionOverride& Override)
{
	switch (Override.Mode)
	{
		case EKzUIInputOverrideMode::Override:
		{
			return Override;
		}
		case EKzUIInputOverrideMode::Additive:
		{
			TArray<FKey> Keys = Base.GetKeys();
			Keys.Append(Override.GetKeys());
			return FKzUIInputDefinition(Keys);
		}
		case EKzUIInputOverrideMode::Subtractive:
		{
			TArray<FKey> Keys = Base.GetKeys();
			Keys.RemoveAll([ToRemove = Override.GetKeys()](const FKey& Key) { return ToRemove.Contains(Key); });
			return FKzUIInputDefinition(Keys);
		}
	}
	return Base;
}

FKzUIInputDefinition FKzUIInputData::GetInputDefinition() const
{
	const FString Platform = FPlatformProperties::IniPlatformName();
	if (Platform == TEXT("PS4") || Platform == TEXT("PS5"))
	{
		return bOverridePlayStation ? ApplyOverride(Default, PlayStation) : Default;
	}
	if (Platform.Contains(TEXT("Xbox")) || Platform == TEXT("XSX"))
	{
		return bOverrideXbox ? ApplyOverride(Default, Xbox) : Default;
	}
	if (Platform == TEXT("Switch"))
	{
		return bOverrideSwitch ? ApplyOverride(Default, Switch) : Default;
	}
	// Any desktop platform uses the Windows override
	return bOverrideWindows ? ApplyOverride(Default, Windows) : Default;
}
// Copyright 2026 kirzo

#include "KzUIIconTheme.h"
#include "KzUIInputSettings.h"
#include "Misc/DataValidation.h"

#define LOCTEXT_NAMESPACE "KzUI"

#if WITH_EDITOR
EDataValidationResult UKzUIIconTheme::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	// A warning only: a game may never show some inputs, and KzTextBlock falls back to the key name
	for (EKzUIInputType Input : TEnumRange<EKzUIInputType>())
	{
		const FKzUIIcon* Icon = InputIcons.Find(Input);
		if (Icon && Icon->Default)
		{
			continue;
		}

		const FKzUIInputDefinition* Definition = UKzUIInputSettings::Get()->FindInputDefinition(Input, EKzUIInputDevice::Xbox);
		const TArray<FKey> Keys = Definition ? Definition->GetKeys() : TArray<FKey>();
		if (!Keys.ContainsByPredicate([this](const FKey& Key) { return KeyIcons.Contains(Key); }))
		{
			Context.AddWarning(FText::Format(LOCTEXT("ThemeMissingIcon", "{0} has no default icon and none of its keys has one: devices without their own override show the key name."), UEnum::GetDisplayValueAsText(Input)));
		}
	}

	for (const TPair<FKey, FKzUIIcon>& Pair : KeyIcons)
	{
		if (!Pair.Key.IsValid())
		{
			Context.AddWarning(FText::Format(LOCTEXT("ThemeInvalidKey", "Key icon '{0}' is not a valid key: nothing ever shows it."), FText::FromName(Pair.Key.GetFName())));
		}
	}

	for (const TPair<FName, FKzUIIcon>& Pair : CustomIcons)
	{
		if (Pair.Key.ToString().Contains(TEXT(":")))
		{
			Context.AddError(FText::Format(LOCTEXT("ThemeTokenColon", "Custom icon '{0}' contains ':', which `{Token:Variation`} reserves: KzTextBlock cannot address it."), FText::FromName(Pair.Key)));
			Result = EDataValidationResult::Invalid;
		}
	}
	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
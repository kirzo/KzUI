// Copyright 2026 kirzo

#include "Factories/KzUISoundThemeFactory.h"
#include "KzUISoundTheme.h"

UKzUISoundThemeFactory::UKzUISoundThemeFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UKzUISoundTheme::StaticClass();
}

UObject* UKzUISoundThemeFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	// One entry per input, so the theme starts as a list to fill in
	UKzUISoundTheme* Theme = NewObject<UKzUISoundTheme>(InParent, Class, Name, Flags | RF_Transactional);
	for (EKzUIInputType Input : TEnumRange<EKzUIInputType>())
	{
		Theme->InputSounds.Add(Input, nullptr);
	}
	return Theme;
}
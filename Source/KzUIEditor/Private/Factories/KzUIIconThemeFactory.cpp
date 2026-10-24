// Copyright 2026 kirzo

#include "Factories/KzUIIconThemeFactory.h"
#include "KzUIIconTheme.h"

UKzUIIconThemeFactory::UKzUIIconThemeFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UKzUIIconTheme::StaticClass();
}

UObject* UKzUIIconThemeFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	// One entry per input, so the theme starts as a list to fill in
	UKzUIIconTheme* Theme = NewObject<UKzUIIconTheme>(InParent, Class, Name, Flags | RF_Transactional);
	for (EKzUIInputType Input : TEnumRange<EKzUIInputType>())
	{
		Theme->InputIcons.Add(Input, FKzUIIcon());
	}
	return Theme;
}
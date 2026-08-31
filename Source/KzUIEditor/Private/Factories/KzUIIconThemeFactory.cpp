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
	return NewObject<UKzUIIconTheme>(InParent, Class, Name, Flags | RF_Transactional);
}
// Copyright 2026 kirzo

#include "Factories/KzUIFlipbookFactory.h"
#include "KzUIFlipbook.h"

UKzUIFlipbookFactory::UKzUIFlipbookFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UKzUIFlipbook::StaticClass();
}

UObject* UKzUIFlipbookFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UKzUIFlipbook>(InParent, Class, Name, Flags | RF_Transactional);
}
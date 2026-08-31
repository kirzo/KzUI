// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "KzUIFlipbookFactory.generated.h"

UCLASS()
class UKzUIFlipbookFactory : public UFactory
{
	GENERATED_BODY()

public:
	UKzUIFlipbookFactory();

	// UFactory interface
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	// End of UFactory interface
};
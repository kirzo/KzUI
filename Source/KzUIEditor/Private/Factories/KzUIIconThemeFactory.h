// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "KzUIIconThemeFactory.generated.h"

UCLASS()
class UKzUIIconThemeFactory : public UFactory
{
	GENERATED_BODY()

public:
	UKzUIIconThemeFactory();

	// UFactory interface
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	// End of UFactory interface
};
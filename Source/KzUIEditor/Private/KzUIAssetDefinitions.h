// Copyright 2026 kirzo

#pragma once

#include "AssetDefinitionDefault.h"
#include "KzUIFlipbook.h"
#include "KzUIIconTheme.h"
#include "KzUISoundTheme.h"
#include "KzUIAssetDefinitions.generated.h"

/** KzUI assets sit in the User Interface category of the Content Browser, next to the Widget Blueprint. */
UCLASS(Abstract)
class UAssetDefinition_KzUI : public UAssetDefinitionDefault
{
	GENERATED_BODY()

public:
	virtual FLinearColor GetAssetColor() const override { return FLinearColor(FColor(64, 201, 255)); }
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override
	{
		static const auto Categories = { EAssetCategoryPaths::UI };
		return Categories;
	}
};

UCLASS()
class UAssetDefinition_KzUISoundTheme : public UAssetDefinition_KzUI
{
	GENERATED_BODY()

public:
	virtual FText GetAssetDisplayName() const override { return INVTEXT("UI Sound Theme"); }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UKzUISoundTheme::StaticClass(); }
};

UCLASS()
class UAssetDefinition_KzUIIconTheme : public UAssetDefinition_KzUI
{
	GENERATED_BODY()

public:
	virtual FText GetAssetDisplayName() const override { return INVTEXT("UI Icon Theme"); }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UKzUIIconTheme::StaticClass(); }
};

UCLASS()
class UAssetDefinition_KzUIFlipbook : public UAssetDefinition_KzUI
{
	GENERATED_BODY()

public:
	virtual FText GetAssetDisplayName() const override { return INVTEXT("UI Flipbook"); }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UKzUIFlipbook::StaticClass(); }
};
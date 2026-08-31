// Copyright 2026 kirzo

#pragma once

#include "Engine/DataAsset.h"
#include "KzUITypes.h"
#include "KzUIIconTheme.generated.h"

class UMaterialInterface;

USTRUCT(BlueprintType)
struct KZUI_API FKzUIIcon
{
	GENERATED_BODY()

	UPROPERTY(Category = Icon, EditAnywhere, BlueprintReadOnly, meta = (AllowedClasses = "/Script/Engine.Texture2D,/Script/Engine.MaterialInterface,/Script/KzUI.KzUIFlipbook"))
	TObjectPtr<UObject> Default;

	/** Per-device replacements for Default. */
	UPROPERTY(Category = Icon, EditAnywhere, BlueprintReadOnly, meta = (AllowedClasses = "/Script/Engine.Texture2D,/Script/Engine.MaterialInterface,/Script/KzUI.KzUIFlipbook"))
	TMap<EKzUIInputDevice, TObjectPtr<UObject>> Overrides;

	UObject* Resolve(EKzUIInputDevice Device) const
	{
		UObject* Icon = Overrides.FindRef(Device);
		return Icon ? Icon : Default.Get();
	}
};

/** Icon set of the game (textures, materials or flipbooks), resolved per device: button prompts per input plus project-defined custom icons addressable as {TokenName}. The default theme is set in project settings; a per-player theme can be swapped at runtime through UKzUIInputSubsystem. */
UCLASS(BlueprintType)
class KZUI_API UKzUIIconTheme : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = Icons, EditAnywhere)
	TMap<EKzUIInputType, FKzUIIcon> InputIcons;

	/** Project-defined icons, resolvable in a KzTextBlock as {TokenName}. Names must not contain ':', reserved for the {Token:Variation} syntax. */
	UPROPERTY(Category = Icons, EditAnywhere)
	TMap<FName, FKzUIIcon> CustomIcons;

	/** Materials wrapping an icon texture for alternate presentations, addressable as {Token:Variation}. The device-resolved texture is injected into VariationTextureParameter, so one material serves every device. */
	UPROPERTY(Category = Variations, EditAnywhere)
	TMap<FName, TObjectPtr<UMaterialInterface>> IconVariations;

	/** Texture parameter of the variation materials that receives the resolved icon. */
	UPROPERTY(Category = Variations, EditAnywhere)
	FName VariationTextureParameter = TEXT("Icon");

	UFUNCTION(Category = "KzUI", BlueprintPure)
	UObject* GetIcon(EKzUIInputType Input, EKzUIInputDevice Device) const
	{
		const FKzUIIcon* Icon = InputIcons.Find(Input);
		return Icon ? Icon->Resolve(Device) : nullptr;
	}

	UFUNCTION(Category = "KzUI", BlueprintPure)
	UObject* GetCustomIcon(FName Token, EKzUIInputDevice Device) const
	{
		const FKzUIIcon* Icon = CustomIcons.Find(Token);
		return Icon ? Icon->Resolve(Device) : nullptr;
	}

	bool HasCustomIcon(FName Token) const { return CustomIcons.Contains(Token); }

	UFUNCTION(Category = "KzUI", BlueprintPure)
	UMaterialInterface* GetVariation(FName Variation) const { return IconVariations.FindRef(Variation); }
};
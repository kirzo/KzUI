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

	/** Per-device replacements for Default. A DualSense without its own entry uses the DualShock one, and the other way round. */
	UPROPERTY(Category = Icon, EditAnywhere, BlueprintReadOnly, meta = (AllowedClasses = "/Script/Engine.Texture2D,/Script/Engine.MaterialInterface,/Script/KzUI.KzUIFlipbook"))
	TMap<EKzUIInputDevice, TObjectPtr<UObject>> Overrides;

	UObject* Resolve(EKzUIInputDevice Device) const
	{
		if (UObject* Icon = Overrides.FindRef(Device))
		{
			return Icon;
		}

		// Both PlayStation pads have the same face buttons, so one's icons stand in for the other's
		const EKzUIInputDevice Sibling = Device == EKzUIInputDevice::DualSense ? EKzUIInputDevice::DualShock : Device == EKzUIInputDevice::DualShock ? EKzUIInputDevice::DualSense : Device;
		UObject* Icon = Sibling != Device ? Overrides.FindRef(Sibling) : nullptr;
		return Icon ? Icon : Default.Get();
	}
};

/** Icon set of the game (textures, materials or flipbooks), resolved per device: button prompts per input plus project-defined custom icons addressable as {TokenName}. The default theme is set in project settings; a per-player theme can be swapped at runtime through UKzUIInputSubsystem. */
UCLASS(BlueprintType)
class KZUI_API UKzUIIconTheme : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Icons of the UI inputs. An input without one here shows the icon of its key. */
	UPROPERTY(Category = Icons, EditAnywhere)
	TMap<EKzUIInputType, FKzUIIcon> InputIcons;

	/** Icons of physical keys, shown for UI inputs without their own icon and for the keys behind Enhanced Input actions. A key without one shows its name. */
	UPROPERTY(Category = Icons, EditAnywhere)
	TMap<FKey, FKzUIIcon> KeyIcons;

	/** Project-defined icons, resolvable in a KzTextBlock as {TokenName}. Names must not contain ':', reserved for the {Token:Variation} syntax. */
	UPROPERTY(Category = Icons, EditAnywhere)
	TMap<FName, FKzUIIcon> CustomIcons;

	/** Materials wrapping an icon texture for alternate presentations, addressable as {Token:Variation}. The device-resolved texture is injected into VariationTextureParameter, so one material serves every device. */
	UPROPERTY(Category = Variations, EditAnywhere)
	TMap<FName, TObjectPtr<UMaterialInterface>> IconVariations;

	/** Texture parameter of the variation materials that receives the resolved icon. */
	UPROPERTY(Category = Variations, EditAnywhere)
	FName VariationTextureParameter = TEXT("Icon");

	/** Scalar parameter of the icon materials that a KzInputIcon with bShowHoldProgress drives with the hold progress. */
	UPROPERTY(Category = Variations, EditAnywhere)
	FName HoldProgressParameter = TEXT("Progress");

	UFUNCTION(Category = "KzUI", BlueprintPure)
	UObject* GetIcon(EKzUIInputType Input, EKzUIInputDevice Device) const
	{
		const FKzUIIcon* Icon = InputIcons.Find(Input);
		return Icon ? Icon->Resolve(Device) : nullptr;
	}

	UFUNCTION(Category = "KzUI", BlueprintPure)
	UObject* GetKeyIcon(FKey Key, EKzUIInputDevice Device) const
	{
		const FKzUIIcon* Icon = KeyIcons.Find(Key);
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

#if WITH_EDITOR
	/** Warns about inputs without an icon of their own or of their keys and about invalid keys, and rejects custom icon names with ':'. */
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
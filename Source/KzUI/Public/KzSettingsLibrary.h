// Copyright 2026 kirzo

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "KzSettingsLibrary.generated.h"

/** Engine settings the option widgets can bind to directly. Resolution, WindowMode and Quality are index-shaped (spinner), VSync is bool-shaped (toggle), ResolutionScale is value-shaped (slider). */
UENUM(BlueprintType)
enum class EKzUIStandardSetting : uint8
{
	None,
	Resolution,
	WindowMode,
	Quality,
	VSync,
	ResolutionScale
};

/** Scalability group a Quality setting drives. */
UENUM(BlueprintType)
enum class EKzUIQualitySetting : uint8
{
	Overall,
	ViewDistance,
	AntiAliasing,
	PostProcessing,
	Shadows,
	GlobalIllumination,
	Reflections,
	Textures,
	Effects,
	Foliage,
	Shading
};

/** Stateless wrappers over UGameUserSettings, shaped for the KzUI option widgets. */
UCLASS()
class KZUI_API UKzSettingsLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Display labels for an index-shaped setting, in index order. */
	UFUNCTION(Category = "KzUI|Settings", BlueprintPure, meta = (AdvancedDisplay = "Quality"))
	static TArray<FText> GetSettingOptions(EKzUIStandardSetting Setting, EKzUIQualitySetting Quality = EKzUIQualitySetting::Overall);

	/** Current index of an index-shaped setting; -1 when the current value is not in the options (mixed quality levels). */
	UFUNCTION(Category = "KzUI|Settings", BlueprintPure, meta = (AdvancedDisplay = "Quality"))
	static int32 GetSettingIndex(EKzUIStandardSetting Setting, EKzUIQualitySetting Quality = EKzUIQualitySetting::Overall);

	UFUNCTION(Category = "KzUI|Settings", BlueprintCallable, meta = (AdvancedDisplay = "Quality"))
	static void SetSettingIndex(EKzUIStandardSetting Setting, int32 Index, bool bApply = true, EKzUIQualitySetting Quality = EKzUIQualitySetting::Overall);

	UFUNCTION(Category = "KzUI|Settings", BlueprintPure)
	static bool GetSettingEnabled(EKzUIStandardSetting Setting);

	UFUNCTION(Category = "KzUI|Settings", BlueprintCallable)
	static void SetSettingEnabled(EKzUIStandardSetting Setting, bool bEnabled, bool bApply = true);

	/** Normalized 0 to 1 value of a value-shaped setting. */
	UFUNCTION(Category = "KzUI|Settings", BlueprintPure)
	static float GetSettingValue(EKzUIStandardSetting Setting);

	UFUNCTION(Category = "KzUI|Settings", BlueprintCallable)
	static void SetSettingValue(EKzUIStandardSetting Setting, float Value, bool bApply = true);

	/** Applies and saves the pending GameUserSettings changes. */
	UFUNCTION(Category = "KzUI|Settings", BlueprintCallable)
	static void ApplySettings();
};
// Copyright 2026 kirzo

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Scalability.h"
#include "KzSettingsLibrary.generated.h"

/**
 * Engine settings the option widgets can bind to directly. Resolution, WindowMode, Quality, FrameRateLimit and Language are
 * index-shaped (spinner), VSync and HDR are bool-shaped (toggle, or a two-option spinner), ResolutionScale and Gamma are
 * value-shaped (slider). Language and Gamma take effect as they change and are saved by ApplySettings, with the rest.
 */
UENUM(BlueprintType)
enum class EKzUIStandardSetting : uint8
{
	None,
	Resolution,
	WindowMode,
	Quality,
	VSync,
	ResolutionScale,
	FrameRateLimit,
	HDR,
	Language,
	Gamma
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

/** Engine values an index-shaped setting stands for when it sits outside its options: mixed quality levels, a resolution not in the list. */
struct FKzUISettingState
{
	FIntPoint Resolution = FIntPoint::ZeroValue;
	Scalability::FQualityLevels Quality;
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

	/** Whether this system can use the setting at all: HDR needs a display that supports it. */
	UFUNCTION(Category = "KzUI|Settings", BlueprintPure)
	static bool IsSettingSupported(EKzUIStandardSetting Setting);

	/** Runs the engine's hardware benchmark and sets the quality levels it recommends, applying them on request. */
	UFUNCTION(Category = "KzUI|Settings", BlueprintCallable)
	static void AutoDetectQuality(bool bApply = true);

	/** Restores the Gamma a previous session saved. Called by the module once the engine is up, outside the editor. */
	static void ApplySavedGamma();

	/** Whether the resolution or window mode in use is not the last one the player confirmed. */
	UFUNCTION(Category = "KzUI|Settings", BlueprintPure)
	static bool IsVideoModeUnconfirmed();

	/** Keeps the current resolution and window mode as the ones RevertVideoMode returns to. */
	UFUNCTION(Category = "KzUI|Settings", BlueprintCallable)
	static void ConfirmVideoMode();

	/** Goes back to the last confirmed resolution and window mode. */
	UFUNCTION(Category = "KzUI|Settings", BlueprintCallable)
	static void RevertVideoMode(bool bApply = true);

	/** Broadcast after any setting changes through this library, so option rows can show what other rows changed (the quality groups after the overall level, for instance). */
	static FSimpleMulticastDelegate& OnSettingChanged();

	static FKzUISettingState CaptureSettingState();

	/** Puts back what CaptureSettingState read for Resolution or Quality. Quality restores every group, since Overall is the one that sits outside its options. */
	static void RestoreSettingState(EKzUIStandardSetting Setting, const FKzUISettingState& State, bool bApply = true);

private:
	/** Applies when asked and broadcasts OnSettingChanged. */
	static void NotifyChanged(bool bApply);
};
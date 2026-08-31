// Copyright 2026 kirzo

#include "KzSettingsLibrary.h"

#include "GameFramework/GameUserSettings.h"
#include "Kismet/KismetSystemLibrary.h"

#define LOCTEXT_NAMESPACE "KzUI"

static TArray<FIntPoint> GetSupportedResolutions()
{
	TArray<FIntPoint> Resolutions;
	UKismetSystemLibrary::GetSupportedFullscreenResolutions(Resolutions);
	return Resolutions;
}

static int32 GetQualityLevel(const UGameUserSettings* Settings, EKzUIQualitySetting Quality)
{
	switch (Quality)
	{
		case EKzUIQualitySetting::ViewDistance:			return Settings->GetViewDistanceQuality();
		case EKzUIQualitySetting::AntiAliasing:			return Settings->GetAntiAliasingQuality();
		case EKzUIQualitySetting::PostProcessing:		return Settings->GetPostProcessingQuality();
		case EKzUIQualitySetting::Shadows:				return Settings->GetShadowQuality();
		case EKzUIQualitySetting::GlobalIllumination:	return Settings->GetGlobalIlluminationQuality();
		case EKzUIQualitySetting::Reflections:			return Settings->GetReflectionQuality();
		case EKzUIQualitySetting::Textures:				return Settings->GetTextureQuality();
		case EKzUIQualitySetting::Effects:				return Settings->GetVisualEffectQuality();
		case EKzUIQualitySetting::Foliage:				return Settings->GetFoliageQuality();
		case EKzUIQualitySetting::Shading:				return Settings->GetShadingQuality();
		default:										return Settings->GetOverallScalabilityLevel();
	}
}

static void SetQualityLevel(UGameUserSettings* Settings, EKzUIQualitySetting Quality, int32 Level)
{
	switch (Quality)
	{
		case EKzUIQualitySetting::ViewDistance:			Settings->SetViewDistanceQuality(Level); break;
		case EKzUIQualitySetting::AntiAliasing:			Settings->SetAntiAliasingQuality(Level); break;
		case EKzUIQualitySetting::PostProcessing:		Settings->SetPostProcessingQuality(Level); break;
		case EKzUIQualitySetting::Shadows:				Settings->SetShadowQuality(Level); break;
		case EKzUIQualitySetting::GlobalIllumination:	Settings->SetGlobalIlluminationQuality(Level); break;
		case EKzUIQualitySetting::Reflections:			Settings->SetReflectionQuality(Level); break;
		case EKzUIQualitySetting::Textures:				Settings->SetTextureQuality(Level); break;
		case EKzUIQualitySetting::Effects:				Settings->SetVisualEffectQuality(Level); break;
		case EKzUIQualitySetting::Foliage:				Settings->SetFoliageQuality(Level); break;
		case EKzUIQualitySetting::Shading:				Settings->SetShadingQuality(Level); break;
		default:										Settings->SetOverallScalabilityLevel(Level); break;
	}
}

TArray<FText> UKzSettingsLibrary::GetSettingOptions(EKzUIStandardSetting Setting, EKzUIQualitySetting Quality)
{
	TArray<FText> Options;
	switch (Setting)
	{
		case EKzUIStandardSetting::Resolution:
		{
			for (const FIntPoint& Resolution : GetSupportedResolutions())
			{
				Options.Add(FText::FromString(FString::Printf(TEXT("%d x %d"), Resolution.X, Resolution.Y)));
			}
			break;
		}
		case EKzUIStandardSetting::WindowMode:
		{
			Options.Add(LOCTEXT("Fullscreen", "Fullscreen"));
			Options.Add(LOCTEXT("Borderless", "Borderless"));
			Options.Add(LOCTEXT("Windowed", "Windowed"));
			break;
		}
		case EKzUIStandardSetting::Quality:
		{
			Options.Add(LOCTEXT("QualityLow", "Low"));
			Options.Add(LOCTEXT("QualityMedium", "Medium"));
			Options.Add(LOCTEXT("QualityHigh", "High"));
			Options.Add(LOCTEXT("QualityEpic", "Epic"));
			Options.Add(LOCTEXT("QualityCinematic", "Cinematic"));
			break;
		}
		case EKzUIStandardSetting::VSync:
		{
			// Bool settings also work as a two-option spinner
			Options.Add(LOCTEXT("ToggleOff", "Off"));
			Options.Add(LOCTEXT("ToggleOn", "On"));
			break;
		}
		default:
			break;
	}
	return Options;
}

int32 UKzSettingsLibrary::GetSettingIndex(EKzUIStandardSetting Setting, EKzUIQualitySetting Quality)
{
	const UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	switch (Setting)
	{
		case EKzUIStandardSetting::Resolution:
			return GetSupportedResolutions().IndexOfByKey(Settings->GetScreenResolution());
		case EKzUIStandardSetting::WindowMode:
			return static_cast<int32>(Settings->GetFullscreenMode());
		case EKzUIStandardSetting::Quality:
			return GetQualityLevel(Settings, Quality);
		case EKzUIStandardSetting::VSync:
			return Settings->IsVSyncEnabled() ? 1 : 0;
		default:
			return INDEX_NONE;
	}
}

void UKzSettingsLibrary::SetSettingIndex(EKzUIStandardSetting Setting, int32 Index, bool bApply, EKzUIQualitySetting Quality)
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	switch (Setting)
	{
		case EKzUIStandardSetting::Resolution:
		{
			const TArray<FIntPoint> Resolutions = GetSupportedResolutions();
			if (Resolutions.IsValidIndex(Index))
			{
				Settings->SetScreenResolution(Resolutions[Index]);
			}
			break;
		}
		case EKzUIStandardSetting::WindowMode:
			Settings->SetFullscreenMode(static_cast<EWindowMode::Type>(FMath::Clamp(Index, 0, 2)));
			break;
		case EKzUIStandardSetting::Quality:
			SetQualityLevel(Settings, Quality, FMath::Clamp(Index, 0, 4));
			break;
		case EKzUIStandardSetting::VSync:
			Settings->SetVSyncEnabled(Index == 1);
			break;
		default:
			return;
	}

	if (bApply)
	{
		ApplySettings();
	}
}

bool UKzSettingsLibrary::GetSettingEnabled(EKzUIStandardSetting Setting)
{
	return Setting == EKzUIStandardSetting::VSync && UGameUserSettings::GetGameUserSettings()->IsVSyncEnabled();
}

void UKzSettingsLibrary::SetSettingEnabled(EKzUIStandardSetting Setting, bool bEnabled, bool bApply)
{
	if (Setting != EKzUIStandardSetting::VSync)
	{
		return;
	}

	UGameUserSettings::GetGameUserSettings()->SetVSyncEnabled(bEnabled);
	if (bApply)
	{
		ApplySettings();
	}
}

float UKzSettingsLibrary::GetSettingValue(EKzUIStandardSetting Setting)
{
	return Setting == EKzUIStandardSetting::ResolutionScale ? UGameUserSettings::GetGameUserSettings()->GetResolutionScaleNormalized() : 0.0f;
}

void UKzSettingsLibrary::SetSettingValue(EKzUIStandardSetting Setting, float Value, bool bApply)
{
	if (Setting != EKzUIStandardSetting::ResolutionScale)
	{
		return;
	}

	UGameUserSettings::GetGameUserSettings()->SetResolutionScaleNormalized(FMath::Clamp(Value, 0.0f, 1.0f));
	if (bApply)
	{
		ApplySettings();
	}
}

void UKzSettingsLibrary::ApplySettings()
{
	UGameUserSettings::GetGameUserSettings()->ApplySettings(false);
}

#undef LOCTEXT_NAMESPACE
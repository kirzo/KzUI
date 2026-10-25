// Copyright 2026 kirzo

#include "KzSettingsLibrary.h"
#include "KzUIInputSettings.h"

#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/TextLocalizationManager.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetInternationalizationLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/ConfigCacheIni.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"
#include "Sound/SoundSubmix.h"

#define LOCTEXT_NAMESPACE "KzUI"

namespace
{
	/** Limits offered by FrameRateLimit, 0 standing for none. */
	const float FrameRateLimits[] = { 30.0f, 60.0f, 120.0f, 144.0f, 240.0f, 0.0f };
	constexpr int32 NumFrameRateLimits = UE_ARRAY_COUNT(FrameRateLimits);

	/** Range the Gamma slider covers, around the engine's 2.2. */
	constexpr float MinGamma = 1.5f;
	constexpr float MaxGamma = 3.0f;

	/** GameUserSettings has no gamma or volumes, so they live in its ini under this section. */
	const TCHAR* SavedSection = TEXT("/Script/KzUI.KzSettingsLibrary");
	const TCHAR* GammaKey = TEXT("DisplayGamma");

	/** Language, Gamma and the volumes take effect right away; these mark them for saving on ApplySettings. */
	bool bLanguagePending = false;
	bool bGammaPending = false;
	TSet<FName> PendingVolumes;

	FString GetVolumeKey(FName Channel)
	{
		return TEXT("Volume.") + Channel.ToString();
	}

	/** Each channel's volume, read the first time from the ini or the channel's default. */
	float& GetVolumeRef(FName Channel)
	{
		static TMap<FName, float> Volumes;
		if (float* Volume = Volumes.Find(Channel))
		{
			return *Volume;
		}

		const FKzUIVolumeChannel* Config = UKzUIInputSettings::Get()->VolumeChannels.Find(Channel);
		float Volume = Config ? Config->DefaultVolume : 1.0f;
		GConfig->GetFloat(SavedSection, *GetVolumeKey(Channel), Volume, GGameUserSettingsIni);
		return Volumes.Add(Channel, FMath::Clamp(Volume, 0.0f, 1.0f));
	}

	/** Sends one channel's volume to its target, or to the project's applier without one. Sound Class volumes need VolumeSoundMix pushed. */
	void SendVolume(const UObject* WorldContextObject, FName Channel, float Volume)
	{
		const UKzUIInputSettings* Settings = UKzUIInputSettings::Get();
		const FKzUIVolumeChannel* Config = Settings->VolumeChannels.Find(Channel);
		UObject* Target = Config ? Config->Target.LoadSynchronous() : nullptr;

		if (USoundClass* SoundClass = Cast<USoundClass>(Target))
		{
			if (USoundMix* Mix = Settings->VolumeSoundMix.LoadSynchronous())
			{
				UGameplayStatics::SetSoundMixClassOverride(WorldContextObject, Mix, SoundClass, Volume, 1.0f, 0.0f, true);
			}
		}
		else if (USoundSubmix* Submix = Cast<USoundSubmix>(Target))
		{
			Submix->SetSubmixOutputVolume(WorldContextObject, Volume);
		}
		else if (UClass* ApplierClass = Settings->VolumeApplier.LoadSynchronous())
		{
			ApplierClass->GetDefaultObject<UKzUIVolumeApplier>()->ApplyVolume(WorldContextObject, Channel, Volume);
		}
	}
}

/** Cultures with game localization as GetSettingOptions last listed them, so indices match the options a spinner shows. */
static const TArray<FString>& GetGameCultures(bool bRefresh = false)
{
	static TArray<FString> Cultures;
	if (bRefresh || Cultures.IsEmpty())
	{
		Cultures = FTextLocalizationManager::Get().GetLocalizedCultureNames(ELocalizationLoadFlags::Game);
		Cultures.Sort();
	}
	return Cultures;
}

/** The list GetSettingOptions last enumerated: indices keep matching the options a spinner shows, and stepping does not query the RHI each time. */
static const TArray<FIntPoint>& GetSupportedResolutions(bool bRefresh = false)
{
	static TArray<FIntPoint> Resolutions;
	if (bRefresh || Resolutions.IsEmpty())
	{
		Resolutions.Reset();
		UKismetSystemLibrary::GetSupportedFullscreenResolutions(Resolutions);
	}
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
			for (const FIntPoint& Resolution : GetSupportedResolutions(true))
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
		case EKzUIStandardSetting::HDR:
		{
			// Bool settings also work as a two-option spinner
			Options.Add(LOCTEXT("ToggleOff", "Off"));
			Options.Add(LOCTEXT("ToggleOn", "On"));
			break;
		}
		case EKzUIStandardSetting::FrameRateLimit:
		{
			for (const float Limit : FrameRateLimits)
			{
				Options.Add(Limit > 0.0f ? FText::AsNumber(FMath::RoundToInt(Limit)) : LOCTEXT("FrameRateUnlimited", "Unlimited"));
			}
			break;
		}
		case EKzUIStandardSetting::Language:
		{
			// Each language in its own name, which reads the same whatever language is active
			for (const FString& Name : GetGameCultures(true))
			{
				const FCulturePtr Culture = FInternationalization::Get().GetCulture(Name);
				Options.Add(FText::FromString(Culture ? Culture->GetNativeName() : Name));
			}
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
		case EKzUIStandardSetting::HDR:
			return Settings->IsHDREnabled() ? 1 : 0;
		case EKzUIStandardSetting::FrameRateLimit:
		{
			for (int32 Index = 0; Index < NumFrameRateLimits; ++Index)
			{
				if (FMath::IsNearlyEqual(FrameRateLimits[Index], Settings->GetFrameRateLimit()))
				{
					return Index;
				}
			}
			return INDEX_NONE;
		}
		case EKzUIStandardSetting::Language:
		{
			// The active culture may be more specific than the localized ones (es-ES for es)
			for (const FString& Name : FInternationalization::Get().GetCurrentLanguage()->GetPrioritizedParentCultureNames())
			{
				const int32 Index = GetGameCultures().IndexOfByKey(Name);
				if (Index != INDEX_NONE)
				{
					return Index;
				}
			}
			return INDEX_NONE;
		}
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
			const TArray<FIntPoint>& Resolutions = GetSupportedResolutions();
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
		case EKzUIStandardSetting::HDR:
			Settings->EnableHDRDisplayOutput(Index == 1);
			break;
		case EKzUIStandardSetting::FrameRateLimit:
			Settings->SetFrameRateLimit(FrameRateLimits[FMath::Clamp(Index, 0, NumFrameRateLimits - 1)]);
			break;
		case EKzUIStandardSetting::Language:
		{
			const TArray<FString>& Cultures = GetGameCultures();
			if (Cultures.IsValidIndex(Index) && UKismetInternationalizationLibrary::SetCurrentCulture(Cultures[Index], false))
			{
				bLanguagePending = true;
			}
			break;
		}
		default:
			return;
	}

	NotifyChanged(bApply);
}

bool UKzSettingsLibrary::GetSettingEnabled(EKzUIStandardSetting Setting)
{
	return GetSettingIndex(Setting) == 1 && (Setting == EKzUIStandardSetting::VSync || Setting == EKzUIStandardSetting::HDR);
}

void UKzSettingsLibrary::SetSettingEnabled(EKzUIStandardSetting Setting, bool bEnabled, bool bApply)
{
	if (Setting == EKzUIStandardSetting::VSync || Setting == EKzUIStandardSetting::HDR)
	{
		SetSettingIndex(Setting, bEnabled ? 1 : 0, bApply);
	}
}

float UKzSettingsLibrary::GetSettingValue(EKzUIStandardSetting Setting)
{
	switch (Setting)
	{
		case EKzUIStandardSetting::ResolutionScale:
			return UGameUserSettings::GetGameUserSettings()->GetResolutionScaleNormalized();
		case EKzUIStandardSetting::Gamma:
			return GEngine ? FMath::Clamp(FMath::GetRangePct(MinGamma, MaxGamma, GEngine->DisplayGamma), 0.0f, 1.0f) : 0.0f;
		default:
			return 0.0f;
	}
}

void UKzSettingsLibrary::SetSettingValue(EKzUIStandardSetting Setting, float Value, bool bApply)
{
	Value = FMath::Clamp(Value, 0.0f, 1.0f);
	switch (Setting)
	{
		case EKzUIStandardSetting::ResolutionScale:
			UGameUserSettings::GetGameUserSettings()->SetResolutionScaleNormalized(Value);
			break;
		case EKzUIStandardSetting::Gamma:
			if (!GEngine)
			{
				return;
			}
			GEngine->DisplayGamma = FMath::Lerp(MinGamma, MaxGamma, Value);
			bGammaPending = true;
			break;
		default:
			return;
	}

	NotifyChanged(bApply);
}

void UKzSettingsLibrary::ApplySettings()
{
	// Written before GameUserSettings saves, which flushes the same ini
	if (bLanguagePending)
	{
		UKismetInternationalizationLibrary::SetCurrentCulture(FInternationalization::Get().GetCurrentCulture()->GetName(), true);
		bLanguagePending = false;
	}
	if (bGammaPending && GEngine && GConfig)
	{
		GConfig->SetFloat(SavedSection, GammaKey, GEngine->DisplayGamma, GGameUserSettingsIni);
		bGammaPending = false;
	}
	for (const FName Channel : PendingVolumes)
	{
		GConfig->SetFloat(SavedSection, *GetVolumeKey(Channel), GetVolumeRef(Channel), GGameUserSettingsIni);
	}
	PendingVolumes.Reset();
	UGameUserSettings::GetGameUserSettings()->ApplySettings(false);
}

bool UKzSettingsLibrary::IsSettingSupported(EKzUIStandardSetting Setting)
{
	return Setting != EKzUIStandardSetting::HDR || UGameUserSettings::GetGameUserSettings()->SupportsHDRDisplayOutput();
}

void UKzSettingsLibrary::AutoDetectQuality(bool bApply)
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	Settings->RunHardwareBenchmark();
	if (bApply)
	{
		Settings->ApplyHardwareBenchmarkResults();
	}
	OnSettingChanged().Broadcast();
}

void UKzSettingsLibrary::ApplySavedGamma()
{
	float Gamma = 0.0f;
	if (GEngine && GConfig && GConfig->GetFloat(SavedSection, GammaKey, Gamma, GGameUserSettingsIni))
	{
		GEngine->DisplayGamma = FMath::Clamp(Gamma, MinGamma, MaxGamma);
	}
}

float UKzSettingsLibrary::GetVolume(FName Channel)
{
	return GetVolumeRef(Channel);
}

void UKzSettingsLibrary::SetVolume(const UObject* WorldContextObject, FName Channel, float Volume, bool bApply)
{
	if (!UKzUIInputSettings::Get()->VolumeChannels.Contains(Channel))
	{
		return;
	}

	Volume = FMath::Clamp(Volume, 0.0f, 1.0f);
	GetVolumeRef(Channel) = Volume;
	PendingVolumes.Add(Channel);
	SendVolume(WorldContextObject, Channel, Volume);
	NotifyChanged(bApply);
}

void UKzSettingsLibrary::ApplyVolumes(const UObject* WorldContextObject)
{
	const UKzUIInputSettings* Settings = UKzUIInputSettings::Get();
	for (const TPair<FName, FKzUIVolumeChannel>& Pair : Settings->VolumeChannels)
	{
		SendVolume(WorldContextObject, Pair.Key, GetVolumeRef(Pair.Key));
	}

	// The class overrides are in place before the mix becomes active, so nothing plays at full volume first
	if (USoundMix* Mix = Settings->VolumeSoundMix.LoadSynchronous())
	{
		UGameplayStatics::PushSoundMixModifier(WorldContextObject, Mix);
	}
}

bool UKzSettingsLibrary::IsVideoModeUnconfirmed()
{
	const UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	return Settings->GetScreenResolution() != Settings->GetLastConfirmedScreenResolution() || Settings->GetFullscreenMode() != Settings->GetLastConfirmedFullscreenMode();
}

void UKzSettingsLibrary::ConfirmVideoMode()
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	Settings->ConfirmVideoMode();
	Settings->SaveSettings();
}

void UKzSettingsLibrary::RevertVideoMode(bool bApply)
{
	UGameUserSettings::GetGameUserSettings()->RevertVideoMode();
	NotifyChanged(bApply);
}

FSimpleMulticastDelegate& UKzSettingsLibrary::OnSettingChanged()
{
	static FSimpleMulticastDelegate Delegate;
	return Delegate;
}

void UKzSettingsLibrary::NotifyChanged(bool bApply)
{
	if (bApply)
	{
		ApplySettings();
	}
	OnSettingChanged().Broadcast();
}

FKzUISettingState UKzSettingsLibrary::CaptureSettingState()
{
	const UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	return { Settings->GetScreenResolution(), Settings->ScalabilityQuality };
}

void UKzSettingsLibrary::RestoreSettingState(EKzUIStandardSetting Setting, const FKzUISettingState& State, bool bApply)
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	switch (Setting)
	{
		case EKzUIStandardSetting::Resolution:
			Settings->SetScreenResolution(State.Resolution);
			break;
		case EKzUIStandardSetting::Quality:
			// A single overall level also moves the resolution scale, so every group comes back
			Settings->ScalabilityQuality = State.Quality;
			break;
		default:
			return;
	}

	NotifyChanged(bApply);
}

#undef LOCTEXT_NAMESPACE
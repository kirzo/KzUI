// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"
#include "KzUITypes.h"
#include "KzUIInputSettings.generated.h"

class UKzPromptWidget;
class UKzUISoundTheme;
class UKzUIIconTheme;
class UKzUIVolumeApplier;
class USoundMix;

/** One volume the player can set (Music, Effects...), saved per machine and applied to its Target. */
USTRUCT(BlueprintType)
struct KZUI_API FKzUIVolumeChannel
{
	GENERATED_BODY()

	/** Sound Class (through VolumeSoundMix) or Sound Submix the volume goes to. Without one, the project's VolumeApplier applies it (Control Buses, Wwise, FMOD). */
	UPROPERTY(Category = Audio, EditAnywhere, meta = (AllowedClasses = "/Script/Engine.SoundClass,/Script/Engine.SoundSubmix"))
	TSoftObjectPtr<UObject> Target;

	UPROPERTY(Category = Audio, EditAnywhere, meta = (ClampMin = 0, ClampMax = 1))
	float DefaultVolume = 1.0f;
};

/**
 * Project-wide key mapping for semantic UI inputs. Available under
 * Project Settings -> Plugins -> KzUI.
 */
UCLASS(Config = KzUI, DefaultConfig, meta = (DisplayName = "KzUI"))
class KZUI_API UKzUIInputSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UKzUIInputSettings();

	//~ UDeveloperSettings
	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }

	static const UKzUIInputSettings* Get() { return GetDefault<UKzUIInputSettings>(); }

	/** Keys of Input for a player on the given pad family, resolved once per family. */
	const FKzUIInputDefinition* FindInputDefinition(EKzUIInputType Input, EKzUIInputDevice GamepadDevice) const;

	virtual void PostInitProperties() override;
	virtual void PostReloadConfig(FProperty* PropertyThatWasLoaded) override;
#if WITH_EDITOR
	/** Also reports the IsDataValid issues to the Asset Check log, since nothing validates project settings on save. */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	/** Warns about inputs without keys and keys mapped to two inputs on the same device, per pad family, and about volume channels that cannot be heard. */
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	UPROPERTY(Category = Input, EditAnywhere, Config, meta = (ReadOnlyKeys, ForceInlineRow))
	TMap<EKzUIInputType, FKzUIInputData> InputMap;

	/** Whether the game can be played with the mouse. Off: the mouse never changes the input device, menus and the splash ignore it, and KzUI keeps the cursor hidden. */
	UPROPERTY(Category = Input, EditAnywhere, Config)
	bool bSupportsMouse = true;

	UPROPERTY(Category = Input, EditAnywhere, Config, meta = (EditCondition = "bSupportsMouse"))
	EKzUICursorPolicy CursorPolicy = EKzUICursorPolicy::Manual;

	/** Maps hardware device identifier names (as reported by the input stack: WinDualShock, GameInput overrides, etc.) to icon device families. Unmatched keyboard/mouse devices resolve to Keyboard, anything else to the platform's own pad (Xbox on desktop). */
	UPROPERTY(Category = Input, EditAnywhere, Config)
	TMap<FName, EKzUIInputDevice> DeviceMappings;

	/** Default sound theme. Do not rename the referenced asset without re-picking it here: config paths are not fixed up by redirectors. */
	UPROPERTY(Category = Sound, EditAnywhere, Config)
	TSoftObjectPtr<UKzUISoundTheme> SoundTheme;

	/** Default icon theme. Same rename caveat as SoundTheme. */
	UPROPERTY(Category = Icons, EditAnywhere, Config)
	TSoftObjectPtr<UKzUIIconTheme> IconTheme;

	/** For games whose prompts are drawn for a pad: a player on the keyboard sees the icons of their last pad, or of the platform's own before they use one. Only the icons change; the keyboard keeps working. */
	UPROPERTY(Category = Icons, EditAnywhere, Config)
	bool bGamepadIconsOnKeyboard = false;

	/** Prompt widget spawned by UKzUserWidget::ShowPrompt. Same rename caveat as SoundTheme. */
	UPROPERTY(Category = Prompt, EditAnywhere, Config)
	TSoftClassPtr<UKzPromptWidget> PromptClass;

	/** Screen layers that AKzHUD puts widgets on, as gameplay tags, with their ZOrder: higher draws on top. Starts with UI.Layer.HUD (0) and UI.Layer.Menu (100). A tag without an entry takes its closest parent's. */
	UPROPERTY(Category = Layers, EditAnywhere, Config, meta = (Categories = "UI.Layer", ForceInlineRow))
	TMap<FGameplayTag, int32> Layers;

	/** ZOrder of the layer or of its closest parent in Layers; 0 if none has one. */
	int32 GetLayerZOrder(FGameplayTag Layer) const;

	/** Volumes the player can set, by the name settings rows and UKzSettingsLibrary::SetVolume use. Applied when each game instance starts, after its first world loads. */
	UPROPERTY(Category = Audio, EditAnywhere, Config, meta = (ForceInlineRow))
	TMap<FName, FKzUIVolumeChannel> VolumeChannels;

	/** Sound Mix that carries the volumes of the channels targeting a Sound Class, pushed when the game instance starts. */
	UPROPERTY(Category = Audio, EditAnywhere, Config)
	TSoftObjectPtr<USoundMix> VolumeSoundMix;

	/** Applies the channels without a Target, for audio a Sound Class or Submix cannot reach. */
	UPROPERTY(Category = Audio, EditAnywhere, Config)
	TSoftClassPtr<UKzUIVolumeApplier> VolumeApplier;

private:
	void AddMissingInputs();

	/** InputMap resolved per pad family, built on first use and dropped when the settings change. */
	mutable TMap<EKzUIInputDevice, TMap<EKzUIInputType, FKzUIInputDefinition>> ResolvedInputs;
};
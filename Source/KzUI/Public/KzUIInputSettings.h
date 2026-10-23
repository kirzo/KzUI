// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "KzUITypes.h"
#include "KzUIInputSettings.generated.h"

class UKzPromptWidget;
class UKzUISoundTheme;
class UKzUIIconTheme;

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
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
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

	/** Prompt widget spawned by UKzUserWidget::ShowPrompt. Same rename caveat as SoundTheme. */
	UPROPERTY(Category = Prompt, EditAnywhere, Config)
	TSoftClassPtr<UKzPromptWidget> PromptClass;

private:
	void AddMissingInputs();

	/** InputMap resolved per pad family, built on first use and dropped when the settings change. */
	mutable TMap<EKzUIInputDevice, TMap<EKzUIInputType, FKzUIInputDefinition>> ResolvedInputs;
};
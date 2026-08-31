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

	UPROPERTY(Category = Input, EditAnywhere, Config, meta = (ReadOnlyKeys, ForceInlineRow))
	TMap<EKzUIInputType, FKzUIInputData> InputMap;

	UPROPERTY(Category = Input, EditAnywhere, Config)
	EKzUICursorPolicy CursorPolicy = EKzUICursorPolicy::Manual;

	/** Maps hardware device identifier names (as reported by the input stack: WinDualShock, GameInput overrides, etc.) to icon device families. Unmatched keyboard/mouse devices resolve to Keyboard, anything else to Xbox. */
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
};
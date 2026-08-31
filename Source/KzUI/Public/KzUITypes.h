// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "KzUITypes.generated.h"

UENUM(BlueprintType, meta = (Bitflags))
enum class EKzUIInputType : uint8
{
	None UMETA(Hidden),
	Back, Accept,
	Down, Up, Left, Right,
	Previous, Next,
	Start, Select,
	MAX UMETA(Hidden),
};
ENUM_RANGE_BY_FIRST_AND_LAST(EKzUIInputType, EKzUIInputType::Back, EKzUIInputType::Select);

enum class EKzUIInputResult : uint8
{
	Ignored, Handled, Unhandled
};

/** Input device family, mainly used to pick matching button prompts. */
UENUM(BlueprintType)
enum class EKzUIInputDevice : uint8
{
	Keyboard, Xbox, DualShock, DualSense, Switch
};

/** Visual state of a selectable widget, used to pick the matching style. Style resolution priority: Disabled > Hovered > Selected > Normal. */
UENUM(BlueprintType)
enum class EKzUIWidgetState : uint8
{
	Normal, Hovered, Selected, Disabled
};

UENUM(BlueprintType)
enum class EKzUICursorPolicy : uint8
{
	/** KzUI never touches the cursor; the game manages it. */
	Manual,
	/** The cursor is shown only while the player's current input device is the keyboard/mouse. */
	ByDevice,
};

UENUM(BlueprintType)
enum class EKzUIInputOverrideMode : uint8
{
	Override, Additive, Subtractive
};

UENUM(BlueprintType)
enum class EKzUIInputTrigger : uint8
{
	/** Fires on key down. Any key of the definition triggers it. */
	Press,
	/** Fires on the second press within TapInterval. */
	DoubleTap,
	/** Fires on its own keys only while every RequiredInput is held. */
	Chord,
	/** Fires once after the input has been held HoldTime seconds. Releasing earlier cancels. */
	Hold,
};

USTRUCT(BlueprintType)
struct KZUI_API FKzUIInputTriggerConfig
{
	GENERATED_BODY()

	UPROPERTY(Category = Trigger, EditAnywhere, BlueprintReadWrite)
	EKzUIInputTrigger Trigger = EKzUIInputTrigger::Press;

	/** Max seconds between the two presses. */
	UPROPERTY(Category = Trigger, EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "Trigger == EKzUIInputTrigger::DoubleTap", EditConditionHides, ClampMin = 0.05))
	float TapInterval = 0.3f;

	/** Inputs that must already be held for this input to fire. Use discrete inputs, not navigation directions. */
	UPROPERTY(Category = Trigger, EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "Trigger == EKzUIInputTrigger::Chord", EditConditionHides))
	TArray<EKzUIInputType> RequiredInputs;

	/** Seconds the input must stay pressed before it fires. */
	UPROPERTY(Category = Trigger, EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "Trigger == EKzUIInputTrigger::Hold", EditConditionHides, ClampMin = 0.05))
	float HoldTime = 0.5f;
};

USTRUCT(BlueprintType)
struct KZUI_API FKzUIInputDefinition
{
	GENERATED_BODY()

private:
	/** Keys bound to this input type */
	UPROPERTY(Category = Input, EditAnywhere)
	TArray<FKey> Keys;

public:
	FKzUIInputDefinition() = default;
	FKzUIInputDefinition(TArray<FKey> InKeys) : Keys(InKeys) {}

	TArray<FKey> GetKeys() const { return Keys; }
	bool Contains(const FKey& Key) const { return Keys.Contains(Key); }

	bool operator==(const FKzUIInputDefinition& Other) const { return Keys == Other.Keys; }
	bool operator!=(const FKzUIInputDefinition& Other) const { return !(*this == Other); }
};

USTRUCT(BlueprintType)
struct KZUI_API FKzUIInputDefinitionOverride : public FKzUIInputDefinition
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = Input, EditAnywhere)
	EKzUIInputOverrideMode Mode = EKzUIInputOverrideMode::Override;

	FKzUIInputDefinitionOverride() = default;
	FKzUIInputDefinitionOverride(EKzUIInputOverrideMode InMode, TArray<FKey> InKeys) : Super(InKeys), Mode(InMode) {}
};

/** Default key set for one UI input, with optional per-platform overrides. */
USTRUCT(BlueprintType)
struct KZUI_API FKzUIInputData
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = Input, EditDefaultsOnly, BlueprintReadOnly)
	FKzUIInputDefinition Default;

	/** How the keys fire this input. The trigger is shared by every platform. */
	UPROPERTY(Category = Input, EditDefaultsOnly, BlueprintReadOnly)
	FKzUIInputTriggerConfig TriggerConfig;

	UPROPERTY(Category = Input, EditDefaultsOnly, BlueprintReadOnly, meta = (InlineEditConditionToggle))
	bool bOverrideWindows = false;

	UPROPERTY(Category = Input, EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "bOverrideWindows"))
	FKzUIInputDefinitionOverride Windows;

	UPROPERTY(Category = Input, EditDefaultsOnly, BlueprintReadOnly, meta = (InlineEditConditionToggle))
	bool bOverrideXbox = false;

	UPROPERTY(Category = Input, EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "bOverrideXbox"))
	FKzUIInputDefinitionOverride Xbox;

	UPROPERTY(Category = Input, EditDefaultsOnly, BlueprintReadOnly, meta = (InlineEditConditionToggle))
	bool bOverridePlayStation = false;

	UPROPERTY(Category = Input, EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "bOverridePlayStation"))
	FKzUIInputDefinitionOverride PlayStation;

	UPROPERTY(Category = Input, EditDefaultsOnly, BlueprintReadOnly, meta = (InlineEditConditionToggle))
	bool bOverrideSwitch = false;

	UPROPERTY(Category = Input, EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "bOverrideSwitch"))
	FKzUIInputDefinitionOverride Switch;

public:
	/** Resolves the effective key set for the platform this build is running on. */
	FKzUIInputDefinition GetInputDefinition() const;
};

DECLARE_DYNAMIC_DELEGATE(FKzUISimpleDelegate);

/** Wraps a dynamic delegate so it can be passed around as an ExposeOnSpawn property. */
USTRUCT(BlueprintType)
struct KZUI_API FKzUIDelegateWrapper
{
	GENERATED_BODY()

	UPROPERTY(Category = Delegate, BlueprintReadWrite)
	FKzUISimpleDelegate Delegate;
};
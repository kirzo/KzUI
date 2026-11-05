// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "KzUITypes.h"
#include "StructUtils/InstancedStruct.h"
#include "KzUIIconSource.generated.h"

class UInputAction;
class UKzInputIcon;
class UKzUIIconTheme;
class ULocalPlayer;

/** Where an icon source resolves: the widget showing it, its player and device, and the theme. */
struct FKzUIIconContext
{
	const UKzInputIcon* Icon = nullptr;

	/** Null in the designer and for widgets without an owning player. */
	const ULocalPlayer* Player = nullptr;

	EKzUIInputDevice Device = EKzUIInputDevice::Keyboard;

	/** The player's pad family, which the UI input map resolves for even while they are on the keyboard. */
	EKzUIInputDevice GamepadDevice = EKzUIInputDevice::Xbox;

	const UKzUIIconTheme* Theme = nullptr;
};

/** What a source shows: an icon of the theme, else a key whose theme icon or name stands in, else a text. */
struct FKzUIIconResolution
{
	UObject* Icon = nullptr;
	FKey Key;
	FText Text;
};

/** What a KzInputIcon shows. Any module adds sources by deriving from it; KzTextBlock offers each {Token} to every source type. */
USTRUCT()
struct KZUI_API FKzUIIconSource
{
	GENERATED_BODY()

	virtual ~FKzUIIconSource() = default;

	virtual FKzUIIconResolution Resolve(const FKzUIIconContext& Context) const { return FKzUIIconResolution(); }

	/** 0 to 1, for a KzInputIcon that shows the hold progress. */
	virtual float GetHoldProgress(const FKzUIIconContext& Context) const { return 0.0f; }

	/** Takes a KzTextBlock token when it is this source's kind, filling the fields from it. */
	virtual bool InitFromToken(const FString& Token, const UKzUIIconTheme* Theme) { return false; }

	/** Order in which KzTextBlock offers a token to the source types, lowest first. */
	virtual int32 GetTokenOrder() const { return 0; }

	/** A source for a KzTextBlock token from the first source type that takes it, or an empty one. */
	static TInstancedStruct<FKzUIIconSource> MakeFromToken(const FString& Token, const UKzUIIconTheme* Theme);

protected:
	/** The first of Keys for the kind of device the player is on: a pad key on a pad, a keyboard or mouse key otherwise. */
	static FKey FindKeyForDevice(TConstArrayView<FKey> Keys, EKzUIInputDevice Device);
};

/** A UI input of KzUI: its icon in the theme, or the key that fires it on the player's device. */
USTRUCT(meta = (DisplayName = "UI Input"))
struct KZUI_API FKzUIIconSource_Input : public FKzUIIconSource
{
	GENERATED_BODY()

	UPROPERTY(Category = Icon, EditAnywhere)
	EKzUIInputType Input = EKzUIInputType::Accept;

	virtual FKzUIIconResolution Resolve(const FKzUIIconContext& Context) const override;

	/** The hold progress of the input in the KzUserWidget that owns the icon. */
	virtual float GetHoldProgress(const FKzUIIconContext& Context) const override;

	virtual bool InitFromToken(const FString& Token, const UKzUIIconTheme* Theme) override;
	virtual int32 GetTokenOrder() const override { return 0; }
};

/** A custom icon of the theme. */
USTRUCT(meta = (DisplayName = "Custom"))
struct KZUI_API FKzUIIconSource_Custom : public FKzUIIconSource
{
	GENERATED_BODY()

	/** Name of the icon in the theme's CustomIcons. */
	UPROPERTY(Category = Icon, EditAnywhere)
	FName Token;

	virtual FKzUIIconResolution Resolve(const FKzUIIconContext& Context) const override;
	virtual bool InitFromToken(const FString& InToken, const UKzUIIconTheme* Theme) override;
	virtual int32 GetTokenOrder() const override { return 100; }
};

/** A physical key: its icon in the theme, or its name. */
USTRUCT(meta = (DisplayName = "Key"))
struct KZUI_API FKzUIIconSource_Key : public FKzUIIconSource
{
	GENERATED_BODY()

	UPROPERTY(Category = Icon, EditAnywhere)
	FKey Key;

	virtual FKzUIIconResolution Resolve(const FKzUIIconContext& Context) const override;
	virtual bool InitFromToken(const FString& Token, const UKzUIIconTheme* Theme) override;
	virtual int32 GetTokenOrder() const override { return 300; }
};

/** An Enhanced Input action: the first key mapped to it for the player's device in the contexts they have on, remaps included. Subclasses choose the action. */
USTRUCT(meta = (Hidden))
struct KZUI_API FKzUIIconSource_InputActionBase : public FKzUIIconSource
{
	GENERATED_BODY()

	virtual FKzUIIconResolution Resolve(const FKzUIIconContext& Context) const override;

	/** The action shown for the context's player. */
	virtual const UInputAction* GetInputAction(const FKzUIIconContext& Context) const { return nullptr; }

	/** Shown in the designer, where there is no player whose keys to read. */
	virtual FText GetPreviewText() const { return FText::GetEmpty(); }
};

/** An Enhanced Input action picked here. */
USTRUCT(meta = (DisplayName = "Input Action"))
struct KZUI_API FKzUIIconSource_InputAction : public FKzUIIconSource_InputActionBase
{
	GENERATED_BODY()

	UPROPERTY(Category = Icon, EditAnywhere)
	TObjectPtr<const UInputAction> InputAction;

	virtual const UInputAction* GetInputAction(const FKzUIIconContext& Context) const override;
	virtual FText GetPreviewText() const override;
};
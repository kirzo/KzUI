// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "KzTextStyle.generated.h"

/** One text look. Designers subclass it in Blueprint and cascade through class inheritance. */
UCLASS(Abstract, Blueprintable, BlueprintType)
class KZUI_API UKzTextStyle : public UObject
{
	GENERATED_BODY()

public:
	UKzTextStyle();

	UPROPERTY(Category = Text, EditDefaultsOnly, BlueprintReadOnly)
	FSlateFontInfo Font;

	UPROPERTY(Category = Text, EditDefaultsOnly, BlueprintReadOnly)
	FSlateColor Color;

	UPROPERTY(Category = Shadow, EditDefaultsOnly, BlueprintReadOnly)
	FVector2D ShadowOffset;

	/** Alpha 0 disables the shadow. */
	UPROPERTY(Category = Shadow, EditDefaultsOnly, BlueprintReadOnly)
	FLinearColor ShadowColor;
};

/** Per-state text looks for a KzTextBlock. Unset states fall back to Normal. Subclasses may override individual states. */
UCLASS(Abstract, Blueprintable, BlueprintType)
class KZUI_API UKzTextBlockStyle : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = States, EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UKzTextStyle> Normal;

	UPROPERTY(Category = States, EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UKzTextStyle> Hovered;

	UPROPERTY(Category = States, EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UKzTextStyle> Selected;

	UPROPERTY(Category = States, EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UKzTextStyle> Disabled;
};
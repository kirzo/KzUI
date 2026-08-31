// Copyright 2026 kirzo

#pragma once

#include "Engine/DataAsset.h"
#include "KzUIFlipbook.generated.h"

class UTexture2D;

/** Frame-based UI animation. One texture = sprite sheet in a grid; several = consecutive pages, each holding as many frames as its grid fits. */
UCLASS(BlueprintType)
class KZUI_API UKzUIFlipbook : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = Flipbook, EditAnywhere, BlueprintReadOnly)
	TArray<TObjectPtr<UTexture2D>> Textures;

	UPROPERTY(Category = Flipbook, EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 1))
	int32 TotalFrames = 1;

	UPROPERTY(Category = Flipbook, EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 1))
	int32 FramesPerSecond = 10;

	/** Size of one frame inside the texture, in pixels. */
	UPROPERTY(Category = Flipbook, EditAnywhere, BlueprintReadOnly)
	FVector2D FrameSize = FVector2D(64.0, 64.0);

	/** Looping playback starts automatically and is synced to a global clock across every widget using this flipbook. */
	UPROPERTY(Category = Flipbook, EditAnywhere, BlueprintReadOnly)
	bool bLooping = true;

	UFUNCTION(Category = "KzUI", BlueprintPure)
	float GetDuration() const { return TotalFrames / float(FramesPerSecond); }

	/** Resolves the texture page and UV region of a frame. */
	bool GetFrame(int32 Frame, UTexture2D*& OutTexture, FBox2D& OutUVRegion) const;
};
// Copyright 2026 kirzo

#pragma once

#include "Engine/DataAsset.h"
#include "KzUITypes.h"
#include "KzUISoundTheme.generated.h"

class USoundBase;

/** Sound set for KzUI feedback. The default theme is set in project settings; a per-player theme can be swapped at runtime through UKzUIInputSubsystem. */
UCLASS(BlueprintType)
class KZUI_API UKzUISoundTheme : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Sound played when a widget processes the given input. Widgets can override per instance. */
	UPROPERTY(Category = Sound, EditAnywhere)
	TMap<EKzUIInputType, TObjectPtr<USoundBase>> InputSounds;

	/** Sound played when a navigable widget moves its hover, by mouse or directional navigation. On navigable widgets it replaces the per-direction InputSounds. */
	UPROPERTY(Category = Sound, EditAnywhere)
	TObjectPtr<USoundBase> HoverSound;

	/** Sound played when a navigable widget commits a new selection, by mouse, Accept or SelectOption. Plays in addition to any Accept entry of InputSounds. */
	UPROPERTY(Category = Sound, EditAnywhere)
	TObjectPtr<USoundBase> SelectSound;
};
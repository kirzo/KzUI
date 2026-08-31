// Copyright 2026 kirzo

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/LatentActionManager.h"
#include "KzUITypes.h"
#include "KzUIFunctionLibrary.generated.h"

class UUserWidget;
class UWidget;
struct FKeyEvent;

UCLASS()
class KZUI_API UKzUIFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Maps a key event to a semantic UI input using the project settings. */
	static EKzUIInputType GetInputFromKeyEvent(const FKeyEvent& InKeyEvent);

	/** Loads a soft widget class asynchronously and creates an instance once ready. */
	UFUNCTION(Category = "KzUI", BlueprintCallable, DisplayName = "Create Widget (Async)", meta = (WorldContext = "WorldContextObject", Latent, LatentInfo = "LatentInfo"))
	static void CreateWidgetAsync(UObject* WorldContextObject, FLatentActionInfo LatentInfo, UUserWidget*& Widget, TSoftClassPtr<UUserWidget> Class, APlayerController* OwningPlayer = nullptr);

	/** Projects a world location to a position centered on the widget's viewport, optionally clamped inside it. Locations behind the camera are pushed off the bottom edge. */
	UFUNCTION(Category = "KzUI", BlueprintCallable)
	static bool ProjectWorldLocationToViewport(UUserWidget* Widget, FVector WorldLocation, FVector2D& ViewportPosition, bool bClampInsideViewport = false, float ClampOffset = 0.0f);

	/** Internal: used by the Switch on Option node expansion. */
	UFUNCTION(BlueprintPure, meta = (BlueprintInternalUseOnly = "true"))
	static bool IsWidgetNamed(const UWidget* Widget, FName Name);
};
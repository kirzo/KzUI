// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "GameplayTagContainer.h"
#include "KzHUD.generated.h"

class UUserWidget;

/** A widget the HUD creates when the map starts, and the layer it goes on. */
USTRUCT(BlueprintType)
struct KZUI_API FKzHUDWidget
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KzUI")
	TSubclassOf<UUserWidget> Class;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KzUI", meta = (Categories = "UI.Layer"))
	FGameplayTag Layer;
};

/**
 * A player's UI in a map: puts widgets on layers (gameplay tags ordered in the KzUI settings), hides layers and pauses the game.
 * Hiding and pausing are counted by source: a layer shows again, and the game resumes, once every source has removed its request.
 * Hiding a tag hides the layers under it too.
 */
UCLASS()
class KZUI_API AKzHUD : public AHUD
{
	GENERATED_BODY()

public:
	/** The first local player's HUD. In local multiplayer, ask each player controller for its own. */
	UFUNCTION(BlueprintPure, Category = "KzUI", meta = (WorldContext = "WorldContextObject", DisplayName = "Get Kz HUD"))
	static AKzHUD* Get(const UObject* WorldContextObject);

	/** Adds the widget to the player's screen at its layer's ZOrder. */
	UFUNCTION(BlueprintCallable, Category = "KzUI")
	void AddToLayer(UUserWidget* Widget, UPARAM(meta = (Categories = "UI.Layer")) FGameplayTag Layer);

	UFUNCTION(BlueprintCallable, Category = "KzUI", meta = (DefaultToSelf = "Source"))
	void PushLayerHidden(UPARAM(meta = (Categories = "UI.Layer")) FGameplayTag Layer, UObject* Source);

	UFUNCTION(BlueprintCallable, Category = "KzUI", meta = (DefaultToSelf = "Source"))
	void RemoveLayerHidden(UPARAM(meta = (Categories = "UI.Layer")) FGameplayTag Layer, UObject* Source);

	/** Whether a request hides the layer, itself or through a parent tag. */
	UFUNCTION(BlueprintPure, Category = "KzUI")
	bool IsLayerHidden(UPARAM(meta = (Categories = "UI.Layer")) FGameplayTag Layer) const;

	UFUNCTION(BlueprintCallable, Category = "KzUI", meta = (DefaultToSelf = "Source"))
	void PushPause(UObject* Source);

	UFUNCTION(BlueprintCallable, Category = "KzUI", meta = (DefaultToSelf = "Source"))
	void RemovePause(UObject* Source);

	/** The first widget of the class on this HUD's layers, such as one it created at the start. */
	UFUNCTION(BlueprintPure, Category = "KzUI", meta = (DeterminesOutputType = "Class"))
	UUserWidget* GetWidget(TSubclassOf<UUserWidget> Class) const;

protected:
	/** Created on their layers when the map starts. */
	UPROPERTY(EditDefaultsOnly, Category = "KzUI")
	TArray<FKzHUDWidget> Widgets;

	virtual void BeginPlay() override;

private:
	struct FLayerWidget
	{
		TWeakObjectPtr<UUserWidget> Widget;
		FGameplayTag Layer;

		/** The visibility it had when its layer was hidden, given back when the layer shows. */
		TOptional<ESlateVisibility> ShownVisibility;
	};
	TArray<FLayerWidget> LayerWidgets;

	TMap<FGameplayTag, TSet<TWeakObjectPtr<UObject>>> HiddenLayers;
	TSet<TWeakObjectPtr<UObject>> PauseSources;

	void RefreshLayers();
};

// Copyright 2026 kirzo

#pragma once

#include "KzImage.h"
#include "KzUITypes.h"
#include "KzInputIcon.generated.h"

/** Displays the button prompt icon of a semantic input, resolved from the active icon theme and refreshed automatically when the input device changes. */
UCLASS(meta = (PrioritizeCategories = "Icon Image"))
class KZUI_API UKzInputIcon : public UKzImage
{
	GENERATED_BODY()

public:
	/** Input whose icon is displayed. */
	UPROPERTY(Category = Icon, EditAnywhere, BlueprintReadOnly)
	EKzUIInputType Input = EKzUIInputType::Accept;

	/** When set, displays this custom icon of the theme instead of the Input one. */
	UPROPERTY(Category = Icon, EditAnywhere, BlueprintReadOnly)
	FName CustomToken;

	/** Icon variation of the theme wrapping the resolved icon texture in a material. */
	UPROPERTY(Category = Icon, EditAnywhere, BlueprintReadOnly)
	FName Variation;

	/** Feeds the hold progress of Input in the owning KzUserWidget (0 to 1) to the theme's HoldProgressParameter of the icon material every frame, for hold-to-confirm prompts. Needs a material icon or variation. */
	UPROPERTY(Category = Icon, EditAnywhere, BlueprintReadOnly)
	bool bShowHoldProgress = false;

#if WITH_EDITORONLY_DATA
	/** Device used to preview the icon in the designer. */
	UPROPERTY(Category = Icon, EditAnywhere)
	EKzUIInputDevice PreviewDevice = EKzUIInputDevice::Keyboard;
#endif

	UFUNCTION(Category = "KzUI|Icon", BlueprintCallable)
	void SetInput(EKzUIInputType NewInput);

	UFUNCTION(Category = "KzUI|Icon", BlueprintCallable)
	void SetCustomToken(FName NewToken);

	UFUNCTION(Category = "KzUI|Icon", BlueprintCallable)
	void SetVariation(FName NewVariation);

	/** Size of the icon shown, for its aspect ratio: the texture or flipbook frame size, square for a material, zero when nothing is shown. */
	FVector2D GetNaturalSize() const { return NaturalSize; }

	/** While the theme has no icon for the input or its key on this device, the display name of that key ("Space", "E"). Empty for custom tokens. */
	FText GetFallbackText() const { return FallbackText; }

	//~ Begin UWidget Interface
	virtual void SynchronizeProperties() override;

protected:
	virtual void OnWidgetRebuilt() override;
	//~ End UWidget Interface

private:
	void RefreshIcon();

	/** Subscribes to the device changes of the player's subsystem, moving the subscription if the player changed. */
	class UKzUIInputSubsystem* BindPlayerSubsystem();

	/** Shows no icon at all, rather than the last device's icon or the white box of an empty brush. */
	void ClearIcon();

	UFUNCTION()
	void OnInputDeviceChanged(EKzUIInputDevice InputDevice);

	EActiveTimerReturnType UpdateHoldProgress();

	/** Last value pushed to the material, so it is only set when it changes. */
	float LastHoldProgress = -1.0f;

	TWeakObjectPtr<class UKzUIInputSubsystem> BoundSubsystem;

	FVector2D NaturalSize = FVector2D::ZeroVector;

	FText FallbackText;
};
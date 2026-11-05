// Copyright 2026 kirzo

#pragma once

#include "KzImage.h"
#include "KzUIIconSource.h"
#include "KzUITypes.h"
#include "KzInputIcon.generated.h"

/** Displays an input icon of the active icon theme, for whatever its source names (a UI input, a key, a custom icon...), refreshed automatically when the input device changes. */
UCLASS(meta = (PrioritizeCategories = "Icon Image"))
class KZUI_API UKzInputIcon : public UKzImage
{
	GENERATED_BODY()

public:
	UKzInputIcon(const FObjectInitializer& ObjectInitializer);

	/** What the icon shows. */
	UPROPERTY(Category = Icon, EditAnywhere, meta = (ExcludeBaseStruct))
	TInstancedStruct<FKzUIIconSource> Source;

	/** Icon variation of the theme wrapping the resolved icon texture in a material. */
	UPROPERTY(Category = Icon, EditAnywhere, BlueprintReadOnly)
	FName Variation;

	/** Feeds the source's hold progress (0 to 1) to the theme's HoldProgressParameter of the icon material every frame, for hold-to-confirm prompts; a UI Input source gives its input's in the owning KzUserWidget. Needs a material icon or variation. */
	UPROPERTY(Category = Icon, EditAnywhere, BlueprintReadOnly)
	bool bShowHoldProgress = false;

#if WITH_EDITORONLY_DATA
	/** Device used to preview the icon in the designer. */
	UPROPERTY(Category = Icon, EditAnywhere)
	EKzUIInputDevice PreviewDevice = EKzUIInputDevice::Keyboard;
#endif

	/** Shows whatever NewSource names. */
	void SetSource(TInstancedStruct<FKzUIIconSource> NewSource);

	/** The source as T, or null when it is another kind. */
	template<typename T>
	const T* GetSource() const { return Source.GetPtr<T>(); }

	/** Shows a UI input. */
	UFUNCTION(Category = "KzUI|Icon", BlueprintCallable)
	void SetInput(EKzUIInputType NewInput);

	/** Shows a custom icon of the theme. */
	UFUNCTION(Category = "KzUI|Icon", BlueprintCallable)
	void SetCustomToken(FName NewToken);

	UFUNCTION(Category = "KzUI|Icon", BlueprintCallable)
	void SetVariation(FName NewVariation);

	/** Size of the icon shown, for its aspect ratio: the texture or flipbook frame size, square for a material, zero when nothing is shown. */
	FVector2D GetNaturalSize() const { return NaturalSize; }

	/** While the theme has no icon for what the source names on this device, the display name of its key ("Space", "E") or the source's own text. */
	FText GetFallbackText() const { return FallbackText; }

	//~ Begin UWidget Interface
	virtual void SynchronizeProperties() override;

protected:
	virtual void OnWidgetRebuilt() override;
	//~ End UWidget Interface

private:
	/** The player, device and theme the source resolves in. */
	FKzUIIconContext MakeContext();

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
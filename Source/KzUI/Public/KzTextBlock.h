// Copyright 2026 kirzo

#pragma once

#include "Components/TextBlock.h"
#include "KzSelectableWidgetInterface.h"
#include "KzUITypes.h"
#include "KzTextBlock.generated.h"

class SWrapBox;
class UKzInputIcon;
class UKzTextBlockStyle;
class UKzTextStyle;
class UKzUIIconTheme;

/**
 * TextBlock with inline input icons: tokens like {Start} or {Accept} in the text are replaced
 * by a KzInputIcon, so prompts follow the active icon theme and input device. Unknown tokens
 * render as literal text. Property bindings on Text are not supported.
 *
 * When a style set is assigned, it drives the inherited text appearance properties per widget
 * state (normal/hovered/selected/disabled); states arrive through IKzSelectableWidgetInterface.
 */
UCLASS(meta = (PrioritizeCategories = "Style Icon Selection"))
class KZUI_API UKzTextBlock : public UTextBlock, public IKzSelectableWidgetInterface
{
	GENERATED_BODY()

public:
	/** Per-state looks driving the text appearance. Null keeps the plain TextBlock properties. */
	UPROPERTY(Category = Style, EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UKzTextBlockStyle> Style;

	/** Per-widget replacements for complete states of the style set. */
	UPROPERTY(Category = Style, EditAnywhere, BlueprintReadOnly)
	TMap<EKzUIWidgetState, TSubclassOf<UKzTextStyle>> StateOverrides;

	/** Whether this text can be directly selected as a navigation option. Linked selection ignores this. */
	UPROPERTY(Category = Selection, EditAnywhere, BlueprintReadOnly)
	bool bSelectable = false;

	/** Tint the icons with ColorAndOpacity as well. */
	UPROPERTY(Category = Icon, EditAnywhere)
	bool bApplyColorToIcons = false;

	/** Size the icons to the text line height. */
	UPROPERTY(Category = Icon, EditAnywhere)
	bool bAutoIconSize = true;

	/** Forced icon size. 0 = natural size from the icon theme. */
	UPROPERTY(Category = Icon, EditAnywhere, meta = (EditCondition = "!bAutoIconSize"))
	FVector2D IconSize = FVector2D::ZeroVector;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UKzInputIcon>> GeneratedIcons;

	bool bHovered = false;

	bool bSelected = false;

	TSharedPtr<SWrapBox> MyBox;

public:
	/** Icons generated for the current text, in order of appearance. Regenerated on every text or property change: do not cache them across SetText. */
	UFUNCTION(Category = "KzUI", BlueprintPure)
	TArray<UKzInputIcon*> GetIcons() const;

	/** First generated icon of the given input, or null. */
	UFUNCTION(Category = "KzUI", BlueprintPure)
	UKzInputIcon* GetIcon(EKzUIInputType Input) const;

	/** First generated icon of the given custom token, or null. */
	UFUNCTION(Category = "KzUI", BlueprintPure)
	UKzInputIcon* GetCustomIcon(FName Token) const;

	/** Replaces the style set and refreshes the appearance. */
	UFUNCTION(Category = "KzUI", BlueprintCallable)
	void SetStyle(TSubclassOf<UKzTextBlockStyle> NewStyle);

	//~ Begin UTextBlock Interface
	virtual void SetText(FText InText) override;
	//~ End UTextBlock Interface

	//~ Begin UWidget Interface
	virtual void SetIsEnabled(bool bInIsEnabled) override;
	//~ End UWidget Interface

	//~ Begin IKzSelectableWidgetInterface
	virtual bool IsSelectable_Implementation() const override { return bSelectable; }
	virtual void SetIsSelectable_Implementation(bool bNewSelectable) override { bSelectable = bNewSelectable; }
	virtual void OnSelect_Implementation() override;
	virtual void OnDeselect_Implementation() override;
	virtual void OnHovered_Implementation() override;
	virtual void OnUnhovered_Implementation() override;
	//~ End IKzSelectableWidgetInterface

	//~ Begin UWidget Interface
	virtual void SynchronizeProperties() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	//~ End UWidget Interface

private:
	void ApplyStyle();
	const UKzTextStyle* ResolveLook(EKzUIWidgetState State) const;
	void RebuildContent();
	void AddTextSegment(FString& Pending);
	void AddTextBlock(const FString& Word);
	void AddIcon(EKzUIInputType Input, FName CustomToken, FName Variation, int32 SpacesBefore, int32 SpacesAfter);
	const UKzUIIconTheme* GetActiveIconTheme() const;
};
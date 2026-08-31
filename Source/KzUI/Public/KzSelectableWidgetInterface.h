// Copyright 2026 kirzo

#pragma once

#include "KzUITypes.h"
#include "UObject/Interface.h"
#include "KzSelectableWidgetInterface.generated.h"

UINTERFACE(Blueprintable)
class KZUI_API UKzSelectableWidgetInterface : public UInterface
{
	GENERATED_BODY()
};

class UWidget;

/** Implemented by widgets that can be selected as options of a UKzNavigableWidget. */
class KZUI_API IKzSelectableWidgetInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(Category = Selection, BlueprintCallable, BlueprintNativeEvent)
	bool IsSelectable() const;
	virtual bool IsSelectable_Implementation() const { return true; }

	/** Whether Accept commits this widget as the selection. Hover-only widgets (e.g. setting rows that act instead of being selected) return false. */
	UFUNCTION(Category = Selection, BlueprintCallable, BlueprintNativeEvent)
	bool CanBeSelected() const;
	virtual bool CanBeSelected_Implementation() const { return true; }

	UFUNCTION(Category = Selection, BlueprintCallable, BlueprintNativeEvent)
	void SetIsSelectable(bool bNewSelectable);

	UFUNCTION(Category = Selection, BlueprintNativeEvent)
	void OnSelect();

	UFUNCTION(Category = Selection, BlueprintNativeEvent)
	void OnDeselect();

	/** The navigation or mouse cursor landed on this widget. */
	UFUNCTION(Category = Selection, BlueprintNativeEvent)
	void OnHovered();

	UFUNCTION(Category = Selection, BlueprintNativeEvent)
	void OnUnhovered();

	/** Widgets that should receive this widget's selection and hover changes as well, regardless of their own IsSelectable. */
	UFUNCTION(Category = Selection, BlueprintNativeEvent)
	TArray<UWidget*> GetLinkedSelectables() const;
	virtual TArray<UWidget*> GetLinkedSelectables_Implementation() const { return TArray<UWidget*>(); }

	/** While hovered, inputs this widget claims for itself (e.g. Left/Right on a spinner). The navigable widget sends them to HandleInput instead of navigating or selecting. */
	UFUNCTION(Category = Selection, BlueprintCallable, BlueprintNativeEvent)
	bool WantsInput(EKzUIInputType Input) const;
	virtual bool WantsInput_Implementation(EKzUIInputType Input) const { return false; }

	/** Performs the action of an input claimed through WantsInput. Returns whether the action had any effect, gating the feedback sound. */
	UFUNCTION(Category = Selection, BlueprintCallable, BlueprintNativeEvent)
	bool HandleInput(EKzUIInputType Input);
	virtual bool HandleInput_Implementation(EKzUIInputType Input) { return false; }

	/** Applies the selection change to the widget and to its linked selectables, guarding against cycles. */
	static void Select(UWidget* Widget, bool bSelected);

	/** Applies the hover change to the widget and to its linked selectables, guarding against cycles. */
	static void Hover(UWidget* Widget, bool bHovered);
};
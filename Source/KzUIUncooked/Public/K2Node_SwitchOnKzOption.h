// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "K2Node_SwitchOnKzOption.generated.h"

/**
 * Switch with one exec pin per option of the owning KzNavigableWidget Blueprint, enumerated
 * from its source widget tree (selectable and acceptable widgets, tree order). The editor module
 * refreshes the pins before every compile, so a renamed option with links fails to compile
 * instead of silently falling through to Default.
 */
UCLASS()
class KZUIUNCOOKED_API UK2Node_SwitchOnKzOption : public UK2Node
{
	GENERATED_BODY()

public:
	//~ Begin UEdGraphNode Interface
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	virtual bool IsCompatibleWithGraph(const UEdGraph* TargetGraph) const override;
	virtual void ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const override;
	//~ End UEdGraphNode Interface

	//~ Begin UK2Node Interface
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual FText GetMenuCategory() const override;
	virtual void ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	virtual bool ShouldShowNodeProperties() const override { return true; }
	//~ End UK2Node Interface

	TArray<FName> GetOptionNames() const;

	/** Reads the options from the widget tree again and rebuilds the pins if they changed. Returns whether they did. */
	bool RefreshOptions();

	/** Options toggled off in the details panel: no pin is generated, so they fall through to Default. */
	UPROPERTY()
	TArray<FName> DisabledOptions;

private:
	void RefreshCachedOptionNames();

	/** Option names from the source widget trees. False while the trees are unavailable (the Blueprint is loading). */
	bool CollectOptionNames(TArray<FName>& OutNames, bool* bOutSkippedDefault = nullptr) const;

	/** Last successfully enumerated option names. Pins are built from these so reconstruction while the widget tree is unavailable (e.g. during editor startup) does not wipe them. */
	UPROPERTY()
	TArray<FName> CachedOptionNames;
};
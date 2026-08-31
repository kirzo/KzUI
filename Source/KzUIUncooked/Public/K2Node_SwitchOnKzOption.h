// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "K2Node_SwitchOnKzOption.generated.h"

/**
 * Switch with one exec pin per option of the owning KzNavigableWidget Blueprint, enumerated
 * from its widget tree (selectable widgets, tree order). Pins refresh when the Blueprint
 * compiles; the comparison is baked against the current widget names on every compile.
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
	virtual bool IsCompatibleWithGraph(const UEdGraph* TargetGraph) const override;
	//~ End UEdGraphNode Interface

	//~ Begin UK2Node Interface
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual FText GetMenuCategory() const override;
	virtual void ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	virtual bool ShouldShowNodeProperties() const override { return true; }
	//~ End UK2Node Interface

	TArray<FName> GetOptionNames() const;

	/** Options toggled off in the details panel: no pin is generated, so they fall through to Default. */
	UPROPERTY()
	TArray<FName> DisabledOptions;

private:
	void RefreshCachedOptionNames();

	/** Last successfully enumerated option names. Pins are built from these so reconstruction while the widget tree is unavailable (e.g. during editor startup) does not wipe them. */
	UPROPERTY()
	TArray<FName> CachedOptionNames;
};
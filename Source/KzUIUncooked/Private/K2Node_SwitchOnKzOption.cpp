// Copyright 2026 kirzo

#include "K2Node_SwitchOnKzOption.h"

#include "KzNavigableWidget.h"
#include "KzSelectableWidgetInterface.h"
#include "KzUIFunctionLibrary.h"

#include "BaseWidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "K2Node_CallFunction.h"
#include "K2Node_IfThenElse.h"
#include "KismetCompiler.h"
#include "Styling/AppStyle.h"

namespace
{
	const FName OptionPinName(TEXT("Option"));
	const FName DefaultPinName(TEXT("Default"));

	/** Hover-only rows (spinner, slider, toggle) never reach OnOptionAccepted, so they get no pin. */
	bool IsAcceptableOption(UWidget* Widget)
	{
		return Widget->Implements<UKzSelectableWidgetInterface>()
			&& IKzSelectableWidgetInterface::Execute_IsSelectable(Widget)
			&& IKzSelectableWidgetInterface::Execute_CanBeSelected(Widget);
	}
}

void UK2Node_SwitchOnKzOption::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object, UWidget::StaticClass(), OptionPinName);

	RefreshCachedOptionNames();
	for (const FName OptionName : CachedOptionNames)
	{
		if (!DisabledOptions.Contains(OptionName))
		{
			CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, OptionName);
		}
	}
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, DefaultPinName);

	Super::AllocateDefaultPins();
}

FText UK2Node_SwitchOnKzOption::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return INVTEXT("Switch on Option");
}

FText UK2Node_SwitchOnKzOption::GetTooltipText() const
{
	return INVTEXT("Routes the execution to the pin matching the given option widget. Pins are generated from the selectable widgets of this Blueprint and refresh before every compile.");
}

FSlateIcon UK2Node_SwitchOnKzOption::GetIconAndTint(FLinearColor& OutColor) const
{
	OutColor = FLinearColor::White;
	return FSlateIcon(FAppStyle::GetAppStyleSetName(), "GraphEditor.Switch_16x");
}

bool UK2Node_SwitchOnKzOption::IsCompatibleWithGraph(const UEdGraph* TargetGraph) const
{
	if (!Super::IsCompatibleWithGraph(TargetGraph))
	{
		return false;
	}

	const UBlueprint* Blueprint = TargetGraph ? TargetGraph->GetTypedOuter<UBlueprint>() : nullptr;
	return Blueprint && Blueprint->ParentClass && Blueprint->ParentClass->IsChildOf(UKzNavigableWidget::StaticClass());
}

void UK2Node_SwitchOnKzOption::ValidateNodeDuringCompilation(FCompilerResultsLog& MessageLog) const
{
	Super::ValidateNodeDuringCompilation(MessageLog);

	// Placement checks the parent class only once, and a Blueprint can be reparented later
	const UBlueprint* Blueprint = GetBlueprint();
	if (!Blueprint || !Blueprint->ParentClass || !Blueprint->ParentClass->IsChildOf(UKzNavigableWidget::StaticClass()))
	{
		MessageLog.Warning(TEXT("@@ only works in a Kz Navigable Widget Blueprint."), this);
	}

	const UEdGraphPin* OptionPin = FindPin(OptionPinName, EGPD_Input);
	if (OptionPin && OptionPin->LinkedTo.Num() == 0)
	{
		MessageLog.Warning(TEXT("@@ has no Option connected, so it always takes Default."), this);
	}

	TArray<FName> Names;
	bool bSkippedDefault = false;
	if (CollectOptionNames(Names, &bSkippedDefault) && bSkippedDefault)
	{
		MessageLog.Warning(TEXT("@@ cannot give a pin to the option named Default, which would take the Default pin's place: rename the widget."), this);
	}
}

void UK2Node_SwitchOnKzOption::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

FText UK2Node_SwitchOnKzOption::GetMenuCategory() const
{
	return INVTEXT("Kz UI");
}

void UK2Node_SwitchOnKzOption::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();
	UEdGraphPin* ExecPin = GetExecPin();
	UEdGraphPin* OptionPin = FindPinChecked(OptionPinName, EGPD_Input);

	// Chain of "is it named X?" branches, one per option pin, falling through to Default
	UEdGraphPin* ChainPin = ExecPin;
	bool bChainIsThisNode = true;

	for (UEdGraphPin* Pin : Pins)
	{
		if (Pin->Direction != EGPD_Output || Pin->PinName == DefaultPinName || Pin->bOrphanedPin)
		{
			continue;
		}

		UK2Node_CallFunction* Compare = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
		Compare->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UKzUIFunctionLibrary, IsWidgetNamed), UKzUIFunctionLibrary::StaticClass());
		Compare->AllocateDefaultPins();
		CompilerContext.CopyPinLinksToIntermediate(*OptionPin, *Compare->FindPinChecked(TEXT("Widget")));
		Compare->FindPinChecked(TEXT("Name"))->DefaultValue = Pin->PinName.ToString();

		UK2Node_IfThenElse* Branch = CompilerContext.SpawnIntermediateNode<UK2Node_IfThenElse>(this, SourceGraph);
		Branch->AllocateDefaultPins();
		Schema->TryCreateConnection(Compare->GetReturnValuePin(), Branch->GetConditionPin());

		if (bChainIsThisNode)
		{
			CompilerContext.MovePinLinksToIntermediate(*ChainPin, *Branch->GetExecPin());
			bChainIsThisNode = false;
		}
		else
		{
			Schema->TryCreateConnection(ChainPin, Branch->GetExecPin());
		}

		CompilerContext.MovePinLinksToIntermediate(*Pin, *Branch->GetThenPin());
		ChainPin = Branch->GetElsePin();
	}

	UEdGraphPin* DefaultPin = FindPinChecked(DefaultPinName, EGPD_Output);
	if (bChainIsThisNode)
	{
		// No option pins at all: pass straight through to Default via an always-false branch
		UK2Node_IfThenElse* Branch = CompilerContext.SpawnIntermediateNode<UK2Node_IfThenElse>(this, SourceGraph);
		Branch->AllocateDefaultPins();
		Branch->GetConditionPin()->DefaultValue = TEXT("false");
		CompilerContext.MovePinLinksToIntermediate(*ExecPin, *Branch->GetExecPin());
		CompilerContext.MovePinLinksToIntermediate(*DefaultPin, *Branch->GetElsePin());
	}
	else
	{
		// Route the fallthrough of the last branch to whatever Default is linked to
		UK2Node_IfThenElse* LastBranch = CastChecked<UK2Node_IfThenElse>(ChainPin->GetOwningNode());
		CompilerContext.MovePinLinksToIntermediate(*DefaultPin, *LastBranch->GetElsePin());
	}

	BreakAllNodeLinks();
}

TArray<FName> UK2Node_SwitchOnKzOption::GetOptionNames() const
{
	return CachedOptionNames;
}

bool UK2Node_SwitchOnKzOption::RefreshOptions()
{
	TArray<FName> Names;
	if (!CollectOptionNames(Names) || Names == CachedOptionNames)
	{
		return false;
	}
	ReconstructNode();
	return true;
}

void UK2Node_SwitchOnKzOption::RefreshCachedOptionNames()
{
	// Without the trees (the Blueprint is loading) the serialized names stand
	TArray<FName> Names;
	if (CollectOptionNames(Names))
	{
		CachedOptionNames = MoveTemp(Names);
		DisabledOptions.RemoveAll([this](const FName Name) { return !CachedOptionNames.Contains(Name); });
	}
}

bool UK2Node_SwitchOnKzOption::CollectOptionNames(TArray<FName>& OutNames, bool* bOutSkippedDefault) const
{
	// The source trees and not the generated class, whose tree only changes at the end of a compile: designer
	// edits show right away. Nothing is read while loading, when Blueprint events must not run on templates.
	const UBaseWidgetBlueprint* Blueprint = Cast<UBaseWidgetBlueprint>(GetBlueprint());
	if (!Blueprint || Blueprint->bIsRegeneratingOnLoad)
	{
		return false;
	}

	// A child Widget Blueprint draws the tree of the nearest parent that has one and only fills its named slots
	const UWidgetTree* RootTree = nullptr;
	for (const UBaseWidgetBlueprint* Current = Blueprint; Current && !RootTree; Current = Cast<UBaseWidgetBlueprint>(UBlueprint::GetBlueprintFromClass(Current->ParentClass)))
	{
		if (Current->WidgetTree && Current->WidgetTree->RootWidget)
		{
			RootTree = Current->WidgetTree;
		}
	}
	if (!RootTree)
	{
		return false;
	}

	OutNames.Reset();
	auto AddOption = [&OutNames, bOutSkippedDefault](UWidget* Widget)
		{
			if (!IsAcceptableOption(Widget))
			{
				return;
			}
			if (Widget->GetFName() == DefaultPinName)
			{
				if (bOutSkippedDefault)
				{
					*bOutSkippedDefault = true;
				}
				return;
			}
			OutNames.AddUnique(Widget->GetFName());
		};
	RootTree->ForEachWidget(AddOption);
	if (Blueprint->WidgetTree && Blueprint->WidgetTree != RootTree)
	{
		Blueprint->WidgetTree->ForEachWidget(AddOption);
	}
	return true;
}
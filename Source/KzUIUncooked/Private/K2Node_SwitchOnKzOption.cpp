// Copyright 2026 kirzo

#include "K2Node_SwitchOnKzOption.h"

#include "KzNavigableWidget.h"
#include "KzSelectableWidgetInterface.h"
#include "KzUIFunctionLibrary.h"

#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "K2Node_CallFunction.h"
#include "K2Node_IfThenElse.h"
#include "KismetCompiler.h"

namespace
{
	const FName OptionPinName(TEXT("Option"));
	const FName DefaultPinName(TEXT("Default"));
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
	return INVTEXT("Routes the execution to the pin matching the given option widget. Pins are generated from the selectable widgets of this Blueprint and refresh on compile.");
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

void UK2Node_SwitchOnKzOption::RefreshCachedOptionNames()
{
	const UBlueprint* Blueprint = GetBlueprint();
	UClass* GeneratedClass = Blueprint ? *Blueprint->GeneratedClass : nullptr;
	const UWidgetBlueprintGeneratedClass* WidgetClass = Cast<UWidgetBlueprintGeneratedClass>(GeneratedClass);
	const UWidgetTree* Tree = WidgetClass ? WidgetClass->GetWidgetTreeArchetype() : nullptr;

	// An absent or rootless tree means the class is mid-load or mid-compile: keep the serialized names
	if (!Tree || !Tree->RootWidget)
	{
		return;
	}

	CachedOptionNames.Reset();
	Tree->ForEachWidget([this](UWidget* Widget)
		{
			if (Widget->Implements<UKzSelectableWidgetInterface>() && IKzSelectableWidgetInterface::Execute_IsSelectable(Widget))
			{
				CachedOptionNames.Add(Widget->GetFName());
			}
		}
	);
}
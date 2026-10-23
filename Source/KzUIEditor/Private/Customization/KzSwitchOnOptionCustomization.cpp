// Copyright 2026 kirzo

#include "KzSwitchOnOptionCustomization.h"

#include "K2Node_SwitchOnKzOption.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "ScopedTransaction.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Text/STextBlock.h"

TSharedRef<IDetailCustomization> FKzSwitchOnOptionCustomization::MakeInstance()
{
	return MakeShared<FKzSwitchOnOptionCustomization>();
}

void FKzSwitchOnOptionCustomization::CustomizeDetails(const TSharedPtr<IDetailLayoutBuilder>& DetailBuilder)
{
	LayoutBuilder = DetailBuilder;
	CustomizeDetails(*DetailBuilder);
}

void FKzSwitchOnOptionCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> Objects;
	DetailBuilder.GetObjectsBeingCustomized(Objects);
	if (Objects.Num() != 1)
	{
		return;
	}

	UK2Node_SwitchOnKzOption* Node = Cast<UK2Node_SwitchOnKzOption>(Objects[0].Get());
	if (!Node)
	{
		return;
	}

	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory("Options", INVTEXT("Options"));
	const TWeakObjectPtr<UK2Node_SwitchOnKzOption> WeakNode = Node;
	const TWeakPtr<IDetailLayoutBuilder> WeakLayout = LayoutBuilder;

	if (Node->GetOptionNames().IsEmpty())
	{
		Category.AddCustomRow(INVTEXT("No options"))
		.WholeRowContent()
		[
			SNew(STextBlock)
			.Text(INVTEXT("No selectable widgets found. Options are the widgets of this Blueprint that are selectable and can be accepted."))
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.AutoWrapText(true)
		];
		return;
	}

	for (const FName OptionName : Node->GetOptionNames())
	{
		Category.AddCustomRow(FText::FromName(OptionName))
		.NameContent()
		[
			SNew(STextBlock)
			.Text(FText::FromName(OptionName))
			.Font(IDetailLayoutBuilder::GetDetailFont())
		]
		.ValueContent()
		[
			SNew(SCheckBox)
			.IsChecked_Lambda([WeakNode, OptionName]()
				{
					const UK2Node_SwitchOnKzOption* Node = WeakNode.Get();
					return Node && !Node->DisabledOptions.Contains(OptionName) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
				}
			)
			.OnCheckStateChanged_Lambda([WeakNode, WeakLayout, OptionName](ECheckBoxState State)
				{
					UK2Node_SwitchOnKzOption* Node = WeakNode.Get();
					if (!Node)
					{
						return;
					}

					const FScopedTransaction Transaction(INVTEXT("Toggle Switch Option Pin"));
					const TArray<FName> OptionsBefore = Node->GetOptionNames();
					Node->Modify();
					if (State == ECheckBoxState::Checked)
					{
						Node->DisabledOptions.Remove(OptionName);
					}
					else
					{
						Node->DisabledOptions.AddUnique(OptionName);
					}
					Node->ReconstructNode();
					FBlueprintEditorUtils::MarkBlueprintAsModified(Node->GetBlueprint());

					// Reconstructing reads the widget tree again, which may have changed since the rows were built
					const TSharedPtr<IDetailLayoutBuilder> Layout = WeakLayout.Pin();
					if (Layout && Node->GetOptionNames() != OptionsBefore)
					{
						Layout->ForceRefreshDetails();
					}
				}
			)
		];
	}
}
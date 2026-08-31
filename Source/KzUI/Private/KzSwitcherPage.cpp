// Copyright 2026 kirzo

#include "KzSwitcherPage.h"

#include "Widgets/Layout/SBox.h"
#include "Widgets/SNullWidget.h"

TSharedRef<SWidget> UKzSwitcherPage::RebuildWidget()
{
	MyBox = SNew(SBox);
	if (UWidget* Content = GetContent())
	{
		MyBox->SetContent(Content->TakeWidget());
	}
	return MyBox.ToSharedRef();
}

void UKzSwitcherPage::OnSlotAdded(UPanelSlot* InSlot)
{
	if (MyBox.IsValid() && InSlot->Content)
	{
		MyBox->SetContent(InSlot->Content->TakeWidget());
	}
}

void UKzSwitcherPage::OnSlotRemoved(UPanelSlot* InSlot)
{
	if (MyBox.IsValid())
	{
		MyBox->SetContent(SNullWidget::NullWidget);
	}
}

void UKzSwitcherPage::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyBox.Reset();
}

#if WITH_EDITOR
const FText UKzSwitcherPage::GetPaletteCategory()
{
	return INVTEXT("Kz UI");
}
#endif
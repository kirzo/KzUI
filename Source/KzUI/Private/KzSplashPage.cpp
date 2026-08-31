// Copyright 2026 kirzo

#include "KzSplashPage.h"

#include "Widgets/Layout/SBox.h"
#include "Widgets/SNullWidget.h"

TSharedRef<SWidget> UKzSplashPage::RebuildWidget()
{
	MyBox = SNew(SBox);
	if (UWidget* Content = GetContent())
	{
		MyBox->SetContent(Content->TakeWidget());
	}
	return MyBox.ToSharedRef();
}

void UKzSplashPage::OnSlotAdded(UPanelSlot* InSlot)
{
	if (MyBox.IsValid() && InSlot->Content)
	{
		MyBox->SetContent(InSlot->Content->TakeWidget());
	}
}

void UKzSplashPage::OnSlotRemoved(UPanelSlot* InSlot)
{
	if (MyBox.IsValid())
	{
		MyBox->SetContent(SNullWidget::NullWidget);
	}
}

void UKzSplashPage::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyBox.Reset();
}

#if WITH_EDITOR
const FText UKzSplashPage::GetPaletteCategory()
{
	return INVTEXT("Kz UI");
}
#endif
// Copyright 2026 kirzo

#include "KzUIFlipbookCustomization.h"
#include "KzUIFlipbook.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Engine/Texture2D.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"

TSharedRef<IDetailCustomization> FKzUIFlipbookCustomization::MakeInstance()
{
	return MakeShared<FKzUIFlipbookCustomization>();
}

void FKzUIFlipbookCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> Objects;
	DetailBuilder.GetObjectsBeingCustomized(Objects);
	const TWeakObjectPtr<UKzUIFlipbook> WeakFlipbook = Objects.Num() == 1 ? Cast<UKzUIFlipbook>(Objects[0].Get()) : nullptr;
	if (!WeakFlipbook.IsValid())
	{
		return;
	}

	// The brush lives as long as the image's timer, which owns it
	const TSharedRef<FSlateBrush> Brush = MakeShared<FSlateBrush>();
	const TSharedRef<SImage> Image = SNew(SImage).Image(&Brush.Get());
	const TWeakPtr<SImage> WeakImage = Image;

	// Loops on the editor clock at the flipbook's own rate, whatever bLooping says, and follows property edits live
	Image->RegisterActiveTimer(0.0f, FWidgetActiveTimerDelegate::CreateLambda([WeakFlipbook, Brush, WeakImage](double CurrentTime, float)
		{
			const UKzUIFlipbook* Flipbook = WeakFlipbook.Get();
			if (!Flipbook)
			{
				return EActiveTimerReturnType::Stop;
			}

			UTexture2D* Texture = nullptr;
			FBox2D UVs(ForceInit);
			const int32 Frame = Flipbook->TotalFrames > 0 ? static_cast<int32>(static_cast<int64>(CurrentTime * Flipbook->FramesPerSecond) % Flipbook->TotalFrames) : 0;
			if (Flipbook->GetFrame(Frame, Texture, UVs))
			{
				Brush->SetResourceObject(Texture);
				Brush->SetUVRegion(UVs);
				Brush->ImageSize = Flipbook->FrameSize;
				Brush->DrawAs = ESlateBrushDrawType::Image;
			}
			else
			{
				Brush->DrawAs = ESlateBrushDrawType::NoDrawType;
			}

			if (const TSharedPtr<SImage> PinnedImage = WeakImage.Pin())
			{
				PinnedImage->Invalidate(EInvalidateWidgetReason::LayoutAndVolatility);
			}
			return EActiveTimerReturnType::Continue;
		}
	));

	DetailBuilder.EditCategory("Preview", INVTEXT("Preview"), ECategoryPriority::Important)
	.AddCustomRow(INVTEXT("Preview"))
	.WholeRowContent()
	[
		SNew(SBox)
		.HeightOverride(128.0f)
		.HAlign(HAlign_Center)
		[
			SNew(SScaleBox)
			.Stretch(EStretch::ScaleToFit)
			[
				Image
			]
		]
	];
}
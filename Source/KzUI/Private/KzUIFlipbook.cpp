// Copyright 2026 kirzo

#include "KzUIFlipbook.h"
#include "Engine/Texture2D.h"
#include "Misc/DataValidation.h"

#define LOCTEXT_NAMESPACE "KzUI"

#if WITH_EDITOR
EDataValidationResult UKzUIFlipbook::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	if (FrameSize.X <= 0.0 || FrameSize.Y <= 0.0)
	{
		Context.AddError(LOCTEXT("FlipbookFrameSize", "FrameSize must be positive."));
		return EDataValidationResult::Invalid;
	}

	// Counted the way GetFrame walks the pages
	int32 Capacity = 0;
	for (const UTexture2D* Page : Textures)
	{
		if (!Page)
		{
			Context.AddError(LOCTEXT("FlipbookEmptyTexture", "A texture slot is empty: the frames past it never show."));
			return EDataValidationResult::Invalid;
		}
		Capacity += FMath::Max(1, FMath::FloorToInt(Page->GetSizeX() / FrameSize.X)) * FMath::Max(1, FMath::FloorToInt(Page->GetSizeY() / FrameSize.Y));
	}
	if (TotalFrames > Capacity)
	{
		Context.AddError(FText::Format(LOCTEXT("FlipbookTooManyFrames", "TotalFrames is {0}, but the textures only fit {1} frames of {2} x {3}."), TotalFrames, Capacity, FrameSize.X, FrameSize.Y));
		Result = EDataValidationResult::Invalid;
	}
	return Result;
}
#endif

bool UKzUIFlipbook::GetFrame(int32 Frame, UTexture2D*& OutTexture, FBox2D& OutUVRegion) const
{
	if (Textures.Num() == 0 || TotalFrames <= 0 || FrameSize.X <= 0.0 || FrameSize.Y <= 0.0)
	{
		return false;
	}

	Frame = FMath::Clamp(Frame, 0, TotalFrames - 1);

	for (UTexture2D* Page : Textures)
	{
		if (!Page)
		{
			return false;
		}

		const int32 Columns = FMath::Max(1, FMath::FloorToInt(Page->GetSizeX() / FrameSize.X));
		const int32 Rows = FMath::Max(1, FMath::FloorToInt(Page->GetSizeY() / FrameSize.Y));
		const int32 Capacity = Columns * Rows;

		if (Frame < Capacity)
		{
			const FVector2D TextureSize(Page->GetSizeX(), Page->GetSizeY());
			const FVector2D Min(FrameSize.X * (Frame % Columns), FrameSize.Y * (Frame / Columns));
			OutUVRegion = FBox2D(Min / TextureSize, (Min + FrameSize) / TextureSize);
			OutUVRegion.bIsValid = true;
			OutTexture = Page;
			return true;
		}
		Frame -= Capacity;
	}
	return false;
}

#undef LOCTEXT_NAMESPACE
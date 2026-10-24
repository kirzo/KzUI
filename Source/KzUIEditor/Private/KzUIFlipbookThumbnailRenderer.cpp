// Copyright 2026 kirzo

#include "KzUIFlipbookThumbnailRenderer.h"
#include "KzUIFlipbook.h"

#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "Engine/Texture2D.h"

/** First frame of the flipbook, when it has one ready to draw. */
static UTexture2D* GetFirstFrame(UObject* Object, FBox2D& OutUVs)
{
	UTexture2D* Texture = nullptr;
	const UKzUIFlipbook* Flipbook = Cast<UKzUIFlipbook>(Object);
	return Flipbook && Flipbook->GetFrame(0, Texture, OutUVs) && Texture && Texture->GetResource() ? Texture : nullptr;
}

bool UKzUIFlipbookThumbnailRenderer::CanVisualizeAsset(UObject* Object)
{
	FBox2D UVs(ForceInit);
	return GetFirstFrame(Object, UVs) != nullptr;
}

void UKzUIFlipbookThumbnailRenderer::Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget* Viewport, FCanvas* Canvas, bool bAdditionalViewFamily)
{
	FBox2D UVs(ForceInit);
	UTexture2D* Texture = GetFirstFrame(Object, UVs);
	if (!Texture)
	{
		return;
	}

	// Fitted inside the square thumbnail with the frame's own proportions
	const FVector2D FrameSize = CastChecked<UKzUIFlipbook>(Object)->FrameSize;
	const double Scale = FMath::Min(Width / FrameSize.X, Height / FrameSize.Y);
	const FVector2D Size = FrameSize * Scale;
	const FVector2D Position(X + (Width - Size.X) * 0.5, Y + (Height - Size.Y) * 0.5);

	FCanvasTileItem Tile(Position, Texture->GetResource(), Size, UVs.Min, UVs.Max, FLinearColor::White);
	Tile.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Tile);
}
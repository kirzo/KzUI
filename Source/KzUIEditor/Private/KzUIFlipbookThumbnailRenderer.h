// Copyright 2026 kirzo

#pragma once

#include "ThumbnailRendering/DefaultSizedThumbnailRenderer.h"
#include "KzUIFlipbookThumbnailRenderer.generated.h"

/** Content Browser thumbnail of a UI flipbook: its first frame, keeping the frame's proportions. */
UCLASS()
class UKzUIFlipbookThumbnailRenderer : public UDefaultSizedThumbnailRenderer
{
	GENERATED_BODY()

public:
	virtual bool CanVisualizeAsset(UObject* Object) override;
	virtual void Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget* Viewport, FCanvas* Canvas, bool bAdditionalViewFamily) override;
};
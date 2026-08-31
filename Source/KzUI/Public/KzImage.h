// Copyright 2026 kirzo

#pragma once

#include "Components/Image.h"
#include "Engine/LatentActionManager.h"
#include "KzImage.generated.h"

class UKzImage;
class UKzUIFlipbook;
class UMaterialInterface;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKzImageSignature, UKzImage*, Image);

/**
 * UImage with soft-texture streaming helpers (hide-while-streaming, load queries, latent wait)
 * and optional flipbook playback. Looping flipbooks play automatically, synced to a global
 * clock so every instance of the same flipbook stays in phase.
 */
UCLASS(meta = (PrioritizeCategories = "Image"))
class KZUI_API UKzImage : public UImage
{
	GENERATED_BODY()

public:
	/** Hide the brush while a soft texture is streaming in. */
	UPROPERTY(Category = Image, EditAnywhere, BlueprintReadWrite)
	bool bHideWhenStreaming = false;

	/** Flipbook driving the brush. Null shows the plain brush. */
	UPROPERTY(Category = Image, EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UKzUIFlipbook> Flipbook;

	UPROPERTY(Category = Image, BlueprintAssignable)
	FKzImageSignature ImageStreamingComplete;

	/** Broadcast when a non-looping flipbook reaches its last frame. */
	UPROPERTY(Category = Image, BlueprintAssignable)
	FKzImageSignature OnFlipbookFinished;

private:
	bool bPlayingFlipbook = false;
	float PlaybackTime = 0.0f;
	int32 LastAppliedFrame = INDEX_NONE;

protected:
	//~ Begin UWidget Interface
	virtual TSharedRef<SWidget> RebuildWidget() override;

public:
	virtual void SynchronizeProperties() override;
#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif
	//~ End UWidget Interface

protected:
	//~ Begin UImage Interface
	virtual void SetBrushFromSoftTexture(TSoftObjectPtr<UTexture2D> SoftTexture, bool bMatchSize = false) override;
	virtual void OnImageStreamingComplete(TSoftObjectPtr<UObject> LoadedSoftObject) override;
	//~ End UImage Interface

public:
	/** Sets the image from a texture, a material or a flipbook. */
	UFUNCTION(Category = "KzUI|Image", BlueprintCallable)
	void SetImage(UObject* Image);

	/** Sets a scalar parameter on the brush's dynamic material, creating the instance on demand. Safe to call every update: it survives icon refreshes on device changes. */
	UFUNCTION(Category = "KzUI|Image", BlueprintCallable)
	void SetMaterialScalarParameter(FName ParameterName, float Value);

	UFUNCTION(Category = "KzUI|Image", BlueprintCallable)
	void SetFlipbook(UKzUIFlipbook* NewFlipbook);

	/** Restarts flipbook playback; only needed for non-looping flipbooks or after Stop. */
	UFUNCTION(Category = "KzUI|Image", BlueprintCallable)
	void Play();

	/** Stops flipbook playback and shows the first frame. */
	UFUNCTION(Category = "KzUI|Image", BlueprintCallable)
	void Stop();

	/** Stops playback and shows the given frame. */
	UFUNCTION(Category = "KzUI|Image", BlueprintCallable)
	void SetCurrentFrame(int32 Frame);

	UFUNCTION(Category = "KzUI|Image", BlueprintPure)
	bool IsPlaying() const { return bPlayingFlipbook; }

	void TickFlipbook(double CurrentTime, float DeltaSeconds);

	/** Copies the brush or flipbook, the in-flight streaming request and the visibility to another image. */
	UFUNCTION(Category = "KzUI|Image", BlueprintCallable)
	virtual void CopyTo(UKzImage* Other);

	UFUNCTION(Category = "KzUI|Image", BlueprintPure)
	virtual bool AreBrushTexturesLoaded() const;

	UFUNCTION(Category = "KzUI|Image", BlueprintCallable)
	virtual void ClearTextures();

	UFUNCTION(Category = "KzUI|Image", BlueprintPure)
	virtual bool IsStreaming() const;

	UFUNCTION(Category = "KzUI|Image", BlueprintCallable, meta = (Latent, LatentInfo = "LatentInfo", WorldContext = "WorldContextObject"))
	static void WaitUntilBrushTexturesAreLoaded(const UObject* WorldContextObject, const TArray<UKzImage*>& Images, FLatentActionInfo LatentInfo);

private:
	void RefreshFlipbook();
	void ApplyFlipbookFrame(int32 Frame);
};
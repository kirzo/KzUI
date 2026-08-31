// Copyright 2026 kirzo

#include "KzImage.h"
#include "KzUIFlipbook.h"

#include "Algo/Count.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "LatentActions.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Widgets/Images/SImage.h"

struct FKzWaitUntilTexturesLoaded : public FPendingLatentAction
{
public:
	TArray<UKzImage*> Images;
	FName ExecutionFunction;
	int32 OutputLink;
	FWeakObjectPtr CallbackTarget;

	FKzWaitUntilTexturesLoaded(const TArray<UKzImage*>& InImages, const FLatentActionInfo& InLatentInfo)
		: Images(InImages)
		, ExecutionFunction(InLatentInfo.ExecutionFunction)
		, OutputLink(InLatentInfo.Linkage)
		, CallbackTarget(InLatentInfo.CallbackTarget)
	{
	}

	virtual void UpdateOperation(FLatentResponse& Response) override
	{
		const bool bLoading = Algo::CountIf(Images, [](UKzImage* Image) { return IsValid(Image) && !Image->AreBrushTexturesLoaded(); }) > 0;
		Response.FinishAndTriggerIf(!bLoading, ExecutionFunction, OutputLink, CallbackTarget);
	}
};

/** SImage that ticks its owning UKzImage while a flipbook is playing. */
class SKzImage : public SImage
{
public:
	SLATE_BEGIN_ARGS(SKzImage)
	{}
	SLATE_END_ARGS()

	TWeakObjectPtr<UKzImage> Owner;

	SKzImage()
	{
		SetCanTick(false);
	}

	void Construct(const FArguments& InArgs)
	{
		SImage::Construct(SImage::FArguments());
	}

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override
	{
		SImage::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

		if (UKzImage* Image = Owner.Get())
		{
			Image->TickFlipbook(InCurrentTime, InDeltaTime);
		}
	}
};

TSharedRef<SWidget> UKzImage::RebuildWidget()
{
	TSharedRef<SKzImage> NewImage = SNew(SKzImage);
	NewImage->Owner = this;
	MyImage = NewImage;
	RefreshFlipbook();
	return NewImage;
}

void UKzImage::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	RefreshFlipbook();
}

#if WITH_EDITOR
const FText UKzImage::GetPaletteCategory()
{
	return INVTEXT("Kz UI");
}
#endif

void UKzImage::SetBrushFromSoftTexture(TSoftObjectPtr<UTexture2D> SoftTexture, bool bMatchSize)
{
	if (bHideWhenStreaming)
	{
		FSlateBrush NewBrush = GetBrush();
		NewBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
		SetBrush(NewBrush);
	}

	Super::SetBrushFromSoftTexture(SoftTexture, bMatchSize);

	// The engine skips OnImageStreamingComplete when the texture was already loaded
	if (SoftTexture.Get())
	{
		OnImageStreamingComplete(SoftTexture);
	}
}

void UKzImage::OnImageStreamingComplete(TSoftObjectPtr<UObject> LoadedSoftObject)
{
	if (bHideWhenStreaming)
	{
		FSlateBrush NewBrush = GetBrush();
		NewBrush.DrawAs = ESlateBrushDrawType::Image;
		SetBrush(NewBrush);
	}

	ImageStreamingComplete.Broadcast(this);
}

void UKzImage::SetMaterialScalarParameter(FName ParameterName, float Value)
{
	if (UMaterialInstanceDynamic* Material = GetDynamicMaterial())
	{
		Material->SetScalarParameterValue(ParameterName, Value);
	}
}

void UKzImage::SetImage(UObject* Image)
{
	if (UKzUIFlipbook* AsFlipbook = Cast<UKzUIFlipbook>(Image))
	{
		SetFlipbook(AsFlipbook);
		return;
	}

	SetFlipbook(nullptr);
	if (UTexture2D* AsTexture = Cast<UTexture2D>(Image))
	{
		SetBrushFromTexture(AsTexture);
	}
	else if (UMaterialInterface* AsMaterial = Cast<UMaterialInterface>(Image))
	{
		SetBrushFromMaterial(AsMaterial);
	}
}

void UKzImage::SetFlipbook(UKzUIFlipbook* NewFlipbook)
{
	Flipbook = NewFlipbook;
	RefreshFlipbook();
}

void UKzImage::Play()
{
	if (Flipbook)
	{
		PlaybackTime = 0.0f;
		LastAppliedFrame = INDEX_NONE;
		bPlayingFlipbook = true;
		if (MyImage.IsValid())
		{
			MyImage->SetCanTick(true);
		}
	}
}

void UKzImage::Stop()
{
	bPlayingFlipbook = false;
	ApplyFlipbookFrame(0);
	if (MyImage.IsValid())
	{
		MyImage->SetCanTick(false);
	}
}

void UKzImage::SetCurrentFrame(int32 Frame)
{
	bPlayingFlipbook = false;
	ApplyFlipbookFrame(Frame);
	if (MyImage.IsValid())
	{
		MyImage->SetCanTick(false);
	}
}

void UKzImage::TickFlipbook(double CurrentTime, float DeltaSeconds)
{
	if (!bPlayingFlipbook || !Flipbook)
	{
		return;
	}

	if (Flipbook->bLooping)
	{
		// The global clock keeps every instance of the same flipbook in phase
		ApplyFlipbookFrame(int32(CurrentTime * Flipbook->FramesPerSecond) % Flipbook->TotalFrames);
		return;
	}

	PlaybackTime += DeltaSeconds;
	const int32 Frame = FMath::FloorToInt(PlaybackTime * Flipbook->FramesPerSecond);
	if (Frame >= Flipbook->TotalFrames)
	{
		ApplyFlipbookFrame(Flipbook->TotalFrames - 1);
		bPlayingFlipbook = false;
		if (MyImage.IsValid())
		{
			MyImage->SetCanTick(false);
		}
		OnFlipbookFinished.Broadcast(this);
		return;
	}
	ApplyFlipbookFrame(Frame);
}

void UKzImage::RefreshFlipbook()
{
	PlaybackTime = 0.0f;
	LastAppliedFrame = INDEX_NONE;
	bPlayingFlipbook = Flipbook && Flipbook->bLooping;

	if (Flipbook && !bPlayingFlipbook)
	{
		ApplyFlipbookFrame(0);
	}
	if (MyImage.IsValid())
	{
		MyImage->SetCanTick(bPlayingFlipbook);
	}
}

void UKzImage::ApplyFlipbookFrame(int32 Frame)
{
	if (!Flipbook || Frame == LastAppliedFrame)
	{
		return;
	}

	UTexture2D* Page = nullptr;
	FBox2D UVRegion(ForceInit);
	if (!Flipbook->GetFrame(Frame, Page, UVRegion))
	{
		return;
	}

	LastAppliedFrame = Frame;

	FSlateBrush NewBrush = GetBrush();
	NewBrush.SetResourceObject(Page);
	NewBrush.SetUVRegion(UVRegion);
	NewBrush.ImageSize = Flipbook->FrameSize;
	SetBrush(NewBrush);
}

void UKzImage::CopyTo(UKzImage* Other)
{
	if (IsValid(Other))
	{
		if (Flipbook)
		{
			Other->SetFlipbook(Flipbook);
		}
		else if (IsStreaming())
		{
			Other->Super::SetBrushFromSoftTexture(TSoftObjectPtr<UTexture2D>(StreamingObjectPath));
		}
		else
		{
			Other->SetFlipbook(nullptr);
			Other->SetBrush(GetBrush());
		}

		Other->SetVisibility(GetVisibility());
	}
}

bool UKzImage::AreBrushTexturesLoaded() const
{
	return IsValid(GetBrush().GetResourceObject()) && !IsStreaming();
}

void UKzImage::ClearTextures()
{
	SetBrushResourceObject(nullptr);
}

bool UKzImage::IsStreaming() const
{
	return !UAssetManager::GetStreamableManager().IsAsyncLoadComplete(StreamingObjectPath);
}

void UKzImage::WaitUntilBrushTexturesAreLoaded(const UObject* WorldContextObject, const TArray<UKzImage*>& Images, FLatentActionInfo LatentInfo)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		FLatentActionManager& LatentManager = World->GetLatentActionManager();
		FKzWaitUntilTexturesLoaded* NewAction = new FKzWaitUntilTexturesLoaded(Images, LatentInfo);
		LatentManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, NewAction);
	}
}
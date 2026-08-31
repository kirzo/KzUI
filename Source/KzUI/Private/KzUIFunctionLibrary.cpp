// Copyright 2026 kirzo

#include "KzUIFunctionLibrary.h"
#include "KzUIInputSettings.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Kismet/GameplayStatics.h"
#include "LatentActions.h"

struct FKzLoadWidgetAction : public FPendingLatentAction
{
public:
	FSoftObjectPath SoftObjectPath;
	TSharedPtr<FStreamableHandle> Handle;
	FName ExecutionFunction;
	int32 OutputLink;
	FWeakObjectPtr CallbackTarget;
	TFunction<void(TSubclassOf<UUserWidget>)> Callback;

	FKzLoadWidgetAction(const FSoftObjectPath& InSoftObjectPath, const TFunction<void(TSubclassOf<UUserWidget>)>& InCallback, const FLatentActionInfo& InLatentInfo)
		: SoftObjectPath(InSoftObjectPath)
		, ExecutionFunction(InLatentInfo.ExecutionFunction)
		, OutputLink(InLatentInfo.Linkage)
		, CallbackTarget(InLatentInfo.CallbackTarget)
		, Callback(InCallback)
	{
		Handle = UAssetManager::GetStreamableManager().RequestAsyncLoad(SoftObjectPath);
	}

	virtual ~FKzLoadWidgetAction()
	{
		if (Handle.IsValid())
		{
			Handle->ReleaseHandle();
		}
	}

	virtual void UpdateOperation(FLatentResponse& Response) override
	{
		const bool bLoaded = !Handle.IsValid() || Handle->HasLoadCompleted() || Handle->WasCanceled();
		if (bLoaded)
		{
			Callback(Cast<UClass>(SoftObjectPath.ResolveObject()));
		}
		Response.FinishAndTriggerIf(bLoaded, ExecutionFunction, OutputLink, CallbackTarget);
	}

#if WITH_EDITOR
	virtual FString GetDescription() const override
	{
		return FString::Printf(TEXT("Create Widget: %s"), *SoftObjectPath.ToString());
	}
#endif
};

bool UKzUIFunctionLibrary::IsWidgetNamed(const UWidget* Widget, FName Name)
{
	return Widget && Widget->GetFName() == Name;
}

EKzUIInputType UKzUIFunctionLibrary::GetInputFromKeyEvent(const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	for (const auto& Pair : UKzUIInputSettings::Get()->InputMap)
	{
		if (Pair.Value.GetInputDefinition().Contains(Key))
		{
			return Pair.Key;
		}
	}
	return EKzUIInputType::None;
}

void UKzUIFunctionLibrary::CreateWidgetAsync(UObject* WorldContextObject, FLatentActionInfo LatentInfo, UUserWidget*& Widget, TSoftClassPtr<UUserWidget> Class, APlayerController* OwningPlayer)
{
	auto CreateWidgetLambda = [WorldContextObject, &Widget, OwningPlayer](TSubclassOf<UUserWidget> WidgetType)
	{
		Widget = UWidgetBlueprintLibrary::Create(WorldContextObject, WidgetType, OwningPlayer);
	};

	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		FLatentActionManager& LatentManager = World->GetLatentActionManager();
		FKzLoadWidgetAction* NewAction = new FKzLoadWidgetAction(Class.ToSoftObjectPath(), CreateWidgetLambda, LatentInfo);
		LatentManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, NewAction);
	}
}

bool UKzUIFunctionLibrary::ProjectWorldLocationToViewport(UUserWidget* Widget, FVector WorldLocation, FVector2D& ViewportPosition, bool bClampInsideViewport, float ClampOffset)
{
	ViewportPosition = FVector2D::ZeroVector;
	APlayerController* PC = Widget->GetOwningPlayer();
	if (!IsValid(PC) || !PC->PlayerCameraManager)
	{
		return false;
	}

	// Mirror locations behind the camera so they still project, then push them off the bottom edge
	const FTransform CameraTransform(PC->PlayerCameraManager->GetCameraRotation(), PC->PlayerCameraManager->GetCameraLocation(), FVector::OneVector);
	FVector LocalPosition = CameraTransform.InverseTransformPositionNoScale(WorldLocation);
	const bool bIsBehind = LocalPosition.X < 0.0f;
	if (bIsBehind)
	{
		LocalPosition.X *= -1.0f;
		WorldLocation = CameraTransform.TransformPositionNoScale(LocalPosition);
	}

	if (UGameplayStatics::ProjectWorldToScreen(PC, WorldLocation, ViewportPosition, true))
	{
		if (bIsBehind)
		{
			ViewportPosition.Y = BIG_NUMBER;
		}

		const FVector2D ViewportSize = Widget->GetCachedGeometry().GetAbsoluteSize() * 0.5f;
		ViewportPosition -= ViewportSize;

		if (bClampInsideViewport)
		{
			const float ClampX = ViewportSize.X - ClampOffset;
			const float ClampY = ViewportSize.Y - ClampOffset;
			ViewportPosition.X = FMath::Clamp(ViewportPosition.X, -ClampX, ClampX);
			ViewportPosition.Y = FMath::Clamp(ViewportPosition.Y, -ClampY, ClampY);
		}

		ViewportPosition /= UWidgetLayoutLibrary::GetViewportScale(PC);
		return true;
	}

	return false;
}
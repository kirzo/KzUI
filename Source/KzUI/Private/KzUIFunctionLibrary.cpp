// Copyright 2026 kirzo

#include "KzUIFunctionLibrary.h"
#include "KzUIInputSettings.h"
#include "KzUIInputSubsystem.h"

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
	const UKzUIInputSubsystem* Subsystem = UKzUIInputSubsystem::FindForSlateUser(InKeyEvent.GetUserIndex());
	const EKzUIInputDevice GamepadDevice = Subsystem ? Subsystem->GetGamepadDevice() : UKzUIInputSubsystem::GetPlatformGamepadDevice();
	const UKzUIInputSettings* Settings = UKzUIInputSettings::Get();
	for (const auto& Pair : Settings->InputMap)
	{
		const FKzUIInputDefinition* Definition = Settings->FindInputDefinition(Pair.Key, GamepadDevice);
		if (Definition && Definition->Contains(Key))
		{
			return Pair.Key;
		}
	}
	return EKzUIInputType::None;
}

void UKzUIFunctionLibrary::CreateWidgetAsync(UObject* WorldContextObject, FLatentActionInfo LatentInfo, UUserWidget*& Widget, TSoftClassPtr<UUserWidget> Class, APlayerController* OwningPlayer)
{
	// The load can outlive the context object or the player, so they are held weakly
	TWeakObjectPtr<UObject> WeakContext = WorldContextObject;
	TWeakObjectPtr<APlayerController> WeakPlayer = OwningPlayer;
	auto CreateWidgetLambda = [WeakContext, &Widget, WeakPlayer](TSubclassOf<UUserWidget> WidgetType)
	{
		if (UObject* Context = WeakContext.Get())
		{
			Widget = UWidgetBlueprintLibrary::Create(Context, WidgetType, WeakPlayer.Get());
		}
	};

	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		// A second call with the same node while loading (a double Accept) must not create a second widget
		FLatentActionManager& LatentManager = World->GetLatentActionManager();
		if (!LatentManager.FindExistingAction<FKzLoadWidgetAction>(LatentInfo.CallbackTarget, LatentInfo.UUID))
		{
			LatentManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, new FKzLoadWidgetAction(Class.ToSoftObjectPath(), CreateWidgetLambda, LatentInfo));
		}
	}
}

bool UKzUIFunctionLibrary::ProjectWorldLocationToViewport(UUserWidget* Widget, FVector WorldLocation, FVector2D& ViewportPosition, bool bClampInsideViewport, float ClampOffset)
{
	ViewportPosition = FVector2D::ZeroVector;
	APlayerController* PC = Widget ? Widget->GetOwningPlayer() : nullptr;
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
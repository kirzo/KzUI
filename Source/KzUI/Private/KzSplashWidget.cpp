// Copyright 2026 kirzo

#include "KzSplashWidget.h"
#include "KzSplashPage.h"
#include "KzUIInputSettings.h"

#include "Components/AudioComponent.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/InputSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Widgets/SViewport.h"

namespace
{
	// Timing for switcher children that are not UKzSplashPage
	constexpr float DefaultPageDuration = 3.0f;
	constexpr float DefaultFadeTime = 0.5f;
}

/**
 * Skips the splash on any key or click aimed at the game, from any device and player, consuming the press so the game
 * underneath ignores button mashing. Releases, shortcuts and input aimed elsewhere (the editor in PIE) always pass.
 */
class FKzSplashInputListener : public IInputProcessor
{
public:
	TWeakObjectPtr<UKzSplashWidget> Owner;

	FKzSplashInputListener()
	{
		ActivationHandle = FSlateApplication::Get().OnApplicationActivationStateChanged().AddRaw(this, &FKzSplashInputListener::OnActivationChanged);
	}

	virtual ~FKzSplashInputListener() override
	{
		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().OnApplicationActivationStateChanged().Remove(ActivationHandle);
		}
	}

	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override {}

	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override
	{
		// Shortcuts (Alt+Tab, Alt+Enter, Shift+F1), the console and Escape to stop PIE keep working during the splash
		const FKey Key = InKeyEvent.GetKey();
		if (Key.IsModifierKey() || InKeyEvent.GetModifierKeys().AnyModifiersDown() || GetDefault<UInputSettings>()->ConsoleKeys.Contains(Key) || (GIsEditor && Key == EKeys::Escape))
		{
			return false;
		}

		const TSharedPtr<SViewport> Viewport = GetGameViewportWidget();
		return Viewport && IsGameFocus(SlateApp.GetUserFocusedWidget(InKeyEvent.GetUserIndex()), Viewport) && TrySkip(SlateApp, InKeyEvent.IsRepeat());
	}

	virtual bool HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override
	{
		return HandlePointerDown(SlateApp, MouseEvent);
	}

	virtual bool HandleMouseButtonDoubleClickEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override
	{
		return HandlePointerDown(SlateApp, MouseEvent);
	}

private:
	/** Slate time of the last app activation: the click that brings the window back does not skip. */
	double ActivationTime = -1.0;

	FDelegateHandle ActivationHandle;

	void OnActivationChanged(bool bActive)
	{
		if (bActive)
		{
			ActivationTime = FSlateApplication::Get().GetCurrentTime();
		}
	}

	bool HandlePointerDown(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
	{
		constexpr double ActivationClickWindow = 0.1;
		if (!UKzUIInputSettings::Get()->bSupportsMouse || SlateApp.GetCurrentTime() - ActivationTime < ActivationClickWindow)
		{
			return false;
		}

		const TSharedPtr<SViewport> Viewport = GetGameViewportWidget();
		const FWidgetPath UnderCursor = SlateApp.LocateWindowUnderMouse(MouseEvent.GetScreenSpacePosition(), SlateApp.GetInteractiveTopLevelWindows());
		return Viewport && UnderCursor.IsValid() && UnderCursor.ContainsWidget(Viewport.Get()) && TrySkip(SlateApp, false);
	}

	bool TrySkip(FSlateApplication& SlateApp, bool bRepeat)
	{
		UKzSplashWidget* Splash = Owner.Get();
		if (!Splash || !SlateApp.IsActive())
		{
			return false;
		}
		if (!bRepeat)
		{
			Splash->Skip();
		}
		return true;
	}

	TSharedPtr<SViewport> GetGameViewportWidget() const
	{
		const UKzSplashWidget* Splash = Owner.Get();
		const UWorld* World = Splash ? Splash->GetWorld() : nullptr;
		const UGameViewportClient* ViewportClient = World ? World->GetGameViewport() : nullptr;
		return ViewportClient ? ViewportClient->GetGameViewportWidget() : nullptr;
	}

	/** Whether a user's focused widget lies in the game viewport; a user with no focus at all counts as the game's. */
	static bool IsGameFocus(TSharedPtr<SWidget> Focused, const TSharedPtr<SViewport>& Viewport)
	{
		if (!Focused)
		{
			return true;
		}
		for (; Focused; Focused = Focused->GetParentWidget())
		{
			if (Focused == Viewport)
			{
				return true;
			}
		}
		return false;
	}
};

void UKzSplashWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Pages)
	{
		for (int32 i = 0; i < Pages->GetChildrenCount(); i++)
		{
			if (!Cast<UKzSplashPage>(Pages->GetChildAt(i)))
			{
				UE_LOG(LogKzUI, Warning, TEXT("[%s] Splash page %d is not a KzSplashPage: default timing will be used"), *GetName(), i);
			}
		}
	}

	if (FSlateApplication::IsInitialized() && !InputListener)
	{
		InputListener = MakeShared<FKzSplashInputListener>();
		InputListener->Owner = this;
		FSlateApplication::Get().RegisterInputPreProcessor(InputListener);
	}

	StartPage(0);
}

void UKzSplashWidget::NativeDestruct()
{
	if (InputListener)
	{
		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().UnregisterInputPreProcessor(InputListener);
		}
		InputListener.Reset();
	}
	if (UAudioComponent* Sound = PageSound.Get())
	{
		Sound->Stop();
	}
	Super::NativeDestruct();
}

void UKzSplashWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UWidget* Page = GetCurrentPage();
	if (!Page || State == EKzSplashState::Inactive || State == EKzSplashState::Finished)
	{
		return;
	}

	// Capped so the hitch of the first frames after a load does not swallow a fade-in or a whole hold
	const UKzSplashPage* Config = GetCurrentSplashPage();
	StateTime += FMath::Min(InDeltaTime, 0.1f);

	switch (State)
	{
		case EKzSplashState::FadeIn:
		{
			const float FadeTime = Config ? Config->FadeInTime : DefaultFadeTime;
			Page->SetRenderOpacity(FadeTime > 0.0f ? FMath::Clamp(StateTime / FadeTime, 0.0f, 1.0f) : 1.0f);
			if (StateTime >= FadeTime)
			{
				State = EKzSplashState::Hold;
				StateTime = 0.0f;
			}
			break;
		}
		case EKzSplashState::Hold:
		{
			if (StateTime >= (Config ? Config->Duration : DefaultPageDuration))
			{
				BeginFadeOut();
			}
			break;
		}
		case EKzSplashState::FadeOut:
		{
			const float FadeTime = Config ? Config->FadeOutTime : DefaultFadeTime;
			Page->SetRenderOpacity(FadeTime > 0.0f ? FadeOutStartOpacity * (1.0f - FMath::Clamp(StateTime / FadeTime, 0.0f, 1.0f)) : 0.0f);
			if (StateTime >= FadeTime)
			{
				StartPage(PageIndex + 1);
			}
			break;
		}
		default:
			break;
	}
}

bool UKzSplashWidget::Skip()
{
	if (State != EKzSplashState::FadeIn && State != EKzSplashState::Hold)
	{
		return false;
	}

	const UKzSplashPage* Config = GetCurrentSplashPage();
	if (Config && !Config->bSkippable)
	{
		return false;
	}

	BeginFadeOut();

	// A page that plays out keeps its sound to the end; a skipped one takes it along in its fade
	if (UAudioComponent* Sound = PageSound.Get())
	{
		Sound->FadeOut(Config ? Config->FadeOutTime : DefaultFadeTime, 0.0f);
	}
	return true;
}

void UKzSplashWidget::StartPage(int32 Index)
{
	if (!Pages || Index >= Pages->GetChildrenCount())
	{
		Finish();
		return;
	}

	PageIndex = Index;
	Pages->SetActiveWidgetIndex(Index);
	State = EKzSplashState::FadeIn;
	StateTime = 0.0f;
	PageSound.Reset();

	UWidget* Page = Pages->GetChildAt(Index);
	if (UKzSplashPage* SplashPage = Cast<UKzSplashPage>(Page))
	{
		Page->SetRenderOpacity(SplashPage->FadeInTime > 0.0f ? 0.0f : 1.0f);
		if (SplashPage->Sound)
		{
			PageSound = UGameplayStatics::SpawnSound2D(this, SplashPage->Sound);
		}
		SplashPage->OnShown.Broadcast();
	}
	else
	{
		Page->SetRenderOpacity(0.0f);
	}
	ReceiveOnPageChanged(Index, Page);
}

void UKzSplashWidget::BeginFadeOut()
{
	State = EKzSplashState::FadeOut;
	StateTime = 0.0f;

	UWidget* Page = GetCurrentPage();
	FadeOutStartOpacity = Page ? Page->GetRenderOpacity() : 1.0f;
}

void UKzSplashWidget::Finish()
{
	if (State == EKzSplashState::Finished)
	{
		return;
	}
	State = EKzSplashState::Finished;

	if (InputListener)
	{
		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().UnregisterInputPreProcessor(InputListener);
		}
		InputListener.Reset();
	}

	OnFinished.Broadcast();
	ReceiveOnFinished();
}

UWidget* UKzSplashWidget::GetCurrentPage() const
{
	return Pages && Pages->GetChildrenCount() > PageIndex && PageIndex >= 0 ? Pages->GetChildAt(PageIndex) : nullptr;
}

UKzSplashPage* UKzSplashWidget::GetCurrentSplashPage() const
{
	return Cast<UKzSplashPage>(GetCurrentPage());
}
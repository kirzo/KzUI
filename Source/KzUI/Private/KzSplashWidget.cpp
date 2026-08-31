// Copyright 2026 kirzo

#include "KzSplashWidget.h"
#include "KzSplashPage.h"

#include "Components/WidgetSwitcher.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

DEFINE_LOG_CATEGORY_STATIC(LogKzUI, Log, All);

namespace
{
	// Timing for switcher children that are not UKzSplashPage
	constexpr float DefaultPageDuration = 3.0f;
	constexpr float DefaultFadeTime = 0.5f;
}

/**
 * Skips the splash on any key or click, from any device and any player. While the splash is
 * alive every key and click is consumed, whether it skipped or not: the game underneath must
 * not react to button mashing during the sequence.
 */
class FKzSplashInputListener : public IInputProcessor
{
public:
	TWeakObjectPtr<UKzSplashWidget> Owner;

	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override {}

	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override
	{
		UKzSplashWidget* Splash = Owner.Get();
		if (!Splash)
		{
			return false;
		}
		if (!InKeyEvent.IsRepeat())
		{
			Splash->Skip();
		}
		return true;
	}

	virtual bool HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override
	{
		return Owner.IsValid();
	}

	virtual bool HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override
	{
		UKzSplashWidget* Splash = Owner.Get();
		if (!Splash)
		{
			return false;
		}
		Splash->Skip();
		return true;
	}

	virtual bool HandleMouseButtonUpEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override
	{
		return Owner.IsValid();
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

	const UKzSplashPage* Config = GetCurrentSplashPage();
	StateTime += InDeltaTime;

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

	UWidget* Page = Pages->GetChildAt(Index);
	if (UKzSplashPage* SplashPage = Cast<UKzSplashPage>(Page))
	{
		Page->SetRenderOpacity(SplashPage->FadeInTime > 0.0f ? 0.0f : 1.0f);
		if (SplashPage->Sound)
		{
			UGameplayStatics::PlaySound2D(this, SplashPage->Sound);
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
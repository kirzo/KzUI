// Copyright 2026 kirzo

#include "KzUserWidget.h"
#include "KzPromptWidget.h"
#include "KzUIInputSettings.h"
#include "KzUIInputSubsystem.h"
#include "KzUISoundTheme.h"
#include "KzWidgetSwitcher.h"

#include "Components/CanvasPanelSlot.h"
#include "Components/PanelWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

/** Nearest ancestor KzWidgetSwitcher within the same widget tree, reporting whether Widget sits on its active page. */
static UKzWidgetSwitcher* FindOwningSwitcher(const UWidget* Widget, bool& bOutOnActivePage)
{
	bOutOnActivePage = true;
	const UWidget* Child = Widget;
	for (UWidget* Parent = Widget->GetParent(); Parent; Child = Parent, Parent = Parent->GetParent())
	{
		if (UKzWidgetSwitcher* Switcher = Cast<UKzWidgetSwitcher>(Parent))
		{
			bOutOnActivePage = Switcher->GetActiveWidget() == Child;
			return Switcher;
		}
	}
	return nullptr;
}

#define ADD_INPUT_CALLBACKS(Type) \
	CanHandleInputFunc.Add(EKzUIInputType::Type, [this]() { return CanHandle##Type##Input(); }); \
	InputTriggered.Add(EKzUIInputType::Type, [this](bool bHandled) { Type##InputTriggered(bHandled); }); \
	InputHeld.Add(EKzUIInputType::Type, [this](bool bHandled, float Time) { Type##InputHeld(bHandled, Time); }); \
	InputReleased.Add(EKzUIInputType::Type, [this](bool bHandled) { Type##InputReleased(bHandled); });

UKzUserWidget::UKzUserWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);

	ContinuousInputMask =
		(1 << (uint8)EKzUIInputType::Down) |
		(1 << (uint8)EKzUIInputType::Up) |
		(1 << (uint8)EKzUIInputType::Left) |
		(1 << (uint8)EKzUIInputType::Right) |
		(1 << (uint8)EKzUIInputType::Previous) |
		(1 << (uint8)EKzUIInputType::Next);

	ADD_INPUT_CALLBACKS(Back);
	ADD_INPUT_CALLBACKS(Accept);
	ADD_INPUT_CALLBACKS(Down);
	ADD_INPUT_CALLBACKS(Up);
	ADD_INPUT_CALLBACKS(Left);
	ADD_INPUT_CALLBACKS(Right);
	ADD_INPUT_CALLBACKS(Previous);
	ADD_INPUT_CALLBACKS(Next);
	ADD_INPUT_CALLBACKS(Start);
	ADD_INPUT_CALLBACKS(Select);
}

#undef ADD_INPUT_CALLBACKS

void UKzUserWidget::NativeConstruct()
{
	for (const auto& Pair : UKzUIInputSettings::Get()->InputMap)
	{
		InputMap.Add(Pair.Key, Pair.Value.GetInputDefinition());
		TriggerMap.Add(Pair.Key, Pair.Value.TriggerConfig);
	}

	SetInputEnabled(bStartWithInputEnabled);

	// Named slots with default content are not registered as slot content, so WidgetTree->ForEachWidget
	// misses them (see UUserWidget::GetContentForSlot). Register their first child explicitly.
	{
		TArray<FName> SlotNames;
		GetSlotNames(SlotNames);

		for (FName SlotName : SlotNames)
		{
			if (UPanelWidget* AsPanel = Cast<UPanelWidget>(GetWidgetFromName(SlotName)))
			{
				if (UWidget* Content = AsPanel->GetChildAt(0))
				{
					SetContentForSlot(SlotName, Content);
				}
			}
		}
	}

	// Pages of a KzWidgetSwitcher register when the switcher activates them, not on construct
	bool bOnActivePage = true;
	FindOwningSwitcher(this, bOnActivePage);
	if (bOnActivePage)
	{
		if (UKzUIInputSubsystem* Subsystem = UKzUIInputSubsystem::Get(GetOwningLocalPlayer()))
		{
			Subsystem->RegisterWidget(this);
		}
		OnActivated();
	}

	Super::NativeConstruct();
}

void UKzUserWidget::OnActivated()
{
	ReceiveOnActivated();
}

void UKzUserWidget::NativeDestruct()
{
	if (UKzUIInputSubsystem* Subsystem = UKzUIInputSubsystem::Get(GetOwningLocalPlayer()))
	{
		Subsystem->UnregisterWidget(this);
	}
	Super::NativeDestruct();
}

FReply UKzUserWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	FReply Reply = FReply::Unhandled();
	if (IsInputEnabled())
	{
		const FKey Key = InKeyEvent.GetKey();
		const EKzUIInputType Input = GetInputFromKey(Key);
		if (Input != EKzUIInputType::None)
		{
			// Only inputs the widget actually handled are consumed; rejected ones bubble to the game
			if (ProcessInputPressed(Input, Key, InKeyEvent.IsRepeat()) == EKzUIInputResult::Handled)
			{
				Reply = FReply::Handled();
			}
		}
	}
	return bAllowNativeOnKeyDownCall ? Super::NativeOnKeyDown(InGeometry, InKeyEvent) : Reply;
}

FReply UKzUserWidget::NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	FReply Reply = FReply::Unhandled();
	if (IsInputEnabled())
	{
		const FKey Key = InKeyEvent.GetKey();
		const EKzUIInputType Input = GetInputFromKey(Key);
		if (Input != EKzUIInputType::None)
		{
			if (ProcessInputReleased(Input, Key) == EKzUIInputResult::Handled)
			{
				Reply = FReply::Handled();
			}
		}
	}
	return bAllowNativeOnKeyUpCall ? Super::NativeOnKeyUp(InGeometry, InKeyEvent) : Reply;
}

void UKzUserWidget::NativeOnFocusLost(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnFocusLost(InFocusEvent);

	// The release of any in-flight press will go to the new focus owner: reset silently,
	// otherwise the next press of the same input is swallowed as a repeat
	ClearInput();
}

void UKzUserWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	LifeTime += InDeltaTime;

	for (EKzUIInputType Input : TEnumRange<EKzUIInputType>())
	{
		if (IsInputPressed(Input))
		{
			FKzUIInputPressedState& State = InputPressedState[Input];
			State.Time += InDeltaTime;

			// Unsupported inputs are tracked for chords but fire nothing
			if (!SupportsInput(Input))
			{
				continue;
			}

			const bool bHandled = CanHandleAnyInput() && CanHandleInput(Input);

			AnyInputHeld(Input, bHandled, State.Time);
			InputHeld[Input](bHandled, State.Time);

			const FKzUIInputTriggerConfig Trigger = TriggerMap.FindRef(Input);
			if (Trigger.Trigger == EKzUIInputTrigger::Hold && !State.bTriggerFired && State.Time >= Trigger.HoldTime)
			{
				State.bTriggerFired = true;
				FireInputTriggered(Input);
			}
		}
	}

	Super::NativeTick(MyGeometry, InDeltaTime);
}

void UKzUserWidget::RemoveFromParent()
{
	RemoveFromStack(false);
}

void UKzUserWidget::RemoveFromStack(bool bPropagate)
{
	if (!IsDesignTime() && !HasAnyFlags(RF_ClassDefaultObject | RF_BeginDestroyed))
	{
		// A switcher page closes by handing control back to the switcher, not by removing itself
		bool bOnActivePage = true;
		if (UKzWidgetSwitcher* Switcher = FindOwningSwitcher(this, bOnActivePage))
		{
			if (bOnActivePage)
			{
				Switcher->GoBack();
			}
			return;
		}

		ReceiveOnRemoveFromParent();
		OnRemovedFromParent.Broadcast(this, bPropagate);
		SetInputEnabled(false);
	}
	Super::RemoveFromParent();
}

UKzUserWidget* UKzUserWidget::CreateDynamicChild(TSubclassOf<UKzUserWidget> ContentClass, int32 ZOrder, bool bHideWhileOpen)
{
	if (!ContentClass)
	{
		return nullptr;
	}

	UKzUserWidget* Content = CreateWidget<UKzUserWidget>(this, ContentClass.Get());
	Content->SetOwningLocalPlayer(GetOwningLocalPlayer());
	AddDynamicChild(Content, ZOrder, bHideWhileOpen);
	return Content;
}

void UKzUserWidget::AddDynamicChild(UKzUserWidget* Content, int32 ZOrder, bool bHideWhileOpen)
{
	if (Content != nullptr)
	{
		ClearChild();
		ChildWidget = Content;
		ChildWidget->OnRemovedFromParent.AddDynamic(this, &UKzUserWidget::OnChildRemoved);
		ChildWidget->AddToViewport(ZOrder);
		ChildWidget->SetFocus();

		// IsSet guards replacing one hiding child with another: the visibility to restore is
		// the one from before the first of them
		if (bHideWhileOpen && !VisibilityBeforeChild.IsSet())
		{
			VisibilityBeforeChild = GetVisibility();
			SetVisibility(ESlateVisibility::Collapsed);
		}

		OnAddChild(ChildWidget);
	}
}

UKzPromptWidget* UKzUserWidget::ShowPrompt(FText Message, FText ConfirmMessage, FText CancelMessage, FKzUISimpleDelegate OnConfirm, FKzUISimpleDelegate OnCancel, TSubclassOf<UKzPromptWidget> ClassOverride)
{
	TSubclassOf<UKzPromptWidget> PromptClass = ClassOverride ? ClassOverride : TSubclassOf<UKzPromptWidget>(UKzUIInputSettings::Get()->PromptClass.LoadSynchronous());
	if (!PromptClass)
	{
		return nullptr;
	}

	UKzPromptWidget* Prompt = CreateWidget<UKzPromptWidget>(this, PromptClass);
	Prompt->SetOwningLocalPlayer(GetOwningLocalPlayer());
	Prompt->Message = Message;
	Prompt->ConfirmMessage = ConfirmMessage;
	Prompt->CancelMessage = CancelMessage;
	Prompt->OnConfirm.Delegate = OnConfirm;
	Prompt->OnCancel.Delegate = OnCancel;
	AddDynamicChild(Prompt);
	return Prompt;
}

UKzUserWidget* UKzUserWidget::CreateSlottedDynamicChild(UPanelWidget* Panel, TSubclassOf<UKzUserWidget> ContentClass)
{
	if (!ContentClass || !Panel)
	{
		return nullptr;
	}

	UKzUserWidget* Content = CreateWidget<UKzUserWidget>(this, ContentClass.Get());
	Content->SetOwningLocalPlayer(GetOwningLocalPlayer());
	AddSlottedDynamicChild(Panel, Content);
	return Content;
}

void UKzUserWidget::AddSlottedDynamicChild(UPanelWidget* Panel, UKzUserWidget* Content)
{
	if (Content != nullptr && Panel != nullptr)
	{
		ClearChild();
		ChildWidget = Content;
		ChildWidget->OnRemovedFromParent.AddDynamic(this, &UKzUserWidget::OnChildRemoved);
		UPanelSlot* ReturnSlot = Panel->AddChild(ChildWidget);
		ChildWidget->SetFocus();
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(ReturnSlot))
		{
			CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
			CanvasSlot->SetOffsets(FMargin());
		}
		OnAddChild(ChildWidget);
	}
}

void UKzUserWidget::OnAddChild(UKzUserWidget* Child)
{
	ReceiveOnAddChild(Child);
}

void UKzUserWidget::ClearChild()
{
	if (ChildWidget != nullptr)
	{
		ChildWidget->OnRemovedFromParent.RemoveDynamic(this, &UKzUserWidget::OnChildRemoved);
		ChildWidget = nullptr;
		ClearInput();
	}
}

void UKzUserWidget::OnChildRemoved(UKzUserWidget* Child, bool bPropagate)
{
	ClearChild();

	if (VisibilityBeforeChild.IsSet())
	{
		SetVisibility(VisibilityBeforeChild.GetValue());
		VisibilityBeforeChild.Reset();
	}

	if (bPropagate)
	{
		RemoveFromStack(true);
	}
	else
	{
		SetFocus();
		ReceiveOnChildRemoved(Child);
	}
}

void UKzUserWidget::SetInputEnabled(const bool bNewEnabled)
{
	if (bInputEnabled != bNewEnabled)
	{
		bInputEnabled = bNewEnabled;

		if (bInputEnabled)
		{
			LifeTime = 0.0f;
		}
		else
		{
			ClearInput();
		}
	}
}

bool UKzUserWidget::IsInputEnabled() const
{
	if (!bInputEnabled || ChildWidget != nullptr)
	{
		return false;
	}

	const UKzUIInputSubsystem* Subsystem = UKzUIInputSubsystem::Get(GetOwningLocalPlayer());
	return !Subsystem || !Subsystem->IsInputSuspended();
}

void UKzUserWidget::ClearInput()
{
	InputPressedState.Empty();
	LastTapTime.Empty();
}

bool UKzUserWidget::SupportsInput(const EKzUIInputType Input) const
{
	return InputMask & (1 << (uint8)Input);
}

void UKzUserWidget::SetInputSupport(const EKzUIInputType Input, const bool bIsSupported)
{
	if (bIsSupported)
	{
		InputMask |= (1 << (uint8)Input);
	}
	else
	{
		InputMask &= ~(1 << (uint8)Input);
	}
}

bool UKzUserWidget::IsInputPressed(const EKzUIInputType Input) const
{
	return InputPressedState.Contains(Input);
}

float UKzUserWidget::GetInputHeldTime(const EKzUIInputType Input) const
{
	return IsInputPressed(Input) ? InputPressedState[Input].Time : 0.0f;
}

float UKzUserWidget::GetInputHoldProgress(const EKzUIInputType Input) const
{
	const FKzUIInputPressedState* State = InputPressedState.Find(Input);
	if (!State)
	{
		return 0.0f;
	}

	const FKzUIInputTriggerConfig Trigger = TriggerMap.FindRef(Input);
	if (Trigger.Trigger == EKzUIInputTrigger::Hold && Trigger.HoldTime > 0.0f)
	{
		return FMath::Clamp(State->Time / Trigger.HoldTime, 0.0f, 1.0f);
	}
	return 1.0f;
}

EKzUIInputType UKzUserWidget::GetInputFromKey(const FKey& Key) const
{
	for (const auto& Pair : InputMap)
	{
		if (Pair.Value.Contains(Key))
		{
			return Pair.Key;
		}
	}
	return EKzUIInputType::None;
}

bool UKzUserWidget::IsInputMappedToKey(EKzUIInputType Input, const FKey& Key) const
{
	const FKzUIInputDefinition* InputDefinition = InputMap.Find(Input);
	return InputDefinition && InputDefinition->Contains(Key);
}

void UKzUserWidget::ResetInputMap(EKzUIInputType Input)
{
	const UKzUIInputSettings* Settings = UKzUIInputSettings::Get();
	if (Settings->InputMap.Contains(Input))
	{
		InputMap.Add(Input, Settings->InputMap[Input].GetInputDefinition());
		TriggerMap.Add(Input, Settings->InputMap[Input].TriggerConfig);
	}
}

void UKzUserWidget::OverrideInputMap(const EKzUIInputType Input, TArray<FKey> Keys)
{
	InputMap.Add(Input, FKzUIInputDefinition(Keys));
	InputPressedState.Remove(Input);
}

void UKzUserWidget::OverrideInputTrigger(const EKzUIInputType Input, const FKzUIInputTriggerConfig& Config)
{
	TriggerMap.Add(Input, Config);
	InputPressedState.Remove(Input);
}

EKzUIInputResult UKzUserWidget::ProcessInputPressed(const EKzUIInputType Input, const FKey& Key, bool bRepeat)
{
	// Continuous inputs repeat on every key event and keep plain Press semantics
	if (ContinuousInputMask & (1 << (uint8)Input))
	{
		return SupportsInput(Input) ? FireInputTriggered(Input) : EKzUIInputResult::Ignored;
	}

	// A repeat can never start a press: its original key down went to a previous focus owner
	if (bRepeat && !IsInputPressed(Input))
	{
		return EKzUIInputResult::Ignored;
	}

	// Track the pressed state of every discrete input, supported or not: chords query other inputs
	const bool bWasPressed = IsInputPressed(Input);
	FKzUIInputPressedState& State = InputPressedState.FindOrAdd(Input);
	State.Keys.AddUnique(Key);

	if (!SupportsInput(Input) || bWasPressed)
	{
		return EKzUIInputResult::Ignored;
	}

	const FKzUIInputTriggerConfig Trigger = TriggerMap.FindRef(Input);
	switch (Trigger.Trigger)
	{
		case EKzUIInputTrigger::DoubleTap:
		{
			const float* LastTap = LastTapTime.Find(Input);
			if (LastTap && LifeTime - *LastTap <= Trigger.TapInterval)
			{
				LastTapTime.Remove(Input);
				State.bTriggerFired = true;
				return FireInputTriggered(Input);
			}
			// The first tap is not consumed: the key keeps working for the game
			LastTapTime.Add(Input, LifeTime);
			return EKzUIInputResult::Unhandled;
		}
		case EKzUIInputTrigger::Chord:
		{
			for (EKzUIInputType Required : Trigger.RequiredInputs)
			{
				// Modifiers must already be held; an incomplete chord bubbles to the game
				if (!IsInputPressed(Required))
				{
					return EKzUIInputResult::Unhandled;
				}
			}
			State.bTriggerFired = true;
			return FireInputTriggered(Input);
		}
		case EKzUIInputTrigger::Hold:
		{
			// Not consumed: NativeTick fires once the input has been held HoldTime seconds
			return EKzUIInputResult::Unhandled;
		}
		default:
		{
			State.bTriggerFired = true;
			return FireInputTriggered(Input);
		}
	}
}

EKzUIInputResult UKzUserWidget::ProcessInputReleased(const EKzUIInputType Input, const FKey& Key)
{
	if (ContinuousInputMask & (1 << (uint8)Input))
	{
		return SupportsInput(Input) ? FireInputReleased(Input) : EKzUIInputResult::Ignored;
	}

	if (!IsInputPressed(Input))
	{
		return EKzUIInputResult::Ignored;
	}

	FKzUIInputPressedState& State = InputPressedState[Input];
	State.Keys.Remove(Key);
	if (State.Keys.Num() > 0)
	{
		return EKzUIInputResult::Ignored;
	}

	const bool bFired = State.bTriggerFired || TriggerMap.FindRef(Input).Trigger == EKzUIInputTrigger::Press;
	InputPressedState.Remove(Input);

	if (!SupportsInput(Input))
	{
		return EKzUIInputResult::Ignored;
	}

	// Released always fires so a cancelled pending trigger can undo its feedback, but only a
	// release whose press actually triggered may consume the key: the game saw the pending
	// press go down, so it must see it go up too
	const EKzUIInputResult Result = FireInputReleased(Input);
	return bFired ? Result : EKzUIInputResult::Unhandled;
}

EKzUIInputResult UKzUserWidget::FireInputTriggered(const EKzUIInputType Input)
{
	const bool bHandled = CanHandleAnyInput() && CanHandleInput(Input);

	if (!bPlaySoundOnlyHandled || bHandled)
	{
		PlayInputSound(Input);
	}

	AnyInputTriggered(Input, bHandled);
	InputTriggered[Input](bHandled);
	return bHandled ? EKzUIInputResult::Handled : EKzUIInputResult::Unhandled;
}

EKzUIInputResult UKzUserWidget::FireInputReleased(const EKzUIInputType Input)
{
	const bool bHandled = CanHandleAnyInput() && CanHandleInput(Input);

	AnyInputReleased(Input, bHandled);
	InputReleased[Input](bHandled);
	return bHandled ? EKzUIInputResult::Handled : EKzUIInputResult::Unhandled;
}

UKzUISoundTheme* UKzUserWidget::GetSoundTheme() const
{
	const UKzUIInputSubsystem* Subsystem = UKzUIInputSubsystem::Get(GetOwningLocalPlayer());
	return Subsystem ? Subsystem->GetSoundTheme() : nullptr;
}

void UKzUserWidget::PlayInputSound(EKzUIInputType Input)
{
	USoundBase* Sound = InputSoundOverrides.FindRef(Input);
	if (!Sound)
	{
		if (const UKzUISoundTheme* Theme = GetSoundTheme())
		{
			Sound = Theme->InputSounds.FindRef(Input);
		}
	}
	if (Sound)
	{
		UGameplayStatics::PlaySound2D(this, Sound);
	}
}

bool UKzUserWidget::CanHandleInput(EKzUIInputType Input)
{
	return CanHandleInputFunc[Input]();
}
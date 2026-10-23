// Copyright 2026 kirzo

#pragma once

#include "Blueprint/UserWidget.h"
#include "KzUITypes.h"
#include "KzUserWidget.generated.h"

class UKzPromptWidget;
class UPanelWidget;
class USoundBase;
class UKzUserWidget;

struct FKzUIInputPressedState
{
	/** Slate application time of the first key down, so holds keep counting while the widget is hidden or does not tick. */
	double StartTime = 0.0;
	bool bTriggerFired = false;
	/** Whether the first key down was consumed: its repeats and its release go the same way, so the game never sees half a press. */
	bool bConsumed = false;
	TArray<FKey> Keys;

	float GetHeldTime() const;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FKzUserWidgetRemovedSignature, UKzUserWidget*, Child, bool, bPropagate);

/**
 * User widget with semantic UI input (Back/Accept/navigation/etc.) mapped from UKzUIInputSettings,
 * plus a single-child modal stack (CreateDynamicChild and friends). While a child is up, this
 * widget's input is suspended.
 */
UCLASS(Abstract, meta = (PrioritizeCategories = "KzUI"))
class KZUI_API UKzUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = "KzUI|Input", EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = true))
	bool bStartWithInputEnabled = false;

	/** While this widget is on screen, its player keeps the only live UI input: every other player's widget input is suspended. */
	UPROPERTY(Category = "KzUI|Input", EditAnywhere)
	bool bSoloInput = false;

	/** Inputs this widget reacts to. */
	UPROPERTY(Category = "KzUI|Input", EditAnywhere, meta = (Bitmask, BitmaskEnum = "/Script/KzUI.EKzUIInputType"))
	uint32 InputMask = 0;

	/** Inputs whose Triggered event fires on every key event instead of once per press. */
	UPROPERTY(Category = "KzUI|Input", EditAnywhere, meta = (Bitmask, BitmaskEnum = "/Script/KzUI.EKzUIInputType"))
	uint32 ContinuousInputMask = 0;

	UPROPERTY(Category = "KzUI|Events", BlueprintAssignable)
	FKzUserWidgetRemovedSignature OnRemovedFromParent;

	/** Forward key events to the default UUserWidget handlers after processing UI input. */
	UPROPERTY(Category = "KzUI|Input", EditAnywhere)
	bool bAllowNativeOnKeyDownCall = false;

	UPROPERTY(Category = "KzUI|Input", EditAnywhere)
	bool bAllowNativeOnKeyUpCall = false;

	/** Per-widget sounds taking precedence over the project-settings InputSounds map. */
	UPROPERTY(Category = "KzUI|Sound", EditAnywhere)
	TMap<EKzUIInputType, TObjectPtr<USoundBase>> InputSoundOverrides;

	/** Play input sounds only when the input is actually handled. */
	UPROPERTY(Category = "KzUI|Sound", EditAnywhere)
	bool bPlaySoundOnlyHandled = true;

private:
	bool bInputEnabled = false;

	TMap<EKzUIInputType, FKzUIInputPressedState> InputPressedState;

	UPROPERTY(Transient)
	TObjectPtr<UKzUserWidget> ChildWidget;

	/** Widget this one is the modal child of. */
	TWeakObjectPtr<UKzUserWidget> ModalOwner;

	/** Visibility to restore when the child opened with bHideWhileOpen is removed. */
	TOptional<ESlateVisibility> VisibilityBeforeChild;

	/** Keys replaced on this instance through OverrideInputMap. Every other input resolves from the project settings on each lookup, so it follows the player's pad. */
	TMap<EKzUIInputType, FKzUIInputDefinition> InputMapOverrides;

	/** Triggers replaced on this instance through OverrideInputTrigger; every other input uses the project settings. */
	TMap<EKzUIInputType, FKzUIInputTriggerConfig> TriggerOverrides;

	/** Slate application time of the first tap of each DoubleTap input. */
	TMap<EKzUIInputType, double> LastTapTime;

	TMap<EKzUIInputType, TFunction<bool()>> CanHandleInputFunc;
	TMap<EKzUIInputType, TFunction<void(bool)>> InputTriggered;
	TMap<EKzUIInputType, TFunction<void(bool, float)>> InputHeld;
	TMap<EKzUIInputType, TFunction<void(bool, bool)>> InputReleased;

public:
	UKzUserWidget(const FObjectInitializer& ObjectInitializer);

	//~ Begin UUserWidget Interface
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual void NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent) override;
	virtual FNavigationReply NativeOnNavigation(const FGeometry& MyGeometry, const FNavigationEvent& InNavigationEvent, const FNavigationReply& InDefaultReply) override;
	virtual FCursorReply NativeOnCursorQuery(const FGeometry& InGeometry, const FPointerEvent& InCursorEvent) override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:
	virtual bool Initialize() override;
	virtual void RemoveFromParent() override;
	//~ End UUserWidget Interface

	/** The widget became the active screen: registered on the player's stack at construct, or activated as a KzWidgetSwitcher page. */
	virtual void OnActivated();

	UFUNCTION(Category = "KzUI", BlueprintImplementableEvent, meta = (DisplayName = "On Activated"))
	void ReceiveOnActivated();

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Remove From Parent"))
	void ReceiveOnRemoveFromParent();

	/** Removes this widget; with bPropagate, parents in the dynamic-child chain remove themselves too. A KzWidgetSwitcher page goes back instead, and with bPropagate removes the screen hosting the switcher. */
	UFUNCTION(Category = "KzUI", BlueprintCallable)
	void RemoveFromStack(bool bPropagate = false);

	/** With bHideWhileOpen, this widget collapses while the child is up and restores its previous visibility when the child is removed. */
	UFUNCTION(Category = "KzUI|Children", BlueprintCallable)
	UKzUserWidget* CreateDynamicChild(TSubclassOf<UKzUserWidget> ContentClass, int32 ZOrder = 0, bool bHideWhileOpen = false);

	UFUNCTION(Category = "KzUI|Children", BlueprintCallable)
	void AddDynamicChild(UKzUserWidget* Content, int32 ZOrder = 0, bool bHideWhileOpen = false);

	/** Opens the project-settings prompt (or ClassOverride) as this widget's dynamic child. */
	UFUNCTION(Category = "KzUI|Children", BlueprintCallable, meta = (AdvancedDisplay = "ClassOverride", AutoCreateRefTerm = "OnConfirm,OnCancel"))
	UKzPromptWidget* ShowPrompt(FText Message, FText ConfirmMessage, FText CancelMessage, FKzUISimpleDelegate OnConfirm, FKzUISimpleDelegate OnCancel, TSubclassOf<UKzPromptWidget> ClassOverride = nullptr);

	UFUNCTION(Category = "KzUI|Children", BlueprintCallable)
	UKzUserWidget* CreateSlottedDynamicChild(UPanelWidget* Panel, TSubclassOf<UKzUserWidget> ContentClass);

	UFUNCTION(Category = "KzUI|Children", BlueprintCallable)
	void AddSlottedDynamicChild(UPanelWidget* Panel, UKzUserWidget* Content);

	virtual void OnAddChild(UKzUserWidget* Child);

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Add Child"))
	void ReceiveOnAddChild(UKzUserWidget* Child);

	UFUNCTION(Category = "KzUI|Children", BlueprintCallable)
	void ClearChild();

	UFUNCTION()
	void OnChildRemoved(UKzUserWidget* Child, bool bPropagate = false);

	UFUNCTION(Category = "KzUI|Children", BlueprintImplementableEvent, meta = (DisplayName = "On Child Removed"))
	void ReceiveOnChildRemoved(UKzUserWidget* Child);

	UFUNCTION(Category = "KzUI|Input", BlueprintCallable)
	void SetInputEnabled(const bool bNewEnabled);

	UFUNCTION(Category = "KzUI|Input", BlueprintCallable)
	bool IsInputEnabled() const;

	/** Whether SetInputEnabled switched the input on and no modal child covers the widget. Widgets that do not only get the focus when no other widget wants it. */
	bool WantsFocus() const { return bInputEnabled && !ChildWidget; }

	/** Whether SetInputEnabled switched the input on, whatever else blocks it right now. */
	bool IsInputSwitchedOn() const { return bInputEnabled; }

	/** Resets runtime input state, firing Released with bCancelled for the in-flight presses so their pending feedback can be undone. */
	void ClearInput();

	UFUNCTION(Category = "KzUI|Input", BlueprintCallable)
	bool SupportsInput(const EKzUIInputType Input) const;

	UFUNCTION(Category = "KzUI|Input", BlueprintCallable)
	void SetInputSupport(const EKzUIInputType Input, const bool bIsSupported);

	UFUNCTION(Category = "KzUI|Input", BlueprintPure)
	bool IsInputPressed(const EKzUIInputType Input) const;

	UFUNCTION(Category = "KzUI|Input", BlueprintPure)
	float GetInputHeldTime(const EKzUIInputType Input) const;

	/** Progress of a Hold trigger from 0 to 1. Inputs without a Hold trigger report 1 while pressed. */
	UFUNCTION(Category = "KzUI|Input", BlueprintPure)
	float GetInputHoldProgress(const EKzUIInputType Input) const;

	UFUNCTION(Category = "KzUI|Input", BlueprintCallable)
	EKzUIInputType GetInputFromKey(const FKey& Key) const;

	UFUNCTION(Category = "KzUI|Input", BlueprintPure)
	bool IsInputMappedToKey(EKzUIInputType Input, const FKey& Key) const;

	/** Restores the project-settings keys and trigger for the given input. */
	UFUNCTION(Category = "KzUI|Input", BlueprintCallable)
	void ResetInputMap(EKzUIInputType Input);

	/** Replaces the keys mapped to the given input on this widget instance. */
	UFUNCTION(Category = "KzUI|Input", BlueprintCallable)
	void OverrideInputMap(const EKzUIInputType Input, TArray<FKey> Keys);

	/** Replaces the trigger of the given input on this widget instance. */
	UFUNCTION(Category = "KzUI|Input", BlueprintCallable)
	void OverrideInputTrigger(const EKzUIInputType Input, const FKzUIInputTriggerConfig& Config);

	EKzUIInputResult ProcessInputPressed(const EKzUIInputType Input, const FKey& Key, bool bRepeat = false);
	EKzUIInputResult ProcessInputReleased(const EKzUIInputType Input, const FKey& Key);

	/** Plays the sound mapped to the given input (widget override first, then the active sound theme). */
	UFUNCTION(Category = "KzUI|Sound", BlueprintCallable)
	virtual void PlayInputSound(EKzUIInputType Input);

protected:
	/** Active sound theme for the owning player. */
	class UKzUISoundTheme* GetSoundTheme() const;

	/** Fires an input's Triggered events with its sound, Handled meaning the widget could act on it. Also the way other sources (the mouse) trigger an input. */
	EKzUIInputResult FireInputTriggered(const EKzUIInputType Input);

private:
	/** Makes Content the modal child, removing the one it replaces. */
	void AttachChild(UKzUserWidget* Content);

	/** Removes the modal child and hands control back to the owner, if this is a modal child. */
	void ReleaseModalLinks(bool bPropagate);

	EKzUIInputResult FireInputReleased(const EKzUIInputType Input, bool bCancelled);

	/** FireInputTriggered for a key press, recording whether the press was consumed. */
	EKzUIInputResult FirePressTriggered(const EKzUIInputType Input);

	/** Fires a Hold trigger whose time is up. Checked on tick and on key repeats. */
	void FireHoldIfDue(const EKzUIInputType Input);

	/** Family of the owning player's last pad, which picks the gamepad key overrides. */
	EKzUIInputDevice GetGamepadDevice() const;

	/** This instance's override, or the project settings resolved for the owning player's pad. */
	const FKzUIInputDefinition* FindInputDefinition(EKzUIInputType Input) const;

	/** This instance's override, or the project settings. */
	const FKzUIInputTriggerConfig* FindTriggerConfig(EKzUIInputType Input) const;

public:

	bool CanHandleInput(EKzUIInputType Input);

	/**
	 * CanHandleXInput reports whether X input is currently actionable. It defaults to true and
	 * gates the Handled flag only: the matching Triggered/Held/Released events fire regardless.
	 */
	UFUNCTION(Category = "KzUI|Input", BlueprintNativeEvent, BlueprintCosmetic)
	bool CanHandleAnyInput();
	virtual bool CanHandleAnyInput_Implementation() { return true; }

	UFUNCTION(Category = "KzUI|Input", BlueprintNativeEvent, BlueprintCosmetic)
	bool CanHandleBackInput();
	virtual bool CanHandleBackInput_Implementation() { return true; }

	UFUNCTION(Category = "KzUI|Input", BlueprintNativeEvent, BlueprintCosmetic)
	bool CanHandleAcceptInput();
	virtual bool CanHandleAcceptInput_Implementation() { return true; }

	UFUNCTION(Category = "KzUI|Input", BlueprintNativeEvent, BlueprintCosmetic)
	bool CanHandleDownInput();
	virtual bool CanHandleDownInput_Implementation() { return true; }

	UFUNCTION(Category = "KzUI|Input", BlueprintNativeEvent, BlueprintCosmetic)
	bool CanHandleUpInput();
	virtual bool CanHandleUpInput_Implementation() { return true; }

	UFUNCTION(Category = "KzUI|Input", BlueprintNativeEvent, BlueprintCosmetic)
	bool CanHandleLeftInput();
	virtual bool CanHandleLeftInput_Implementation() { return true; }

	UFUNCTION(Category = "KzUI|Input", BlueprintNativeEvent, BlueprintCosmetic)
	bool CanHandleRightInput();
	virtual bool CanHandleRightInput_Implementation() { return true; }

	UFUNCTION(Category = "KzUI|Input", BlueprintNativeEvent, BlueprintCosmetic)
	bool CanHandlePreviousInput();
	virtual bool CanHandlePreviousInput_Implementation() { return true; }

	UFUNCTION(Category = "KzUI|Input", BlueprintNativeEvent, BlueprintCosmetic)
	bool CanHandleNextInput();
	virtual bool CanHandleNextInput_Implementation() { return true; }

	UFUNCTION(Category = "KzUI|Input", BlueprintNativeEvent, BlueprintCosmetic)
	bool CanHandleStartInput();
	virtual bool CanHandleStartInput_Implementation() { return true; }

	UFUNCTION(Category = "KzUI|Input", BlueprintNativeEvent, BlueprintCosmetic)
	bool CanHandleSelectInput();
	virtual bool CanHandleSelectInput_Implementation() { return true; }

	/** AnyInputX fires for every input, before the input-specific event. */
	UFUNCTION(Category = "KzUI|Input", BlueprintNativeEvent, BlueprintCosmetic)
	void AnyInputTriggered(EKzUIInputType Input, bool Handled);
	virtual void AnyInputTriggered_Implementation(EKzUIInputType Input, bool Handled) {}

	UFUNCTION(Category = "KzUI|Input", BlueprintNativeEvent, BlueprintCosmetic)
	void AnyInputHeld(EKzUIInputType Input, bool Handled, float Time);
	virtual void AnyInputHeld_Implementation(EKzUIInputType Input, bool Handled, float Time) {}

	/** bCancelled: the press ended without a release (focus lost, input switched off, widget removed), so act-on-release logic should skip it while pending feedback is undone. Same flag on every Released event. */
	UFUNCTION(Category = "KzUI|Input", BlueprintNativeEvent, BlueprintCosmetic)
	void AnyInputReleased(EKzUIInputType Input, bool Handled, bool bCancelled);
	virtual void AnyInputReleased_Implementation(EKzUIInputType Input, bool Handled, bool bCancelled) {}

	UFUNCTION(Category = "KzUI|Input", BlueprintNativeEvent, BlueprintCosmetic)
	void BackInputTriggered(bool Handled);
	virtual void BackInputTriggered_Implementation(bool Handled) {}

	UFUNCTION(Category = "KzUI|Input", BlueprintNativeEvent, BlueprintCosmetic)
	void BackInputHeld(bool Handled, float Time);
	virtual void BackInputHeld_Implementation(bool Handled, float Time) {}

	UFUNCTION(Category = "KzUI|Input", BlueprintNativeEvent, BlueprintCosmetic)
	void BackInputReleased(bool Handled, bool bCancelled);
	virtual void BackInputReleased_Implementation(bool Handled, bool bCancelled) {}

	UFUNCTION(Category = "KzUI|Input", BlueprintNativeEvent, BlueprintCosmetic)
	void AcceptInputTriggered(bool Handled);
	virtual void AcceptInputTriggered_Implementation(bool Handled) {}

	UFUNCTION(Category = "KzUI|Input", BlueprintNativeEvent, BlueprintCosmetic)
	void AcceptInputHeld(bool Handled, float Time);
	virtual void AcceptInputHeld_Implementation(bool Handled, float Time) {}

	UFUNCTION(Category = "KzUI|Input", BlueprintNativeEvent, BlueprintCosmetic)
	void AcceptInputReleased(bool Handled, bool bCancelled);
	virtual void AcceptInputReleased_Implementation(bool Handled, bool bCancelled) {}

	UFUNCTION(Category = "KzUI|Input", BlueprintNativeEvent, BlueprintCosmetic)
	void DownInputTriggered(bool Handled);
	virtual void DownInputTriggered_Implementation(bool Handled) {}

	UFUNCTION(Category = "KzUI|Input", BlueprintNativeEvent, BlueprintCosmetic)
	void DownInputHeld(bool Handled, float Time);
	virtual void DownInputHeld_Implementation(bool Handled, float Time) {}

	UFUNCTION(Category = "KzUI|Input", BlueprintNativeEvent, BlueprintCosmetic)
	void DownInputReleased(bool Handled, bool bCancelled);
	virtual void DownInputReleased_Implementation(bool Handled, bool bCancelled) {}

	UFUNCTION(Category = "KzUI|Input", BlueprintNativeEvent, BlueprintCosmetic)
	void UpInputTriggered(bool Handled);
	virtual void UpInputTriggered_Implementation(bool Handled) {}

	UFUNCTION(Category = "KzUI|Input", BlueprintNativeEvent, BlueprintCosmetic)
	void UpInputHeld(bool Handled, float Time);
	virtual void UpInputHeld_Implementation(bool Handled, float Time) {}

	UFUNCTION(Category = "KzUI|Input", BlueprintNativeEvent, BlueprintCosmetic)
	void UpInputReleased(bool Handled, bool bCancelled);
	virtual void UpInputReleased_Implementation(bool Handled, bool bCancelled) {}

	UFUNCTION(Category = "KzUI|Input", BlueprintNativeEvent, BlueprintCosmetic)
	void LeftInputTriggered(bool Handled);
	virtual void LeftInputTriggered_Implementation(bool Handled) {}

	UFUNCTION(Category = "KzUI|Input", BlueprintNativeEvent, BlueprintCosmetic)
	void LeftInputHeld(bool Handled, float Time);
	virtual void LeftInputHeld_Implementation(bool Handled, float Time) {}

	UFUNCTION(Category = "KzUI|Input", BlueprintNativeEvent, BlueprintCosmetic)
	void LeftInputReleased(bool Handled, bool bCancelled);
	virtual void LeftInputReleased_Implementation(bool Handled, bool bCancelled) {}

	UFUNCTION(Category = "KzUI|Input", BlueprintNativeEvent, BlueprintCosmetic)
	void RightInputTriggered(bool Handled);
	virtual void RightInputTriggered_Implementation(bool Handled) {}

	UFUNCTION(Category = "KzUI|Input", BlueprintNativeEvent, BlueprintCosmetic)
	void RightInputHeld(bool Handled, float Time);
	virtual void RightInputHeld_Implementation(bool Handled, float Time) {}

	UFUNCTION(Category = "KzUI|Input", BlueprintNativeEvent, BlueprintCosmetic)
	void RightInputReleased(bool Handled, bool bCancelled);
	virtual void RightInputReleased_Implementation(bool Handled, bool bCancelled) {}

	UFUNCTION(Category = "KzUI|Input", BlueprintNativeEvent, BlueprintCosmetic)
	void PreviousInputTriggered(bool Handled);
	virtual void PreviousInputTriggered_Implementation(bool Handled) {}

	UFUNCTION(Category = "KzUI|Input", BlueprintNativeEvent, BlueprintCosmetic)
	void PreviousInputHeld(bool Handled, float Time);
	virtual void PreviousInputHeld_Implementation(bool Handled, float Time) {}

	UFUNCTION(Category = "KzUI|Input", BlueprintNativeEvent, BlueprintCosmetic)
	void PreviousInputReleased(bool Handled, bool bCancelled);
	virtual void PreviousInputReleased_Implementation(bool Handled, bool bCancelled) {}

	UFUNCTION(Category = "KzUI|Input", BlueprintNativeEvent, BlueprintCosmetic)
	void NextInputTriggered(bool Handled);
	virtual void NextInputTriggered_Implementation(bool Handled) {}

	UFUNCTION(Category = "KzUI|Input", BlueprintNativeEvent, BlueprintCosmetic)
	void NextInputHeld(bool Handled, float Time);
	virtual void NextInputHeld_Implementation(bool Handled, float Time) {}

	UFUNCTION(Category = "KzUI|Input", BlueprintNativeEvent, BlueprintCosmetic)
	void NextInputReleased(bool Handled, bool bCancelled);
	virtual void NextInputReleased_Implementation(bool Handled, bool bCancelled) {}

	UFUNCTION(Category = "KzUI|Input", BlueprintNativeEvent, BlueprintCosmetic)
	void StartInputTriggered(bool Handled);
	virtual void StartInputTriggered_Implementation(bool Handled) {}

	UFUNCTION(Category = "KzUI|Input", BlueprintNativeEvent, BlueprintCosmetic)
	void StartInputHeld(bool Handled, float Time);
	virtual void StartInputHeld_Implementation(bool Handled, float Time) {}

	UFUNCTION(Category = "KzUI|Input", BlueprintNativeEvent, BlueprintCosmetic)
	void StartInputReleased(bool Handled, bool bCancelled);
	virtual void StartInputReleased_Implementation(bool Handled, bool bCancelled) {}

	UFUNCTION(Category = "KzUI|Input", BlueprintNativeEvent, BlueprintCosmetic)
	void SelectInputTriggered(bool Handled);
	virtual void SelectInputTriggered_Implementation(bool Handled) {}

	UFUNCTION(Category = "KzUI|Input", BlueprintNativeEvent, BlueprintCosmetic)
	void SelectInputHeld(bool Handled, float Time);
	virtual void SelectInputHeld_Implementation(bool Handled, float Time) {}

	UFUNCTION(Category = "KzUI|Input", BlueprintNativeEvent, BlueprintCosmetic)
	void SelectInputReleased(bool Handled, bool bCancelled);
	virtual void SelectInputReleased_Implementation(bool Handled, bool bCancelled) {}
};
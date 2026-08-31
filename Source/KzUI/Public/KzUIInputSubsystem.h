// Copyright 2026 kirzo

#pragma once

#include "Subsystems/LocalPlayerSubsystem.h"
#include "KzUITypes.h"
#include "KzUIInputSubsystem.generated.h"

class FWeakWidgetPath;
class FWidgetPath;
class SWidget;
class UWidget;
struct FFocusEvent;
class UKzUserWidget;
class UKzUISoundTheme;
class UKzUIIconTheme;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKzUIInputDeviceChangedSignature, EKzUIInputDevice, InputDevice);

/**
 * Tracks the widgets a local player has on screen and the input device they are using.
 * Widgets self-register from UKzUserWidget; device changes come from UInputDeviceSubsystem
 * and are broadcast to every registered widget tree through IKzInputDeviceListener.
 */
UCLASS(DisplayName = "Kz UI Input")
class KZUI_API UKzUIInputSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UKzUserWidget>> WidgetStack;

	EKzUIInputDevice CurrentInputDevice = EKzUIInputDevice::Xbox;

	UPROPERTY(Transient)
	TObjectPtr<UKzUISoundTheme> SoundThemeOverride;

	UPROPERTY(Transient)
	TObjectPtr<UKzUIIconTheme> IconThemeOverride;

	TSet<TWeakObjectPtr<UObject>> InputSuspensionSources;

	FDelegateHandle AppActivationHandle;

	FDelegateHandle FocusChangingHandle;

public:
	UPROPERTY(Category = KzUI, BlueprintAssignable)
	FKzUIInputDeviceChangedSignature OnInputDeviceChanged;

	static UKzUIInputSubsystem* Get(const ULocalPlayer* LocalPlayer);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Adds the widget to this player's stack; focusable widgets take the focus on arrival. */
	void RegisterWidget(UKzUserWidget* Widget);

	/** Removes the widget from the stack; if it owned the focus slot, the topmost focusable widget gets it back. */
	void UnregisterWidget(UKzUserWidget* Widget);

	/** Focuses the topmost focusable widget of the stack. */
	UFUNCTION(Category = KzUI, BlueprintCallable)
	void FocusTopWidget();

	UFUNCTION(Category = KzUI, BlueprintPure)
	EKzUIInputDevice GetCurrentInputDevice() const { return CurrentInputDevice; }

	UFUNCTION(Category = KzUI, BlueprintCallable)
	void SetCurrentInputDevice(EKzUIInputDevice NewInputDevice);

	/** Re-reads the most recently used hardware device of this player and applies it. */
	UFUNCTION(Category = KzUI, BlueprintCallable)
	void RefreshCurrentInputDevice();

	/** Suspends every widget input of this player until all sources resume. In-flight presses are cleared. */
	UFUNCTION(Category = KzUI, BlueprintCallable)
	void SuspendInput(UObject* Source);

	UFUNCTION(Category = KzUI, BlueprintCallable)
	void ResumeInput(UObject* Source);

	UFUNCTION(Category = KzUI, BlueprintPure)
	bool IsInputSuspended() const;

	/** Replaces the sound theme for this player. Null reverts to the project-settings default. */
	UFUNCTION(Category = KzUI, BlueprintCallable)
	void SetSoundTheme(UKzUISoundTheme* Theme) { SoundThemeOverride = Theme; }

	/** Active sound theme: runtime override if set, project-settings default otherwise. */
	UFUNCTION(Category = KzUI, BlueprintPure)
	UKzUISoundTheme* GetSoundTheme() const;

	/** Replaces the icon theme for this player. Null reverts to the project-settings default. */
	UFUNCTION(Category = KzUI, BlueprintCallable)
	void SetIconTheme(UKzUIIconTheme* Theme);

	/** Active icon theme: runtime override if set, project-settings default otherwise. */
	UFUNCTION(Category = KzUI, BlueprintPure)
	UKzUIIconTheme* GetIconTheme() const;

private:
	UFUNCTION()
	void OnHardwareDeviceChanged(const FPlatformUserId UserId, const FInputDeviceId DeviceId);

	void OnAppActivationChanged(bool bActivated);

	void OnFocusChanging(const FFocusEvent& FocusEvent, const FWeakWidgetPath& OldPath, const TSharedPtr<SWidget>& OldWidget, const FWidgetPath& NewPath, const TSharedPtr<SWidget>& NewWidget);
	void RestoreStackFocus();
	void ApplyCursorPolicy();
	void SuspendOtherPlayers(UKzUserWidget* Widget, bool bSuspend);
	bool IsEditorSimulating() const;
};
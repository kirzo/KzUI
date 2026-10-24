// Copyright 2026 kirzo

#pragma once

#include "KzNavigableWidget.h"
#include "KzPromptWidget.generated.h"

class UTextBlock;

/** Modal confirm/cancel prompt. Messages and callbacks are passed on spawn. */
UCLASS(Abstract, meta = (PrioritizeCategories = "KzUI"))
class KZUI_API UKzPromptWidget : public UKzNavigableWidget
{
	GENERATED_BODY()

public:
	/** Without it there are no options to move between: Accept confirms and Back cancels. */
	UPROPERTY(Category = "KzUI|Prompt", EditAnywhere, BlueprintReadOnly)
	bool bIsNavigable = true;

	/** Confirm and Cancel remove the prompt before their events fire. Off, the events remove it themselves, e.g. after an outro animation. */
	UPROPERTY(Category = "KzUI|Prompt", EditAnywhere, BlueprintReadOnly)
	bool bCloseOnResult = true;

	UPROPERTY(Category = "KzUI|Prompt", BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> MessageText;

	UPROPERTY(Category = "KzUI|Prompt", BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ConfirmText;

	UPROPERTY(Category = "KzUI|Prompt", BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CancelText;

	UPROPERTY(BlueprintReadOnly, meta = (ExposeOnSpawn = true))
	FText Message;

	UPROPERTY(BlueprintReadOnly, meta = (ExposeOnSpawn = true))
	FText ConfirmMessage;

	UPROPERTY(BlueprintReadOnly, meta = (ExposeOnSpawn = true))
	FText CancelMessage;

	/** Called with true on Confirm and false on Cancel. */
	UPROPERTY(BlueprintReadOnly, meta = (ExposeOnSpawn = true))
	FKzUIPromptResultWrapper OnResult;

	/** The hover starts on the cancel option, for destructive prompts. */
	UPROPERTY(BlueprintReadOnly, meta = (ExposeOnSpawn = true))
	bool bDefaultToCancel = false;

	/** Seconds until the prompt cancels itself, 0 for never. A {Seconds} argument in Message counts down. */
	UPROPERTY(BlueprintReadOnly, meta = (ExposeOnSpawn = true))
	float AutoCancelTime = 0.0f;

public:
	UKzPromptWidget(const FObjectInitializer& ObjectInitializer);

	//~ Begin UUserWidget Interface
protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	//~ End UUserWidget Interface

	//~ Begin UKzUserWidget Interface
public:
	virtual bool CanHandleBackInput_Implementation() override;
	virtual bool CanHandleAcceptInput_Implementation() override;
	virtual void BackInputTriggered_Implementation(bool Handled) override;
	virtual void AcceptInputTriggered_Implementation(bool Handled) override;
	virtual void PlayInputSound(EKzUIInputType Input) override;
	//~ End UKzUserWidget Interface

	//~ Begin UKzNavigableWidget Interface
	virtual TArray<UWidget*> GetOptions_Implementation() const override;
	virtual UWidget* GetDefaultOption_Implementation() const override;
	//~ End UKzNavigableWidget Interface

public:
	UFUNCTION(Category = "KzUI|Prompt", BlueprintCallable)
	void Confirm();

	UFUNCTION(Category = "KzUI|Prompt", BlueprintCallable)
	void Cancel();

	UFUNCTION(Category = "KzUI|Prompt", BlueprintImplementableEvent, meta = (DisplayName = "On Confirm"))
	void ReceiveOnConfirm();

	UFUNCTION(Category = "KzUI|Prompt", BlueprintImplementableEvent, meta = (DisplayName = "On Cancel"))
	void ReceiveOnCancel();

	/** Seconds left before the prompt cancels itself; meaningless without AutoCancelTime. */
	UFUNCTION(Category = "KzUI|Prompt", BlueprintPure)
	float GetRemainingTime() const { return RemainingTime; }

private:
	/** Sets the message, with the countdown filled in. */
	void RefreshMessage();

	bool bAnswered = false;

	float RemainingTime = 0.0f;
};
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

	UPROPERTY(BlueprintReadOnly, meta = (ExposeOnSpawn = true))
	FKzUIDelegateWrapper OnConfirm;

	UPROPERTY(BlueprintReadOnly, meta = (ExposeOnSpawn = true))
	FKzUIDelegateWrapper OnCancel;

public:
	UKzPromptWidget(const FObjectInitializer& ObjectInitializer);

	//~ Begin UUserWidget Interface
protected:
	virtual void NativeConstruct() override;
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
	void Confirm();
	void Cancel();

	UFUNCTION(Category = "KzUI|Prompt", BlueprintImplementableEvent, meta = (DisplayName = "On Confirm"))
	void ReceiveOnConfirm();

	UFUNCTION(Category = "KzUI|Prompt", BlueprintImplementableEvent, meta = (DisplayName = "On Cancel"))
	void ReceiveOnCancel();
};
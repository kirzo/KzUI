// Copyright 2026 kirzo

#pragma once

#include "KzImage.h"
#include "KzUITypes.h"
#include "KzInputIcon.generated.h"

/** Displays the button prompt icon of a semantic input, resolved from the active icon theme and refreshed automatically when the input device changes. */
UCLASS(meta = (PrioritizeCategories = "Icon Image"))
class KZUI_API UKzInputIcon : public UKzImage
{
	GENERATED_BODY()

public:
	/** Input whose icon is displayed. */
	UPROPERTY(Category = Icon, EditAnywhere, BlueprintReadOnly)
	EKzUIInputType Input = EKzUIInputType::Accept;

	/** When set, displays this custom icon of the theme instead of the Input one. */
	UPROPERTY(Category = Icon, EditAnywhere, BlueprintReadOnly)
	FName CustomToken;

	/** Icon variation of the theme wrapping the resolved icon texture in a material. */
	UPROPERTY(Category = Icon, EditAnywhere, BlueprintReadOnly)
	FName Variation;

#if WITH_EDITORONLY_DATA
	/** Device used to preview the icon in the designer. */
	UPROPERTY(Category = Icon, EditAnywhere)
	EKzUIInputDevice PreviewDevice = EKzUIInputDevice::Keyboard;
#endif

	UFUNCTION(Category = "KzUI|Icon", BlueprintCallable)
	void SetInput(EKzUIInputType NewInput);

	UFUNCTION(Category = "KzUI|Icon", BlueprintCallable)
	void SetCustomToken(FName NewToken);

	UFUNCTION(Category = "KzUI|Icon", BlueprintCallable)
	void SetVariation(FName NewVariation);

	//~ Begin UWidget Interface
	virtual void SynchronizeProperties() override;

protected:
	virtual void OnWidgetRebuilt() override;
	//~ End UWidget Interface

private:
	void RefreshIcon();

	UFUNCTION()
	void OnInputDeviceChanged(EKzUIInputDevice InputDevice);
};
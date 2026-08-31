// Copyright 2026 kirzo

#pragma once

#include "UObject/Interface.h"
#include "KzUITypes.h"
#include "KzInputDeviceListener.generated.h"

UINTERFACE(Blueprintable)
class KZUI_API UKzInputDeviceListener : public UInterface
{
	GENERATED_BODY()
};

/** Implemented by widgets that want to react when the player switches input device (e.g. swap button prompts). */
class KZUI_API IKzInputDeviceListener
{
	GENERATED_BODY()

public:
	UFUNCTION(Category = Input, BlueprintNativeEvent)
	void OnInputDeviceChanged(EKzUIInputDevice InputDevice);
};
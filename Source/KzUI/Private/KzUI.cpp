// Copyright 2026 kirzo

#include "KzSettingsLibrary.h"
#include "KzUIInputSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "Misc/CoreDelegates.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogKzUI);

class FKzUIModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		// Bound before any input device exists, so pads connected before a player is created are still known
		ConnectionHandle = IPlatformInputDeviceMapper::Get().GetOnInputDeviceConnectionChange().AddStatic(&UKzUIInputSubsystem::HandleInputDeviceConnectionChange);

		// The editor keeps its own gamma: a game session's saved one only applies to the game
		if (!GIsEditor)
		{
			FCoreDelegates::OnPostEngineInit.AddStatic(&UKzSettingsLibrary::ApplySavedGamma);
		}

		// Volumes need a world with its audio device, which a game instance has once its first world loads (PIE too)
		StartGameInstanceHandle = FWorldDelegates::OnStartGameInstance.AddLambda([](UGameInstance* GameInstance)
			{
				UKzSettingsLibrary::ApplyVolumes(GameInstance);
			}
		);
	}

	virtual void ShutdownModule() override
	{
		IPlatformInputDeviceMapper::Get().GetOnInputDeviceConnectionChange().Remove(ConnectionHandle);
		FWorldDelegates::OnStartGameInstance.Remove(StartGameInstanceHandle);
	}

private:
	FDelegateHandle ConnectionHandle;
	FDelegateHandle StartGameInstanceHandle;
};

IMPLEMENT_MODULE(FKzUIModule, KzUI)
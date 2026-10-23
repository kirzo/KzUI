// Copyright 2026 kirzo

#include "KzUIInputSubsystem.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogKzUI);

class FKzUIModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		// Bound before any input device exists, so pads connected before a player is created are still known
		ConnectionHandle = IPlatformInputDeviceMapper::Get().GetOnInputDeviceConnectionChange().AddStatic(&UKzUIInputSubsystem::HandleInputDeviceConnectionChange);
	}

	virtual void ShutdownModule() override
	{
		IPlatformInputDeviceMapper::Get().GetOnInputDeviceConnectionChange().Remove(ConnectionHandle);
	}

private:
	FDelegateHandle ConnectionHandle;
};

IMPLEMENT_MODULE(FKzUIModule, KzUI)
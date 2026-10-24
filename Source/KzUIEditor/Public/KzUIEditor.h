// Copyright 2026 kirzo

#pragma once

#include "Modules/ModuleInterface.h"

class FKzUIEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	FDelegateHandle BlueprintPreCompileHandle;

	FDelegateHandle ToolMenusHandle;
};
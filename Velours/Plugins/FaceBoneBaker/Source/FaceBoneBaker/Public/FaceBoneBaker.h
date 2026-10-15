// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"
#include "ToolMenuContext.h"

class FFaceBoneBakerModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void RegisterMenus();

	static void ExecuteBake(const FToolMenuContext& MenuContext);
};

// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"
#include "Logging/LogMacros.h"

BENDSDIALOGUE_API DECLARE_LOG_CATEGORY_EXTERN(LogBendsDialogue, Log, All);

class FBendsDialogueModule  : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"
#include "Logging/LogMacros.h"

BENDSCORE_API DECLARE_LOG_CATEGORY_EXTERN(LogBends, Log, All);

class FBendsCoreModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

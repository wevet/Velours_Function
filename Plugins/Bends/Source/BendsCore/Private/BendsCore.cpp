// Copyright 2022 wevet works All Rights Reserved.

#include "BendsCore.h"

#define LOCTEXT_NAMESPACE "FBendsCoreModule"

DEFINE_LOG_CATEGORY(LogBends);

void FBendsCoreModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FBendsCoreModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FBendsCoreModule, BendsCore)
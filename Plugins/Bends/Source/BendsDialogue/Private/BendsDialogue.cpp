// Copyright 2022 wevet works All Rights Reserved.

#include "BendsDialogue.h"

#define LOCTEXT_NAMESPACE "FBendsDialogueModule"

DEFINE_LOG_CATEGORY(LogBendsDialogue);

void FBendsDialogueModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FBendsDialogueModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FBendsDialogueModule, BendsDialogue)
// Copyright 2022 wevet works All Rights Reserved.

#include "Game/BendsViewMode.h"
#include "GameFramework/Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BendsViewMode)

EBendsViewMode IBendsViewModeProvider::GetViewModeOf(const AActor* Actor)
{
	if (const IBendsViewModeProvider* Provider = Cast<IBendsViewModeProvider>(Actor))
	{
		return Provider->GetViewMode();
	}
	return EBendsViewMode::FirstPerson;
}


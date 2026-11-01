// Copyright 2022 wevet works All Rights Reserved.

#include "AI/EncounterCoordinatorBase.h"
#include "Character/WvAIController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EncounterCoordinatorBase)

void UEncounterCoordinatorBase::Initialize(UWorld* InWorld)
{
	World = InWorld;
}

void UEncounterCoordinatorBase::RegisterEnemy(AWvAIController* Enemy)
{
	if (!IsValid(Enemy))
	{
		return;
	}

	ActiveEnemies.AddUnique(Enemy);
	Enemy->SetCoordinator(this);
}

void UEncounterCoordinatorBase::UnregisterEnemy(AWvAIController* Enemy)
{
	ActiveEnemies.RemoveAll([](const TWeakObjectPtr<AWvAIController>& Ptr)
		{ 
			return !Ptr.IsValid();
		});

	if (IsValid(Enemy))
	{
		ActiveEnemies.Remove(Enemy);
	}
}

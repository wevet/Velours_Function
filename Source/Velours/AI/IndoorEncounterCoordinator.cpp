// Copyright 2022 wevet works All Rights Reserved.


#include "AI/IndoorEncounterCoordinator.h"
#include "Character/WvAIController.h"
#include "GameFramework/Controller.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(IndoorEncounterCoordinator)

void UIndoorEncounterCoordinator::UpdateFlankingAssignment()
{
	float FarthestDistSq = -1.f;
	AWvAIController* Farthest = nullptr;

	for (const TWeakObjectPtr<AWvAIController>& WeakEnemy : ActiveEnemies)
	{
		AWvAIController* Enemy = WeakEnemy.Get();
		if (!IsValid(Enemy) || !IsValid(Enemy->GetPawn()))
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(Enemy->GetPawn()->GetActorLocation(), EstimatedPlayerLocation);
		if (DistSq > FarthestDistSq)
		{
			FarthestDistSq = DistSq;
			Farthest = Enemy;
		}
	}

	FlankingAgent = Farthest;
}

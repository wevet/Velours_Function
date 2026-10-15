// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AI/EncounterCoordinatorBase.h"
#include "IndoorEncounterCoordinator.generated.h"

/**
 * 
 */
UCLASS()
class VELOURS_API UIndoorEncounterCoordinator : public UEncounterCoordinatorBase
{
	GENERATED_BODY()
	
public:
	// 一番プレイヤーから離れている敵をフランキング役に選ぶ、単純なルール
	void UpdateFlankingAssignment();

	AWvAIController* GetFlankingAgent() const { return FlankingAgent.Get(); }

	UPROPERTY(EditAnywhere, Category = "Indoor")
	TArray<FVector> CoverPoints;

private:
	TWeakObjectPtr<AWvAIController> FlankingAgent;
};

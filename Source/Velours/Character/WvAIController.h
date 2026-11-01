// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Prediction.h"
#include "Perception/AISenseConfig_Damage.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Team.h"
#include "Perception/AIPerceptionComponent.h"
#include "WvAIController.generated.h"

//class UAIPerceptionComponent;
//class UAISenseConfig_Sight;
//class UAISenseConfig_Hearing;
//class UAISenseConfig_Damage;
class UStateTreeAIComponent;
class UEncounterCoordinatorBase;
class ABasePawn;

/**
 *
 */
UCLASS()
class VELOURS_API AWvAIController : public AAIController
{
	GENERATED_BODY()

public:
	AWvAIController(const FObjectInitializer& ObjectInitializer);

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	// IGenericTeamAgentInterface
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;

	// External Data用アクセサ (StateTreeのTask/Conditionから参照させる)
	AActor* GetCurrentTarget() const { return CurrentTarget.Get(); }
	void SetCurrentTarget(AActor* NewTarget) { CurrentTarget = NewTarget; }

	UEncounterCoordinatorBase* GetCoordinator() const { return Coordinator.Get(); }
	void SetCoordinator(UEncounterCoordinatorBase* NewCoordinator) { Coordinator = NewCoordinator; }

protected:
	UPROPERTY(VisibleDefaultsOnly, Category = "AI|Perception")
	TObjectPtr<UAIPerceptionComponent> AIPerceptionComponent;

	UPROPERTY(VisibleDefaultsOnly, Category = "AI|Perception")
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY(VisibleDefaultsOnly, Category = "AI|Perception")
	TObjectPtr<UAISenseConfig_Hearing> HearConfig;

	UPROPERTY(VisibleDefaultsOnly, Category = "AI|Perception")
	TObjectPtr<UAISenseConfig_Damage> DamageConfig;

	UPROPERTY(VisibleDefaultsOnly, Category = "AI|StateTree")
	TObjectPtr<UStateTreeAIComponent> StateTreeAIComponent;

	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	UFUNCTION()
	void OnTargetPerceptionUpdatedRecieve(AActor* Actor, FAIStimulus Stimulus);


private:
	TWeakObjectPtr<ABasePawn> BaseCharacter;

	UPROPERTY()
	TWeakObjectPtr<AActor> CurrentTarget;

	UPROPERTY()
	TWeakObjectPtr<UEncounterCoordinatorBase> Coordinator;
};



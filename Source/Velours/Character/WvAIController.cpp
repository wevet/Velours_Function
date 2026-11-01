// Copyright 2022 wevet works All Rights Reserved.


#include "Character/WvAIController.h"
#include "Character/BasePawn.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Damage.h"
#include "StateTreeAIComponent.h"
#include "AI/EncounterCoordinatorBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvAIController)

AWvAIController::AWvAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bWantsPlayerState = true;

	AIPerceptionComponent = ObjectInitializer.CreateDefaultSubobject<UAIPerceptionComponent>(this, TEXT("AIPerceptionComponent"));

	SightConfig = ObjectInitializer.CreateDefaultSubobject<UAISenseConfig_Sight>(this, TEXT("SightConfig"));
	SightConfig->SightRadius = 1000.f;
	SightConfig->LoseSightRadius = 1200.f;
	SightConfig->PeripheralVisionAngleDegrees = 45.f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->SetMaxAge(5.f);
	AIPerceptionComponent->ConfigureSense(*SightConfig);

	HearConfig = ObjectInitializer.CreateDefaultSubobject<UAISenseConfig_Hearing>(this, TEXT("HearConfig"));
	HearConfig->HearingRange = 1200.f;
	HearConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearConfig->DetectionByAffiliation.bDetectNeutrals = true;
	HearConfig->DetectionByAffiliation.bDetectFriendlies = true;
	HearConfig->SetMaxAge(10.f);
	AIPerceptionComponent->ConfigureSense(*HearConfig);

	DamageConfig = ObjectInitializer.CreateDefaultSubobject<UAISenseConfig_Damage>(this, TEXT("DamageConfig"));
	AIPerceptionComponent->ConfigureSense(*DamageConfig);

	AIPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());
	SetPerceptionComponent(*AIPerceptionComponent);

	StateTreeAIComponent = ObjectInitializer.CreateDefaultSubobject<UStateTreeAIComponent>(this, TEXT("StateTreeAIComponent"));
}

void AWvAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	BaseCharacter = Cast<ABasePawn>(InPawn);

	AIPerceptionComponent->OnTargetPerceptionUpdated.AddUniqueDynamic(this, &ThisClass::OnTargetPerceptionUpdated);

	// StateTreeAIComponentはAAIControllerのOnPossessで自動的にAI Startを走らせないので
	// 明示的にStartLogic相当を呼ぶ (UE5.8のStateTreeAIComponentの仕様に合わせて調整)
	StateTreeAIComponent->StartLogic();
}

void AWvAIController::OnUnPossess()
{
	AIPerceptionComponent->OnTargetPerceptionUpdated.RemoveDynamic(this, &ThisClass::OnTargetPerceptionUpdated);
	StateTreeAIComponent->StopLogic(TEXT("UnPossess"));

	CurrentTarget.Reset();
	Coordinator.Reset();
	BaseCharacter.Reset();

	Super::OnUnPossess();
}

void AWvAIController::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	// PlayerStateやチーム管理側から設定される想定。ここでは何もしない。
}

FGenericTeamId AWvAIController::GetGenericTeamId() const
{
	// Bendsは覆面の敵1種のみを想定。必要になったらPlayerState経由のチームIDに置き換える。
	return FGenericTeamId(1);
}

ETeamAttitude::Type AWvAIController::GetTeamAttitudeTowards(const AActor& Other) const
{
	if (const APawn* OtherPawn = Cast<APawn>(&Other))
	{
		if (const IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(OtherPawn->GetController()))
		{
			return TeamAgent->GetGenericTeamId() == GetGenericTeamId()
				? ETeamAttitude::Friendly
				: ETeamAttitude::Hostile;
		}
	}
	return ETeamAttitude::Neutral;
}

void AWvAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!IsValid(Actor))
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		SetCurrentTarget(Actor);
	}
	else if (GetCurrentTarget() == Actor)
	{
		// 見失った場合。すぐにクリアせず、StateTree側のTransitionで
		// LastKnownLocation等を使いたい場合はここでフラグだけ立てる設計も検討
		SetCurrentTarget(nullptr);
	}
}

void AWvAIController::OnTargetPerceptionUpdatedRecieve(AActor* Actor, FAIStimulus Stimulus)
{
}




// Copyright 2022 wevet works All Rights Reserved.

#include "Interaction/BendsInteractableComponent.h"
#include "Interaction/BendsInteractionEffect.h"
#include "Interaction/BendsInteractionSubsystem.h"
#include "Episode/BendsEpisodeRule.h"
#include "Game/BendsGameModeBase.h"
#include "Progress/BendsProgressSubsystem.h"
#include "BendsInteraction.h"
#include "Animation/AnimMontage.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BendsInteractableComponent)

UBendsInteractableComponent::UBendsInteractableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UBendsInteractableComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UBendsInteractionSubsystem* Subsystem = GetWorld()->GetSubsystem<UBendsInteractionSubsystem>())
	{
		Subsystem->Register(this);
	}

	if (bOneShot)
	{
		if (!InteractionId.IsValid())
		{
			UE_LOG(LogBendsInteraction, Warning, TEXT("%s: one-shot interactable has no InteractionId. Its used state is not saved."), *GetNameSafe(GetOwner()));
		}
		else if (const UBendsProgressSubsystem* Progress = UBendsProgressSubsystem::Get(this))
		{
			// チェックポイントからのやり直しで、使用済みの状態を戻す
			bUsed = Progress->GetFlag(InteractionId) > 0;
			if (bUsed)
			{
				ApplyUsedState();
			}
		}
	}
}

void UBendsInteractableComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UBendsInteractionSubsystem* Subsystem = World->GetSubsystem<UBendsInteractionSubsystem>())
		{
			Subsystem->Unregister(this);
		}
	}
	Super::EndPlay(EndPlayReason);
}

bool UBendsInteractableComponent::CanInteract(const APawn* Instigator) const
{
	if (!bInteractionEnabled || (bOneShot && bUsed))
	{
		return false;
	}

	if (!RequiredPhases.IsEmpty())
	{
		const UBendsEpisodeRule* Rule = ABendsGameModeBase::GetRule(this);
		if (!Rule || !Rule->GetCurrentPhase().MatchesAny(RequiredPhases))
		{
			return false;
		}
	}

	if (RequiredFlags.Num() > 0)
	{
		const UBendsProgressSubsystem* Progress = UBendsProgressSubsystem::Get(this);
		if (!Progress)
		{
			return false;
		}
		for (const FBendsInteractionFlagCondition& Condition : RequiredFlags)
		{
			if (Progress->GetFlag(Condition.Flag) < Condition.MinValue)
			{
				return false;
			}
		}
	}
	return true;
}

void UBendsInteractableComponent::Interact(const FBendsInteractionContext& Context)
{
	// 先に使用済みにする（Effectでチェックポイントのあるフェーズへ進んでも、拾った状態で保存されるように）
	if (bOneShot)
	{
		MarkUsed();
	}

	for (UBendsInteractionEffect* Effect : Effects)
	{
		if (Effect)
		{
			Effect->Execute(Context);
		}
	}

	InteractedEvent.Broadcast(this, Context);
}


void UBendsInteractableComponent::MarkUsed()
{
	bUsed = true;

	if (InteractionId.IsValid())
	{
		if (UBendsProgressSubsystem* Progress = UBendsProgressSubsystem::Get(this))
		{
			Progress->SetFlag(InteractionId, 1);
		}
	}
	ApplyUsedState();
}

void UBendsInteractableComponent::ApplyUsedState()
{
	if (bHideOwnerWhenUsed)
	{
		if (AActor* Owner = GetOwner())
		{
			Owner->SetActorHiddenInGame(true);
			Owner->SetActorEnableCollision(false);
		}
	}
}


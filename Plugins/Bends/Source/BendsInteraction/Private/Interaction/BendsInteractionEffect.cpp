// Copyright 2022 wevet works All Rights Reserved.

#include "Interaction/BendsInteractionEffect.h"
#include "Interaction/BendsInteractableComponent.h"
#include "Episode/BendsEpisodeRule.h"
#include "Game/BendsGameModeBase.h"
#include "GameFramework/Pawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BendsInteractionEffect)

UBendsEpisodeRule* UBendsInteractionEffect::GetRule(const FBendsInteractionContext& Context)
{
	const UObject* WorldContext = Context.Interactable ? static_cast<const UObject*>(Context.Interactable) : Context.Instigator;
	return ABendsGameModeBase::GetRule(WorldContext);
}

void UBendsInteractionEffect_Objective::Execute(const FBendsInteractionContext& Context)
{
	if (UBendsEpisodeRule* Rule = GetRule(Context))
	{
		Rule->NotifyObjective(Objective);
	}
}

void UBendsInteractionEffect_Flag::Execute(const FBendsInteractionContext& Context)
{
	if (UBendsEpisodeRule* Rule = GetRule(Context))
	{
		Rule->AddFlag(Flag, Delta);
	}
}

void UBendsInteractionEffect_Trigger::Execute(const FBendsInteractionContext& Context)
{
	if (UBendsEpisodeRule* Rule = GetRule(Context))
	{
		FBendsTriggerContext TriggerContext;
		TriggerContext.Instigator = Context.Instigator;
		TriggerContext.Target = Context.Target;
		TriggerContext.ViewMode = Context.ViewMode;
		Rule->SendTrigger(Trigger, TriggerContext);
	}
}

void UBendsInteractionEffect_ResolveHold::Execute(const FBendsInteractionContext& Context)
{
	if (UBendsEpisodeRule* Rule = GetRule(Context))
	{
		Rule->ResolveHold(HoldTag);
	}
}


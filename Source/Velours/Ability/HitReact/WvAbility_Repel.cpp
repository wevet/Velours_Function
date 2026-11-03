// Copyright 2022 wevet works All Rights Reserved.

#include "Ability/HitReact/WvAbility_Repel.h"
#include "WvAbilitySystemTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvAbility_Repel)

UWvAbility_Repel::UWvAbility_Repel()
{
	FAbilityTriggerData Trigger;
	Trigger.TriggerTag = TAG_Common_PassiveAbilityTrigger_HitReact;
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(Trigger);
}


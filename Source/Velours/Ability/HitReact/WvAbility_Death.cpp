// Copyright 2022 wevet works All Rights Reserved.

#include "Ability/HitReact/WvAbility_Death.h"
#include "Character/BasePawn.h"
#include "WvAbilitySystemTypes.h"
#include "Animation/AnimMontage.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvAbility_Death)

UWvAbility_Death::UWvAbility_Death()
{
	bStopWhenAbilityEnds = false;

	FAbilityTriggerData Trigger;
	Trigger.TriggerTag = TAG_Common_PassiveAbilityTrigger_KillReact;
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(Trigger);
}

void UWvAbility_Death::ModifyQuery(FVHitReactQuery& Query) const
{
	Query.Feature = TAG_Config_HitReactFeature_Dead;
}

void UWvAbility_Death::OnMontageStarted(ABasePawn& Character, UAnimMontage& Montage)
{
	const float RagdollDelay = FMath::Max(0.f, Montage.GetPlayLength() - MontageTrimTime);

	// TODO: 移植元のBeginDeathAction / EndDeathAction(RagdollDelay)に当たる処理をABasePawnに用意して呼ぶ
	//       （入力の停止、コリジョンの切り替え、RagdollDelay秒後のラグドールなど）
	UE_LOG(LogTemp, Log, TEXT("[%s] Death montage %s, ragdoll after %.2fs"), *FString(__FUNCTION__), *GetNameSafe(&Montage), RagdollDelay);
}


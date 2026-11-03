// Copyright 2022 wevet works All Rights Reserved.

#include "Ability/HitReact/WvAbility_HitReactBase.h"
#include "Ability/Task/WvAT_PlayMontageAndWaitForEvent.h"
#include "Character/BasePawn.h"
#include "WvAbilitySystemBlueprintFunctionLibrary.h"
#include "Component/CombatComponent.h"


#include "Animation/AnimMontage.h"
#include "DrawDebugHelpers.h"
#include "HAL/IConsoleManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvAbility_HitReactBase)

namespace
{
	// 既存の戦闘のデバッグと同じCVarで表示する
	bool IsHitReactDebugEnabled()
	{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		static const IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("wv.CombatSystem.Debug"));
		return CVar && CVar->GetInt() > 0;
#else
		return false;
#endif
	}
}

UWvAbility_HitReactBase::UWvAbility_HitReactBase()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	bRetriggerInstancedAbility = true;
}

void UWvAbility_HitReactBase::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ABasePawn* Character = GetBaseCharacter();
	UCombatComponent* Combat = Character ? Character->GetCombatComponent() : nullptr;
	if (!Combat || !TriggerEventData)
	{
		UE_LOG(LogWvGameplayAbility, Warning, TEXT("[%s] CombatComponent or TriggerEventData is null."), *FString(__FUNCTION__));
		CancelAbility(Handle, ActorInfo, ActivationInfo, false);
		return;
	}

	FVHitReactQuery Query = Combat->BuildHitReactQuery(*TriggerEventData);
	ModifyQuery(Query);

	UAnimMontage* Montage = Combat->ChooseHitReactMontage(Query);

	if (IsHitReactDebugEnabled())
	{
		DrawDebug(*Character, *TriggerEventData, Query, Montage);
	}

	if (!IsValid(Montage))
	{
		UE_LOG(LogWvGameplayAbility, Warning, TEXT("[%s] No montage. Feature=%s Source=%s Dir=%s Vert=%s Crouch=%d"),
			*FString(__FUNCTION__), *Query.Feature.ToString(),
			*UEnum::GetValueAsString(Query.Source),
			*UEnum::GetValueAsString(Query.Direction),
			*UEnum::GetValueAsString(Query.Vertical),
			Query.bIsCrouched ? 1 : 0);
		CancelAbility(Handle, ActorInfo, ActivationInfo, false);
		return;
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	PlayMontage(Montage);
	OnMontageStarted(*Character, *Montage);
}

void UWvAbility_HitReactBase::PlayMontage(UAnimMontage* Montage)
{
	// 連続で被弾したら、前のMontageのコールバックを外してから再生し直す
	if (MontageTask)
	{
		MontageTask->OnCompleted.Clear();
		MontageTask->OnBlendOut.Clear();
		MontageTask->OnCancelled.Clear();
		MontageTask->OnInterrupted.Clear();
		MontageTask->EndTask();
		MontageTask = nullptr;
	}

	MontageTask = UWvAT_PlayMontageAndWaitForEvent::PlayMontageAndWaitForEvent(
		this,
		FName("HitReact"),
		Montage,
		FGameplayTagContainer(),
		1.0,
		0.f,
		FName("Default"),
		bStopWhenAbilityEnds,
		1.0f);

	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnPlayMontageCompleted_Event);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnPlayMontageInterrupted_Event);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnPlayMontageInterrupted_Event);
	MontageTask->ReadyForActivation();
}

void UWvAbility_HitReactBase::OnPlayMontageCompleted_Event(FGameplayTag EventTag, FGameplayEventData EventData)
{
	OnMontageFinished(false);
	K2_EndAbility();
}

void UWvAbility_HitReactBase::OnPlayMontageInterrupted_Event(FGameplayTag EventTag, FGameplayEventData EventData)
{
	OnMontageFinished(true);
	K2_EndAbility();
}

void UWvAbility_HitReactBase::DrawDebug(const ABasePawn& Character, const FGameplayEventData& EventData, const FVHitReactQuery& Query, const UAnimMontage* Montage) const
{
#if ENABLE_DRAW_DEBUG
	const FVector Location = Character.GetActorLocation();
	const FVector AttackDirection = UWvAbilitySystemBlueprintFunctionLibrary::GetAttackDirection(EventData.ContextHandle, Location);

	DrawDebugLine(GetWorld(), Location, Location + AttackDirection * 100.f, FColor::Red, false, DrawDebugTime, 0, 2.f);
	if (const FHitResult* Hit = EventData.ContextHandle.GetHitResult())
	{
		DrawDebugPoint(GetWorld(), Hit->ImpactPoint, 16.f, FColor::Orange, false, DrawDebugTime);
	}

	UE_LOG(LogTemp, Log, TEXT("HitReact Feature=%s Source=%s Attacker=%s Victim=%s Dir=%s Vert=%s Crouch=%d -> %s"),
		*Query.Feature.ToString(),
		*UEnum::GetValueAsString(Query.Source),
		*UEnum::GetValueAsString(Query.AttackerWeapon),
		*UEnum::GetValueAsString(Query.VictimWeapon),
		*UEnum::GetValueAsString(Query.Direction),
		*UEnum::GetValueAsString(Query.Vertical),
		Query.bIsCrouched ? 1 : 0,
		*GetNameSafe(Montage));
#endif
}


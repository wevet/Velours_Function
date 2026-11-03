// Copyright 2022 wevet works All Rights Reserved.


#include "Ability/VAbility_Interact.h"
#include "Ability/VInteractionAnimQuery.h"
#include "Interaction/BendsInteractableComponent.h"
#include "Interaction/BendsInteractorComponent.h"
#include "BendsInteractionTags.h"
#include "Velours.h"

#include "MotionWarpingComponent.h"
#include "Chooser.h"
#include "ChooserFunctionLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(VAbility_Interact)


UVAbility_Interact::UVAbility_Interact()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	FAbilityTriggerData Trigger;
	Trigger.TriggerTag = TAG_Ability_Interact;
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(Trigger);
}

void UVAbility_Interact::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		FinishWith(BendsInteractionTags::Event_Cancelled, true);
		return;
	}

	Interactable = TriggerEventData ? Cast<UBendsInteractableComponent>(TriggerEventData->OptionalObject) : nullptr;
	const EBendsViewMode ViewMode = (TriggerEventData && TriggerEventData->EventMagnitude > 0.5f)
		? EBendsViewMode::ThirdPerson : EBendsViewMode::FirstPerson;

	const UBendsInteractableComponent* Target = Interactable.Get();
	UAnimMontage* Montage = Target ? ChooseMontage(*Target, ViewMode) : nullptr;
	if (!Montage)
	{
		// Montageがなければ、すぐ終える（インタラクターがFinishedで結果を出す）
		FinishWith(BendsInteractionTags::Event_Finished, false);
		return;
	}

	if (AActor* Avatar = GetAvatarActorFromActorInfo())
	{
		if (UMotionWarpingComponent* Warping = Avatar->FindComponentByClass<UMotionWarpingComponent>())
		{
			Warping->AddOrUpdateWarpTargetFromTransform(Interactable->WarpTargetName, Interactable->GetComponentTransform());
		}
	}

	MontageTask = UWvAT_PlayMontageAndWaitForEvent::PlayMontageAndWaitForEvent(
		this,
		FName("Interact"),
		Montage,
		FGameplayTagContainer(),
		1.0,
		0.f,
		FName("Default"),
		true,
		1.0f);

	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnPlayMontageCompleted_Event);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnPlayMontageCompleted_Event);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnPlayMontageInterrupted_Event);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnPlayMontageInterrupted_Event);
	MontageTask->ReadyForActivation();
}

UAnimMontage* UVAbility_Interact::ChooseMontage(const UBendsInteractableComponent& Target, EBendsViewMode ViewMode) const
{
	// この物だけの特別な動作を優先する
	if (UAnimMontage* Override = Target.MontageOverride.LoadSynchronous())
	{
		return Override;
	}

	if (!MontageChooser)
	{
		UE_LOG(LogWvGameplayAbility, Warning, TEXT("%s: MontageChooser is not set."), *GetNameSafe(this));
		return nullptr;
	}

	FVInteractionAnimQuery Query;
	Query.AnimationTag = Target.AnimationTag;
	Query.ViewMode = ViewMode;
	if (const AActor* Avatar = GetAvatarActorFromActorInfo())
	{
		Query.HeightOffset = Target.GetComponentLocation().Z - Avatar->GetActorLocation().Z;
	}

	FChooserEvaluationContext Context;
	Context.AddStructParam(Query);

	FInstancedStruct ChooserStruct = UChooserFunctionLibrary::MakeEvaluateChooser(MontageChooser);
	UObject* Result = UChooserFunctionLibrary::EvaluateObjectChooserBase(Context, ChooserStruct, UAnimMontage::StaticClass(), false);

	UAnimMontage* Montage = Cast<UAnimMontage>(Result);
	if (!Montage)
	{
		UE_LOG(LogWvGameplayAbility, Warning, TEXT("No montage for %s (view %d, height %.1f)."),
			*Query.AnimationTag.ToString(), static_cast<int32>(ViewMode), Query.HeightOffset);
	}
	return Montage;
}


void UVAbility_Interact::OnPlayMontageCompleted_Event(FGameplayTag EventTag, FGameplayEventData EventData)
{
	FinishWith(BendsInteractionTags::Event_Finished, false);
}

void UVAbility_Interact::OnPlayMontageInterrupted_Event(FGameplayTag EventTag, FGameplayEventData EventData)
{
	FinishWith(BendsInteractionTags::Event_Cancelled, true);
}

void UVAbility_Interact::FinishWith(const FGameplayTag& EventTag, bool bWasCancelled)
{
	if (!IsActive())
	{
		return;
	}

	UBendsInteractorComponent::SendInteractionEvent(GetAvatarActorFromActorInfo(), EventTag, Interactable.Get());
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bWasCancelled);
}



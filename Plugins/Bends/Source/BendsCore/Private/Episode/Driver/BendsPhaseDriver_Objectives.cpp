// Copyright 2022 wevet works All Rights Reserved.

#include "Episode/Driver/BendsPhaseDriver_Objectives.h"
#include "Episode/BendsEpisodeRule.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BendsPhaseDriver_Objectives)

void UBendsPhaseDriver_Objectives::OnBegin()
{
	Super::OnBegin();
	Completed.Reset();

	// 必須がない場合は、入った時点で完了扱いにはしない（外からFinishを待つ）
}

bool UBendsPhaseDriver_Objectives::NotifyObjective(const FGameplayTag& Objective)
{
	if (!IsActive() || !Objective.IsValid() || Completed.Contains(Objective))
	{
		return false;
	}

	const bool bRequired = RequiredObjectives.Contains(Objective);
	const bool bOptional = OptionalObjectives.Contains(Objective);
	if (!bRequired && !bOptional)
	{
		return false;
	}

	Completed.Add(Objective);

	if (bOptional && OptionalCountFlag.IsValid())
	{
		if (UBendsEpisodeRule* Rule = GetRule())
		{
			Rule->AddFlag(OptionalCountFlag);
		}
	}

	ObjectiveDoneEvent.Broadcast(Objective);

	if (bRequired && bFinishWhenRequiredDone && AreRequiredDone())
	{
		Finish();
	}
	return true;
}

bool UBendsPhaseDriver_Objectives::AreRequiredDone() const
{
	for (const FGameplayTag& Tag : RequiredObjectives)
	{
		if (!Completed.Contains(Tag))
		{
			return false;
		}
	}
	return true;
}


// Copyright 2022 wevet works All Rights Reserved.

#include "Episode/Driver/BendsPhaseDriver_Timeline.h"
#include "Episode/BendsEpisodeRule.h"
#include "BendsCore.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BendsPhaseDriver_Timeline)


void UBendsPhaseDriver_Timeline::OnBegin()
{
	Super::OnBegin();

	UBendsEpisodeRule* Rule = GetRule();
	UBendsOperationClock* RuleClock = Rule ? Rule->GetClock() : nullptr;
	if (!RuleClock)
	{
		UE_LOG(LogBends, Warning, TEXT("Timeline driver: the rule has no clock. Call CreateClock in BuildPhases."));
		return;
	}

	Clock = RuleClock;
	ClockEventHandle = RuleClock->OnClockEvent().AddUObject(this, &UBendsPhaseDriver_Timeline::HandleClockEvent);
	RuleClock->StartAt(StartTime);
}

void UBendsPhaseDriver_Timeline::OnEnd()
{
	if (UBendsOperationClock* RuleClock = Clock.Get())
	{
		RuleClock->OnClockEvent().Remove(ClockEventHandle);
	}
	ClockEventHandle.Reset();
	Clock.Reset();

	Super::OnEnd();
}

void UBendsPhaseDriver_Timeline::ResolveHold(const FGameplayTag& EventTag)
{
	UBendsOperationClock* RuleClock = Clock.Get();
	if (!IsActive() || !RuleClock)
	{
		return;
	}

	RuleClock->ReleaseHold(EventTag);

	if (EventTag == FinishTag)
	{
		Finish();
	}
}

void UBendsPhaseDriver_Timeline::HandleClockEvent(const FGameplayTag& EventTag, bool bHold)
{
	// ホールドでないFinishTagは、時刻が来た時点で完了
	if (!bHold && EventTag == FinishTag)
	{
		Finish();
	}
}


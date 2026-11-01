// Copyright 2022 wevet works All Rights Reserved.


#include "Episode/Driver/BendsPhaseDriver.h"
#include "Episode/BendsEpisodeRule.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BendsPhaseDriver)

void UBendsPhaseDriver::Begin()
{
	bActive = true;
	bResultSent = false;
	OnBegin();
}

void UBendsPhaseDriver::End()
{
	if (!bActive)
	{
		return;
	}
	bActive = false;
	OnEnd();
}

void UBendsPhaseDriver::Tick(float DeltaTime)
{
	if (WantsTick())
	{
		OnTick(DeltaTime);
	}
}

void UBendsPhaseDriver::Finish(const FGameplayTag& NextPhase)
{
	if (!bActive || bResultSent)
	{
		return;
	}
	bResultSent = true;

	if (UBendsEpisodeRule* Rule = GetRule())
	{
		Rule->HandleDriverFinished(this, NextPhase);
	}
}

void UBendsPhaseDriver::Fail()
{
	if (!bActive || bResultSent)
	{
		return;
	}
	bResultSent = true;

	if (UBendsEpisodeRule* Rule = GetRule())
	{
		Rule->HandleDriverFailed(this);
	}
}

UWorld* UBendsPhaseDriver::GetWorld() const
{
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return nullptr;
	}
	const UObject* Outer = GetOuter();
	return Outer ? Outer->GetWorld() : nullptr;
}

UBendsEpisodeRule* UBendsPhaseDriver::GetRule() const
{
	return Cast<UBendsEpisodeRule>(GetOuter());
}


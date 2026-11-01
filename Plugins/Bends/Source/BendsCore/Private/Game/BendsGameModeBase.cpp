// Copyright 2022 wevet works All Rights Reserved.


#include "Game/BendsGameModeBase.h"
#include "Episode/BendsEpisodeRule.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BendsGameModeBase)

void ABendsGameModeBase::StartPlay()
{
	if (RuleClass)
	{
		Rule = NewObject<UBendsEpisodeRule>(this, RuleClass);
		Rule->Initialize(this);
	}

	// ここでアクターのBeginPlayが走る
	Super::StartPlay();

	if (Rule)
	{
		Rule->BeginEpisode();
	}
}

void ABendsGameModeBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Rule)
	{
		Rule->Shutdown();
	}
	Super::EndPlay(EndPlayReason);
}


// Copyright 2022 wevet works All Rights Reserved.


#include "Progress/BendsProgressSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BendsProgressSubsystem)

UBendsProgressSubsystem* UBendsProgressSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UBendsProgressSubsystem>() : nullptr;
}

void UBendsProgressSubsystem::AddFlag(const FGameplayTag& Flag, int32 Delta)
{
	if (Flag.IsValid())
	{
		Flags.FindOrAdd(Flag) += Delta;
	}
}

void UBendsProgressSubsystem::SetFlag(const FGameplayTag& Flag, int32 Value)
{
	if (Flag.IsValid())
	{
		Flags.FindOrAdd(Flag) = Value;
	}
}

int32 UBendsProgressSubsystem::GetFlag(const FGameplayTag& Flag) const
{
	const int32* Value = Flags.Find(Flag);
	return Value ? *Value : 0;
}

void UBendsProgressSubsystem::WriteCheckpoint(const FGameplayTag& Episode, const FGameplayTag& Phase)
{
	CheckpointEpisode = Episode;
	CheckpointPhase = Phase;
	CheckpointFlags = Flags; // チェックポイント以降に立ったフラグは、再開時に戻す
}

bool UBendsProgressSubsystem::RequestResume(const FGameplayTag& Episode)
{
	if (!HasCheckpoint(Episode))
	{
		return false;
	}
	Flags = CheckpointFlags;
	bResumePending = true;
	return true;
}

bool UBendsProgressSubsystem::ConsumeResume(const FGameplayTag& Episode, FGameplayTag& OutPhase)
{
	if (!bResumePending || CheckpointEpisode != Episode)
	{
		return false;
	}
	bResumePending = false;
	OutPhase = CheckpointPhase;
	return true;
}

bool UBendsProgressSubsystem::HasCheckpoint(const FGameplayTag& Episode) const
{
	return CheckpointEpisode.IsValid() && CheckpointEpisode == Episode && CheckpointPhase.IsValid();
}

void UBendsProgressSubsystem::ResetProgress()
{
	Flags.Reset();
	CheckpointFlags.Reset();
	CheckpointEpisode = FGameplayTag();
	CheckpointPhase = FGameplayTag();
	bResumePending = false;
}



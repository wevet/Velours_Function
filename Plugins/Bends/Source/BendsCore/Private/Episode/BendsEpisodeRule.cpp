// Copyright 2022 wevet works All Rights Reserved.


#include "Episode/BendsEpisodeRule.h"
#include "Game/BendsGameModeBase.h"
#include "Episode/Driver/BendsPhaseDriver.h"
#include "Episode/Driver/BendsPhaseDriver_Objectives.h"
#include "Episode/Driver/BendsPhaseDriver_Timeline.h"
#include "Episode/BendsOperationClock.h"
#include "Progress/BendsProgressSubsystem.h"
#include "BendsCore.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BendsEpisodeRule)

void UBendsEpisodeRule::Initialize(ABendsGameModeBase* InGameMode)
{
	GameMode = InGameMode;
	BuildPhases();
}

void UBendsEpisodeRule::BeginEpisode()
{
	if (bRunning)
	{
		return;
	}
	bRunning = true;

	// チェックポイントからの再開なら、そのフェーズから始める
	FGameplayTag StartPhase;
	if (UBendsProgressSubsystem* Progress = GetProgress())
	{
		Progress->ConsumeResume(EpisodeId, StartPhase);
	}
	if (!StartPhase.IsValid() && Phases.Num() > 0)
	{
		StartPhase = Phases[0].Phase;
	}

	UE_LOG(LogBends, Log, TEXT("BeginEpisode %s from %s"), *EpisodeId.ToString(), *StartPhase.ToString());

	OnEpisodeBegin();

	if (StartPhase.IsValid())
	{
		SetPhase(StartPhase);
	}
}

void UBendsEpisodeRule::Shutdown()
{
	EndCurrentDriver();
	if (Clock)
	{
		Clock->Stop();
	}
	bRunning = false;
	PendingPhase = FGameplayTag();
	PhaseChangedEvent.Clear();
	EpisodeEndedEvent.Clear();
}

void UBendsEpisodeRule::AddPhaseDriver(const FGameplayTag& Phase, UBendsPhaseDriver* Driver, bool bCheckpoint)
{
	if (!Phase.IsValid() || FindPhaseIndex(Phase) != INDEX_NONE)
	{
		UE_LOG(LogBends, Warning, TEXT("AddPhase: invalid or duplicated phase %s"), *Phase.ToString());
		return;
	}

	FBendsPhaseEntry& Entry = Phases.AddDefaulted_GetRef();
	Entry.Phase = Phase;
	Entry.Driver = Driver;
	Entry.bCheckpoint = bCheckpoint;
}

UBendsOperationClock* UBendsEpisodeRule::CreateClock(const TArray<FBendsClockEvent>& Events, const TArray<FBendsClockSpeed>& Speeds)
{
	Clock = NewObject<UBendsOperationClock>(this);
	Clock->Setup(Events, Speeds);
	return Clock;
}

void UBendsEpisodeRule::SetPhase(const FGameplayTag& NewPhase)
{
	if (!bRunning || !NewPhase.IsValid() || NewPhase == CurrentPhase)
	{
		return;
	}

	// フックやドライバーの中から呼ばれたら、今の遷移が終わってから処理する
	if (bInTransition)
	{
		PendingPhase = NewPhase;
		return;
	}

	const int32 Index = FindPhaseIndex(NewPhase);
	if (Index == INDEX_NONE)
	{
		UE_LOG(LogBends, Warning, TEXT("SetPhase: %s is not registered"), *NewPhase.ToString());
		return;
	}

	bInTransition = true;

	const FGameplayTag OldPhase = CurrentPhase;
	EndCurrentDriver();
	if (OldPhase.IsValid())
	{
		OnPhaseExited(OldPhase);
	}

	const FBendsPhaseEntry& Entry = Phases[Index];
	CurrentPhase = Entry.Phase;
	CurrentDriver = Entry.Driver;
	TimeInPhase = 0.f;

	if (Entry.bCheckpoint)
	{
		SaveCheckpoint();
	}

	UE_LOG(LogBends, Log, TEXT("Phase %s -> %s"), *OldPhase.ToString(), *CurrentPhase.ToString());

	OnPhaseEntered(CurrentPhase);
	PhaseChangedEvent.Broadcast(OldPhase, CurrentPhase);

	// フックとイベントの後に始める（ドライバーがすぐ完了しても順番が崩れない）
	if (CurrentDriver)
	{
		CurrentDriver->Begin();
	}

	bInTransition = false;

	if (PendingPhase.IsValid())
	{
		const FGameplayTag Next = PendingPhase;
		PendingPhase = FGameplayTag();
		SetPhase(Next);
	}
}

bool UBendsEpisodeRule::AdvancePhase()
{
	const int32 Index = FindPhaseIndex(CurrentPhase);
	if (Index == INDEX_NONE || !Phases.IsValidIndex(Index + 1))
	{
		return false;
	}
	SetPhase(Phases[Index + 1].Phase);
	return true;
}

void UBendsEpisodeRule::CompleteEpisode()
{
	if (!bRunning)
	{
		return;
	}
	EndCurrentDriver();
	if (CurrentPhase.IsValid())
	{
		OnPhaseExited(CurrentPhase);
	}
	bRunning = false;

	OnEpisodeEnd(true);
	EpisodeEndedEvent.Broadcast(true);
}

void UBendsEpisodeRule::HandleDriverFinished(UBendsPhaseDriver* Driver, const FGameplayTag& NextPhase)
{
	if (!bRunning || Driver != CurrentDriver)
	{
		return;
	}

	if (NextPhase.IsValid())
	{
		SetPhase(NextPhase);
	}
	else if (!AdvancePhase())
	{
		CompleteEpisode(); // 最後のフェーズが終わった
	}
}

void UBendsEpisodeRule::HandleDriverFailed(UBendsPhaseDriver* Driver)
{
	if (!bRunning || Driver != CurrentDriver)
	{
		return;
	}
	OnPhaseFailed(CurrentPhase);
}

void UBendsEpisodeRule::OnPhaseFailed(const FGameplayTag& Phase)
{
	RestartFromCheckpoint();
}

bool UBendsEpisodeRule::NotifyObjective(const FGameplayTag& Objective)
{
	if (UBendsPhaseDriver_Objectives* Objectives = Cast<UBendsPhaseDriver_Objectives>(CurrentDriver))
	{
		return Objectives->NotifyObjective(Objective);
	}
	return false;
}

void UBendsEpisodeRule::ResolveHold(const FGameplayTag& EventTag)
{
	if (UBendsPhaseDriver_Timeline* Timeline = Cast<UBendsPhaseDriver_Timeline>(CurrentDriver))
	{
		Timeline->ResolveHold(EventTag);
	}
	else if (Clock)
	{
		Clock->ReleaseHold(EventTag);
	}
}

void UBendsEpisodeRule::SaveCheckpoint()
{
	if (UBendsProgressSubsystem* Progress = GetProgress())
	{
		Progress->WriteCheckpoint(EpisodeId, CurrentPhase);
	}
}

void UBendsEpisodeRule::RestartFromCheckpoint()
{
	UBendsProgressSubsystem* Progress = GetProgress();
	if (!Progress || !Progress->RequestResume(EpisodeId))
	{
		UE_LOG(LogBends, Warning, TEXT("No checkpoint for %s"), *EpisodeId.ToString());
		return;
	}
	const FString LevelName = UGameplayStatics::GetCurrentLevelName(this, true);
	UGameplayStatics::OpenLevel(this, FName(*LevelName));
}

void UBendsEpisodeRule::OnPlayerDied(AController* Controller)
{
	RestartFromCheckpoint();
}

void UBendsEpisodeRule::AddFlag(const FGameplayTag& Flag, int32 Delta)
{
	if (UBendsProgressSubsystem* Progress = GetProgress())
	{
		Progress->AddFlag(Flag, Delta);
	}
}

int32 UBendsEpisodeRule::GetFlag(const FGameplayTag& Flag) const
{
	const UBendsProgressSubsystem* Progress = GetProgress();
	return Progress ? Progress->GetFlag(Flag) : 0;
}

UWorld* UBendsEpisodeRule::GetWorld() const
{
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return nullptr;
	}
	const UObject* Outer = GetOuter();
	return Outer ? Outer->GetWorld() : nullptr;
}

void UBendsEpisodeRule::Tick(float DeltaTime)
{
	TimeInPhase += DeltaTime;

	if (CurrentDriver)
	{
		CurrentDriver->Tick(DeltaTime);
	}
	if (bPhaseTickEnabled && CurrentPhase.IsValid())
	{
		OnPhaseTick(CurrentPhase, DeltaTime);
	}
}

TStatId UBendsEpisodeRule::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UBendsEpisodeRule, STATGROUP_Tickables);
}

ETickableTickType UBendsEpisodeRule::GetTickableTickType() const
{
	return HasAnyFlags(RF_ClassDefaultObject) ? ETickableTickType::Never : ETickableTickType::Conditional;
}

UBendsProgressSubsystem* UBendsEpisodeRule::GetProgress() const
{
	return UBendsProgressSubsystem::Get(this);
}

int32 UBendsEpisodeRule::FindPhaseIndex(const FGameplayTag& Phase) const
{
	return Phases.IndexOfByPredicate([&Phase](const FBendsPhaseEntry& Entry)
		{
			return Entry.Phase == Phase;
		});
}

void UBendsEpisodeRule::EndCurrentDriver()
{
	if (UBendsPhaseDriver* Driver = CurrentDriver)
	{
		CurrentDriver = nullptr;
		Driver->End();
	}
}


// Copyright 2022 wevet works All Rights Reserved.


#include "Episode/BendsOperationClock.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BendsOperationClock)


void UBendsOperationClock::Setup(const TArray<FBendsClockEvent>& InEvents, const TArray<FBendsClockSpeed>& InSpeeds)
{
	Events = InEvents;
	Events.StableSort([](const FBendsClockEvent& A, const FBendsClockEvent& B)
		{
			return A.Time.ToSeconds() < B.Time.ToSeconds();
		});

	Speeds = InSpeeds;
	Speeds.StableSort([](const FBendsClockSpeed& A, const FBendsClockSpeed& B)
		{
			return A.From.ToSeconds() < B.From.ToSeconds();
		});
}

void UBendsOperationClock::StartAt(const FBendsClockTime& Time, bool bStartPaused)
{
	CurrentSeconds = Time.ToSeconds();
	HoldingTag = FGameplayTag();
	bRunning = true;
	bPaused = bStartPaused;

	// 開始時刻より前のイベントは飛ばす（同時刻のイベントは発火させる）
	NextEventIndex = 0;
	while (Events.IsValidIndex(NextEventIndex) && Events[NextEventIndex].Time.ToSeconds() < CurrentSeconds)
	{
		++NextEventIndex;
	}

	LastWholeSecond = GetTotalSeconds();
	SecondChanged.Broadcast(LastWholeSecond);
	FireDueEvents();
}

void UBendsOperationClock::Stop()
{
	bRunning = false;
	HoldingTag = FGameplayTag();
}

void UBendsOperationClock::ReleaseHold(const FGameplayTag& EventTag)
{
	if (HoldingTag.IsValid() && HoldingTag == EventTag)
	{
		HoldingTag = FGameplayTag();
		FireDueEvents(); // 同じ時刻に続くイベントがあれば発火
	}
}

FText UBendsOperationClock::GetDisplayText() const
{
	const int32 Total = GetTotalSeconds();
	return FText::FromString(FString::Printf(TEXT("%02d:%02d:%02d"),
		(Total / 3600) % 24, (Total / 60) % 60, Total % 60));
}

UWorld* UBendsOperationClock::GetWorld() const
{
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return nullptr;
	}
	const UObject* Outer = GetOuter();
	return Outer ? Outer->GetWorld() : nullptr;
}

void UBendsOperationClock::Tick(float DeltaTime)
{
	if (!HoldingTag.IsValid())
	{
		CurrentSeconds += DeltaTime * GetCurrentScale();
		FireDueEvents();
	}

	const int32 Whole = GetTotalSeconds();
	if (Whole != LastWholeSecond)
	{
		LastWholeSecond = Whole;
		SecondChanged.Broadcast(Whole);
	}
}

void UBendsOperationClock::FireDueEvents()
{
	while (bRunning && !HoldingTag.IsValid() && Events.IsValidIndex(NextEventIndex))
	{
		const FBendsClockEvent& Event = Events[NextEventIndex];
		const double EventSeconds = Event.Time.ToSeconds();
		if (EventSeconds > CurrentSeconds)
		{
			break;
		}

		++NextEventIndex;

		// 受け側でSetupやStartAtが呼ばれても安全なように、先にコピーする
		const FGameplayTag Tag = Event.EventTag;
		const bool bHold = Event.bHold;

		if (bHold)
		{
			// 時計をイベントの時刻に揃えて止める
			CurrentSeconds = EventSeconds;
			HoldingTag = Tag;
		}

		ClockEvent.Broadcast(Tag, bHold);
	}
}

float UBendsOperationClock::GetCurrentScale() const
{
	float Scale = 1.f;
	for (const FBendsClockSpeed& Speed : Speeds)
	{
		if (Speed.From.ToSeconds() > CurrentSeconds)
		{
			break;
		}
		Scale = Speed.TimeScale;
	}
	return Scale;
}

TStatId UBendsOperationClock::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UBendsOperationClock, STATGROUP_Tickables);
}

ETickableTickType UBendsOperationClock::GetTickableTickType() const
{
	return HasAnyFlags(RF_ClassDefaultObject) ? ETickableTickType::Never : ETickableTickType::Conditional;
}


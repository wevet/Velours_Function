// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Tickable.h"
#include "GameplayTagContainer.h"
#include "BendsOperationClock.generated.h"

struct BENDSCORE_API FBendsClockTime
{
	int32 Hour = 0;
	int32 Minute = 0;
	int32 Second = 0;

	FBendsClockTime() = default;
	FBendsClockTime(int32 InHour, int32 InMinute, int32 InSecond = 0)
		: Hour(InHour), Minute(InMinute), Second(InSecond) {
	}

	double ToSeconds() const { return Hour * 3600.0 + Minute * 60.0 + Second; }
};

struct BENDSCORE_API FBendsClockEvent
{
	FBendsClockTime Time;
	FGameplayTag EventTag;

	// trueなら、この時刻で時計が止まり、ReleaseHoldまで進まない
	bool bHold = false;

	FBendsClockEvent() = default;
	FBendsClockEvent(const FBendsClockTime& InTime, const FGameplayTag& InTag, bool bInHold = false)
		: Time(InTime), EventTag(InTag), bHold(bInHold) {
	}
};

struct BENDSCORE_API FBendsClockSpeed
{
	// この時刻から次の区間まで、この速さで進む
	FBendsClockTime From;

	// 作中の秒 / 実時間の秒
	float TimeScale = 1.f;

	FBendsClockSpeed() = default;
	FBendsClockSpeed(const FBendsClockTime& InFrom, float InTimeScale)
		: From(InFrom), TimeScale(InTimeScale) {
	}
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FBendsClockEventSignature, const FGameplayTag& /*EventTag*/, bool /*bHold*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FBendsClockSecondSignature, int32 /*TotalSeconds*/);

/**
 * 
 */
UCLASS()
class BENDSCORE_API UBendsOperationClock : public UObject, public FTickableGameObject
{
	GENERATED_BODY()
	
public:
	void Setup(const TArray<FBendsClockEvent>& InEvents, const TArray<FBendsClockSpeed>& InSpeeds);

	// 指定時刻から開始する。それより前のイベントは発火済みとして飛ばす
	void StartAt(const FBendsClockTime& Time, bool bStartPaused = false);
	void Stop();
	void SetPaused(bool bInPaused) { bPaused = bInPaused; }
	void ReleaseHold(const FGameplayTag& EventTag);

	bool IsRunning() const { return bRunning; }
	bool IsPaused() const { return bPaused; }
	bool IsHolding() const { return HoldingTag.IsValid(); }
	const FGameplayTag& GetHoldingTag() const { return HoldingTag; }
	int32 GetTotalSeconds() const { return FMath::FloorToInt(CurrentSeconds); }
	FText GetDisplayText() const;

	// イベント。Holdのイベントは、受け取った時点で時計が止まっている
	FBendsClockEventSignature& OnClockEvent() { return ClockEvent; }

	// 表示の更新用。作中の1秒ごと
	FBendsClockSecondSignature& OnSecondChanged() { return SecondChanged; }

	// UObject
	virtual UWorld* GetWorld() const override;

	// FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override { return bRunning && !bPaused; }
	virtual ETickableTickType GetTickableTickType() const override;
	virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }

private:
	float GetCurrentScale() const;
	void FireDueEvents();

	FBendsClockEventSignature ClockEvent;
	FBendsClockSecondSignature SecondChanged;

	TArray<FBendsClockEvent> Events;
	TArray<FBendsClockSpeed> Speeds;

	double CurrentSeconds = 0.0;
	int32 NextEventIndex = 0;
	int32 LastWholeSecond = INDEX_NONE;
	FGameplayTag HoldingTag;
	bool bRunning = false;
	bool bPaused = false;
};


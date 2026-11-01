// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Episode/Driver/BendsPhaseDriver.h"
#include "Episode/BendsOperationClock.h"
#include "BendsPhaseDriver_Timeline.generated.h"

/**
 * 作戦の時計で進むフェーズのドライバー（EP1の14:03〜14:20など）。
 *
 * フェーズに入ると、ルールの時計（UBendsOperationClock）をStartTimeから動かす。
 * FinishTagのイベントで完了する。
 *   ホールドでないイベント … 時刻が来た時点で完了
 *   ホールドのイベント     … 時計が止まり、ResolveHoldで解かれた時点で完了
 *
 * 時計の個々のイベント（無線の雑音、偵察の報告など）は、各アクターが
 * GetClock()->OnClockEvent()を購読して処理する。このドライバーは終わりだけを見る。
 *
 * 時計はルールが持つので、フェーズが終わっても止めない（突入後も時刻は進む）。
 * 使う前に、ルールのBuildPhasesでCreateClockを呼んでおくこと。
 *
 * プレイヤーの行動でホールドを解くときは、ルールの窓口を呼ぶ:
 *   ABendsGameModeBase::GetRule(this)->ResolveHold(Clock_EP1_NorthLocked);
 *
 * 例:
 *   UBendsPhaseDriver_Timeline* Countdown = AddPhase<UBendsPhaseDriver_Timeline>(Phase_EP1_Countdown, true);
 *   Countdown->StartTime = FBendsClockTime(14, 3);
 *   Countdown->FinishTag = Clock_EP1_Breach;
 */
UCLASS()
class BENDSCORE_API UBendsPhaseDriver_Timeline : public UBendsPhaseDriver
{
	GENERATED_BODY()

public:
	// プレイヤーの行動でホールドを解く。FinishTagなら完了する
	void ResolveHold(const FGameplayTag& EventTag);

	FBendsClockTime StartTime;
	FGameplayTag FinishTag;

protected:
	virtual void OnBegin() override;
	virtual void OnEnd() override;

private:
	void HandleClockEvent(const FGameplayTag& EventTag, bool bHold);

	TWeakObjectPtr<UBendsOperationClock> Clock;
	FDelegateHandle ClockEventHandle;
};


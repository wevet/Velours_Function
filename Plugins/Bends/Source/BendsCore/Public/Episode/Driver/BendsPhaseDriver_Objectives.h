// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Episode/Driver/BendsPhaseDriver.h"
#include "BendsPhaseDriver_Objectives.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FBendsObjectiveEvent, const FGameplayTag& /*Objective*/);

/**
 * 目標のタグが通知されるのを待つドライバー。
 * 探索、装備の確認、撮影、エリアの制圧など、「何かを済ませたら進む」フェーズに使う。
 *
 * 目標の種類:
 *   RequiredObjectives … 必須。すべて済むと完了する
 *   OptionalObjectives … 任意。完了の条件に含めない。
 *                        済むたびにOptionalCountFlagを+1する（例: EP2の疑念の種）
 *
 * 通知の仕方:
 *   アクターはルールの窓口を呼ぶ。今のフェーズが対象外なら何も起きないので、
 *   トリガーを置きっぱなしにしても誤作動しない。
 *     ABendsGameModeBase::GetRule(this)->NotifyObjective(Objective_EP2_AwardWall);
 *
 * 必須がそろっても演出を挟みたい場合は、bFinishWhenRequiredDoneをfalseにして、
 * 演出の終わりにFinishを呼ぶ。
 *
 * フェーズに入るたびに達成状況はリセットされる（チェックポイントからのやり直しに対応）。
 *
 * 例:
 *   UBendsPhaseDriver_Objectives* Corridor = AddPhase<UBendsPhaseDriver_Objectives>(Phase_EP2_Corridor);
 *   Corridor->RequiredObjectives = { Objective_EP2_ReachRoom };
 *   Corridor->OptionalObjectives = { Objective_EP2_AwardWall };
 *   Corridor->OptionalCountFlag = Flag_Doubt;
 */
UCLASS()
class BENDSCORE_API UBendsPhaseDriver_Objectives : public UBendsPhaseDriver
{
	GENERATED_BODY()

public:
	// 対象の目標なら記録してtrueを返す。対象外、済んだもの、フェーズ外はfalse
	bool NotifyObjective(const FGameplayTag& Objective);

	bool IsObjectiveDone(const FGameplayTag& Objective) const { return Completed.Contains(Objective); }
	bool AreRequiredDone() const;

	// 目標が済んだとき（UIや演出用）
	FBendsObjectiveEvent& OnObjectiveDone() { return ObjectiveDoneEvent; }

	TArray<FGameplayTag> RequiredObjectives;
	TArray<FGameplayTag> OptionalObjectives;

	// 任意の目標が済むたびに+1するフラグ（空なら加算しない）
	FGameplayTag OptionalCountFlag;

	// falseなら、必須がそろってもFinishは外から呼ぶ
	bool bFinishWhenRequiredDone = true;

protected:
	virtual void OnBegin() override;

private:
	TSet<FGameplayTag> Completed;
	FBendsObjectiveEvent ObjectiveDoneEvent;
};


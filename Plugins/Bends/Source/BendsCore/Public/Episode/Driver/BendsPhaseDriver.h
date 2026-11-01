// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "BendsPhaseDriver.generated.h"

class UBendsEpisodeRule;

/**
 * 1つのフェーズの「終わり方」を決める部品の基底。
 *
 * ルール（UBendsEpisodeRule）は、フェーズの並びとチェックポイントだけを持ち、
 * 「何が起きたらそのフェーズが終わるか」はドライバーに任せる。
 * これにより、時計で進むフェーズ、会話で進むフェーズ、探索で進むフェーズを
 * 1つのEPの中で自由に組み合わせられる。
 *
 * ライフサイクル:
 *   フェーズに入る → Begin → OnBegin
 *   毎フレーム     → Tick  → OnTick（bTickEnabledのときだけ）
 *   フェーズを出る → End   → OnEnd
 *
 * 結果の伝え方:
 *   Finish() … 完了。次のフェーズへ（NextPhaseを指定すると分岐）
 *   Fail()   … 失敗。ルールのOnPhaseFailedへ（既定はチェックポイントからやり直し）
 *   結果は1回のBeginにつき1回だけ受け付ける。
 *
 * このクラス自体は条件を持たず、外からFinishが呼ばれるまで待つ。
 * カットシーンの終了や、スクリプトの合図で進めるフェーズにそのまま使える。
 *
 * 生成はルールのBuildPhasesでAddPhase<T>を使う。Outerはルールになる。
 *   AddPhase<UBendsPhaseDriver>(Phase_Prologue_Envelope);
 */
UCLASS()
class BENDSCORE_API UBendsPhaseDriver : public UObject
{
	GENERATED_BODY()

public:
	// ルールから呼ぶ
	void Begin();
	void End();
	void Tick(float DeltaTime);

	// 完了。NextPhaseを指定すると分岐する。空なら並び順の次へ（最後ならEP完了）
	void Finish(const FGameplayTag& NextPhase = FGameplayTag());

	// 失敗。ルールのOnPhaseFailedに渡る
	void Fail();

	bool IsActive() const { return bActive; }
	bool WantsTick() const { return bActive && bTickEnabled; }

	virtual UWorld* GetWorld() const override;

protected:
	// 派生クラスで上書きする
	virtual void OnBegin() {}
	virtual void OnEnd() {}
	virtual void OnTick(float DeltaTime) {}

	UBendsEpisodeRule* GetRule() const;

	// OnTickを呼ぶか。派生クラスのコンストラクタで設定する
	bool bTickEnabled = false;

private:
	bool bActive = false;
	bool bResultSent = false;
};

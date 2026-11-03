// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Ability/WvGameplayAbility.h"
#include "Ability/HitReact/VHitReactTypes.h"
#include "WvAbility_HitReactBase.generated.h"

class UAnimMontage;
class UChooserTable;
class UWvAT_PlayMontageAndWaitForEvent;

/**
 * 被弾の反応のアビリティの基底（Repel、Death）。
 * 被弾のイベントから条件（FVHitReactQuery）を作り、キャラクターのHitReactChooserでMontageを選んで再生する。
 * 派生クラスは、死亡かどうかと、再生の前後の処理だけを書く。
 */
UCLASS(Abstract)
class VELOURS_API UWvAbility_HitReactBase : public UWvGameplayAbility
{
	GENERATED_BODY()

public:
	UWvAbility_HitReactBase();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// 条件を作った後、Chooserで選ぶ前に呼ばれる。派生クラスで条件を上書きできる
	virtual void ModifyQuery(FVHitReactQuery& Query) const {}

	// Montageの再生を始めたとき
	virtual void OnMontageStarted(ABasePawn& Character, UAnimMontage& Montage) {}

	// Montageが終わった、または中断されたとき（この後アビリティを終える）
	virtual void OnMontageFinished(bool bInterrupted) {}

	// 基底のアビリティが止まったら、Montageも止めるか（死亡はfalse）
	UPROPERTY(EditDefaultsOnly, Category = "HitReact")
	bool bStopWhenAbilityEnds = true;

	UPROPERTY(EditDefaultsOnly, Category = "HitReact|Debug")
	float DrawDebugTime = 2.f;

private:
	void PlayMontage(UAnimMontage* Montage);
	void DrawDebug(const ABasePawn& Character, const FGameplayEventData& EventData, const FVHitReactQuery& Query, const UAnimMontage* Montage) const;

	UFUNCTION()
	void OnPlayMontageCompleted_Event(FGameplayTag EventTag, FGameplayEventData EventData);

	UFUNCTION()
	void OnPlayMontageInterrupted_Event(FGameplayTag EventTag, FGameplayEventData EventData);

	UPROPERTY()
	TObjectPtr<UWvAT_PlayMontageAndWaitForEvent> MontageTask{ nullptr };
};


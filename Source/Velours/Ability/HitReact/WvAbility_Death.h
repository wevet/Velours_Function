// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Ability/HitReact/WvAbility_HitReactBase.h"
#include "WvAbility_Death.generated.h"

/**
 * 死亡の反応。PassiveAbilityTrigger.KillReactのイベントで起動する。
 * FeatureをConfig.HitReactFeature.Deadにして、Chooserで死亡のMontageを選ぶ。
 * Montageはアビリティが終わっても止めない（最後の姿勢を保つ）。
 */
UCLASS()
class VELOURS_API UWvAbility_Death : public UWvAbility_HitReactBase
{
	GENERATED_BODY()

public:
	UWvAbility_Death();

protected:
	virtual void ModifyQuery(FVHitReactQuery& Query) const override;
	virtual void OnMontageStarted(ABasePawn& Character, UAnimMontage& Montage) override;

	// Montageの終わりから、この秒数だけ早くラグドールに移る
	UPROPERTY(EditDefaultsOnly, Category = "HitReact")
	float MontageTrimTime = 0.2f;
};


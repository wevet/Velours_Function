// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Ability/HitReact/WvAbility_HitReactBase.h"
#include "WvAbility_Repel.generated.h"

/**
 * 被弾の反応。PassiveAbilityTrigger.HitReactのイベントで起動する。
 * Featureは通常Config.HitReactFeature.Hit（弱点や特殊なFeatureがあればそちら）。
 */
UCLASS()
class VELOURS_API UWvAbility_Repel : public UWvAbility_HitReactBase
{
	GENERATED_BODY()

public:
	UWvAbility_Repel();
};

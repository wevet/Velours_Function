// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Game/BendsViewMode.h"
#include "VInteractionAnimQuery.generated.h"


USTRUCT(BlueprintType)
struct VELOURS_API FVInteractionAnimQuery
{
	GENERATED_BODY()

	// どういう動作か（Bends.Interaction.Anim.*）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FGameplayTag AnimationTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	EBendsViewMode ViewMode = EBendsViewMode::FirstPerson;

	// 対象の高さ。キャラクターのアクターの位置からの差（cm）。床、腰、目の高さの選び分けに使う
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	float HeightOffset = 0.f;
};


// Copyright 2022 wevet works All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Game/BendsViewMode.h"
#include "BendsInteractionTypes.generated.h"

class APawn;
class UBendsInteractableComponent;


/** 押すか、長押しか */
UENUM()
enum class EBendsInteractionMode : uint8
{
	Press,
	Hold,
};


/** 動作中の視点の扱い */
UENUM()
enum class EBendsLookLock : uint8
{
	UseGlobal,  // CVar（bends.Interaction.LookLock）に従う
	Free,       // 止めない
	Lock,       // 止める
	Limit,      // 動作開始時の向きから一定の角度に制限する
};


/** フラグの条件。Flagの値がMinValue以上なら満たす */
USTRUCT()
struct FBendsInteractionFlagCondition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Condition", meta = (Categories = "Bends.Flag"))
	FGameplayTag Flag;

	UPROPERTY(EditAnywhere, Category = "Condition")
	int32 MinValue = 1;
};


/** 視点ごとの探し方 */
USTRUCT()
struct FBendsInteractionScanSettings
{
	GENERATED_BODY()

	// 対象までの最大距離（cm）。FPSはカメラから、TPSはキャラクターから測る
	UPROPERTY(EditAnywhere, Category = "Scan", meta = (ClampMin = 0.0))
	float MaxDistance = 180.f;

	// 視線の方向からの許容角度（度）
	UPROPERTY(EditAnywhere, Category = "Scan", meta = (ClampMin = 0.0, ClampMax = 90.0))
	float MaxViewAngle = 10.f;

	// 視線の先に候補がないとき、キャラクターの近くから選ぶ半径（cm）。0で無効
	UPROPERTY(EditAnywhere, Category = "Scan", meta = (ClampMin = 0.0))
	float ProximityRadius = 0.f;

	// 近くから選ぶときの、キャラクターの正面からの許容角度（度）
	UPROPERTY(EditAnywhere, Category = "Scan", meta = (ClampMin = 0.0, ClampMax = 180.0))
	float ProximityAngle = 70.f;
};


/** インタラクトを実行したときの情報。Effectに渡す */
struct FBendsInteractionContext
{
	APawn* Instigator = nullptr;
	UBendsInteractableComponent* Interactable = nullptr;
	AActor* Target = nullptr;  // Interactableのオーナー
	EBendsViewMode ViewMode = EBendsViewMode::FirstPerson;
};


// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "WvAbilitySystemTypes.h"
#include "Character/CharacterSystemTypes.h"
#include "VHitReactTypes.generated.h"

class USkeletalMeshComponent;

/** 攻撃の種類 */
UENUM(BlueprintType)
enum class EVHitSource : uint8
{
	Melee,
	Bullet,
	Explosion,
	Other,
};

/**
 * 被弾の反応のMontageを選ぶChooser Tableの条件。
 * Chooser TableのContextにこの構造体を指定し、列から各プロパティを参照する。
 * 行は上から評価されるので、条件の細かい行を上に、予備（方向を問わない等）を下に置く。
 */
USTRUCT(BlueprintType)
struct VELOURS_API FVHitReactQuery
{
	GENERATED_BODY()

	// Config.HitReactFeature.Hit / Dead / 弱点などの特殊なFeature
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitReact")
	FGameplayTag Feature;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitReact")
	EVHitSource Source = EVHitSource::Melee;

	// 攻撃側の武器
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitReact")
	EAttackWeaponState AttackerWeapon = EAttackWeaponState::EmptyWeapon;

	// 被弾した側の武器（構えによって反応を変える）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitReact")
	EAttackWeaponState VictimWeapon = EAttackWeaponState::EmptyWeapon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitReact")
	EHitReactDirection Direction = EHitReactDirection::Forward;

	// 銃弾は当たった骨、メレーは攻撃の方向から決める
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitReact")
	EHitVerticalDirection Vertical = EHitVerticalDirection::Middle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitReact")
	bool bIsCrouched = false;
};

/** 
* 銃弾の上中下を、当たった骨で決めるための設定。キャラクターの骨格ごとに設定する 
*/
USTRUCT(BlueprintType)
struct VELOURS_API FVHitReactBoneZones
{
	GENERATED_BODY()

	// この骨と、その子の骨に当たったら上（頭、首）
	UPROPERTY(EditAnywhere, Category = "HitReact")
	FName HeadRootBone = TEXT("neck_01");

	// これらの骨と、その子の骨に当たったら下（脚）
	UPROPERTY(EditAnywhere, Category = "HitReact")
	TArray<FName> LegRootBones = { TEXT("thigh_l"), TEXT("thigh_r") };
};

namespace VHitReact
{
	// 攻撃の情報（ターゲットデータ）から攻撃の種類を決める。Noneならfalse
	VELOURS_API bool TryGetHitSource(EWvBattleDamageAttackSourceType SourceType, EVHitSource& OutSource);

	// 攻撃側の武器から攻撃の種類を決める（ターゲットデータがない場合の予備）
	VELOURS_API EVHitSource ToHitSource(EAttackWeaponState Weapon);

	// 当たった骨から上中下を決める。骨が分からなければfalse
	VELOURS_API bool TryGetVerticalFromBone(
		const USkeletalMeshComponent* Mesh, 
		const FName& BoneName,
		const FVHitReactBoneZones& Zones, 
		EHitVerticalDirection& OutVertical);
}


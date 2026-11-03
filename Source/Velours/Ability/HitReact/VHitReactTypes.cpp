// Copyright 2022 wevet works All Rights Reserved.

#include "Ability/HitReact/VHitReactTypes.h"
#include "Components/SkeletalMeshComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(VHitReactTypes)

namespace VHitReact
{
	bool TryGetHitSource(EWvBattleDamageAttackSourceType SourceType, EVHitSource& OutSource)
	{
		switch (SourceType)
		{
		case EWvBattleDamageAttackSourceType::BasicMelee:
			OutSource = EVHitSource::Melee;
			return true;
		case EWvBattleDamageAttackSourceType::Bullet:
			OutSource = EVHitSource::Bullet;
			return true;
		case EWvBattleDamageAttackSourceType::Bomb:
			OutSource = EVHitSource::Explosion;
			return true;
		default:
			return false;
		}
	}

	EVHitSource ToHitSource(EAttackWeaponState Weapon)
	{
		switch (Weapon)
		{
		case EAttackWeaponState::EmptyWeapon:
		case EAttackWeaponState::Knife:
			return EVHitSource::Melee;
		case EAttackWeaponState::Gun:
		case EAttackWeaponState::Rifle:
		case EAttackWeaponState::SniperRifle:
		case EAttackWeaponState::ShotGun:
			return EVHitSource::Bullet;
		default:
			return EVHitSource::Other;
		}
	}

	bool TryGetVerticalFromBone(
		const USkeletalMeshComponent* Mesh,
		const FName& BoneName, 
		const FVHitReactBoneZones& Zones, 
		EHitVerticalDirection& OutVertical)
	{
		if (!Mesh || BoneName.IsNone() || Mesh->GetBoneIndex(BoneName) == INDEX_NONE)
		{
			return false;
		}

		if (BoneName == Zones.HeadRootBone || Mesh->BoneIsChildOf(BoneName, Zones.HeadRootBone))
		{
			OutVertical = EHitVerticalDirection::Top;
			return true;
		}

		for (const FName& LegBone : Zones.LegRootBones)
		{
			if (BoneName == LegBone || Mesh->BoneIsChildOf(BoneName, LegBone))
			{
				OutVertical = EHitVerticalDirection::Bottom;
				return true;
			}
		}

		OutVertical = EHitVerticalDirection::Middle;
		return true;
	}
}


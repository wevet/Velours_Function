// Copyright 2022 wevet works All Rights Reserved.

#include "Character/VMetaHumanVisualActor.h"


#include "Components/SkeletalMeshComponent.h"
#include "Components/PrimitiveComponent.h"

#include "GroomComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogVMetaHuman, Log, All);

#include UE_INLINE_GENERATED_CPP_BY_NAME(VMetaHumanVisualActor)

AVMetaHumanVisualActor::AVMetaHumanVisualActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AVMetaHumanVisualActor::BeginPlay()
{
	Super::BeginPlay();
	CacheComponents();
	ApplyHeadVisibility();
}

void AVMetaHumanVisualActor::CacheComponents()
{
	BodyMesh = nullptr;
	HeadPrimitives.Reset();

	TArray<UPrimitiveComponent*> Primitives;
	GetComponents<UPrimitiveComponent>(Primitives);

	for (UPrimitiveComponent* Primitive : Primitives)
	{
		const FName Name = Primitive->GetFName();

		if (Name == BodyComponentName)
		{
			BodyMesh = Cast<USkeletalMeshComponent>(Primitive);
		}
		else if (Name == HeadComponentName)
		{
			// 頭と、その子（髪、眉、まつ毛、ひげなど）
			HeadPrimitives.AddUnique(Primitive);

			TArray<USceneComponent*> HeadChildren;
			Primitive->GetChildrenComponents(true, HeadChildren);
			for (USceneComponent* Child : HeadChildren)
			{
				if (UPrimitiveComponent* ChildPrimitive = Cast<UPrimitiveComponent>(Child))
				{
					HeadPrimitives.AddUnique(ChildPrimitive);
				}
			}
		}

		if (!ExtraHiddenTag.IsNone() && Primitive->ComponentHasTag(ExtraHiddenTag))
		{
			HeadPrimitives.AddUnique(Primitive);
		}
	}

	if (!BodyMesh)
	{
		UE_LOG(LogVMetaHuman, Warning, TEXT("[%s] %s: body component '%s' not found."), 
			*FString(__FUNCTION__), *GetName(), *BodyComponentName.ToString());
	}
	if (HeadPrimitives.IsEmpty())
	{
		UE_LOG(LogVMetaHuman, Warning, TEXT("[%s] %s: head component '%s' not found."),
			*FString(__FUNCTION__), *GetName(), *HeadComponentName.ToString());
	}
}

void AVMetaHumanVisualActor::SetFirstPersonView(bool bIsFirstPerson)
{
	bFirstPersonView = bIsFirstPerson;

	// BeginPlayより前に呼ばれた場合は、BeginPlayで反映される
	if (HasActorBegunPlay())
	{
		ApplyHeadVisibility();
	}

}

void AVMetaHumanVisualActor::ApplyHeadVisibility()
{
	UE_LOG(LogVMetaHuman, Log, TEXT("[%s] FirstPerson=%d Owner=%s"), *FString(__FUNCTION__),
		bFirstPersonView ? 1 : 0, *GetNameSafe(GetOwner()));

	const EFirstPersonPrimitiveType HeadType = bFirstPersonView
		? EFirstPersonPrimitiveType::WorldSpaceRepresentation
		: EFirstPersonPrimitiveType::None;

	for (UPrimitiveComponent* Primitive : HeadPrimitives)
	{
		if (!Primitive)
		{
			continue;
		}


		if (UGroomComponent* Groom = Cast<UGroomComponent>(Primitive))
		{
			// Groomは一人称の描画の設定が効かないので、非表示にして影だけ残す
			Groom->SetVisibility(!bFirstPersonView);
			Groom->SetCastHiddenShadow(bFirstPersonView);
		}
		else if (USkeletalMeshComponent* Mesh = Cast<USkeletalMeshComponent>(Primitive))
		{
			// メッシュは一人称の視点からだけ外す（影と反射には残る）
			Mesh->SetFirstPersonPrimitiveType(HeadType);
			Mesh->SetCastHiddenShadow(bFirstPersonView);
		}

		const FName Name = Primitive->GetFName();
		UE_LOG(LogVMetaHuman, Log, TEXT("[%s] PrimitiveName : %s"), *FString(__FUNCTION__), *Name.ToString());

	}
}

bool AVMetaHumanVisualActor::GetFirstPersonCameraAttachment(USceneComponent*& OutParent, FName& OutSocket) const
{
	if (!BodyMesh || !BodyMesh->DoesSocketExist(CameraSocketName))
	{
		return false;
	}
	OutParent = BodyMesh;
	OutSocket = CameraSocketName;
	return true;
}


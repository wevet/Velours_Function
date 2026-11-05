// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Character/VFirstPersonVisual.h"
#include "VMetaHumanVisualActor.generated.h"

class USkeletalMeshComponent;
class UPrimitiveComponent;

/**
 * MetaHumanの見た目の子アクターの基底。MetaHumanのBP（BP_Kellanなど）の親クラスにする。
 * 一人称のとき、頭（Face）とその子（髪、眉、まつ毛、ひげ）を一人称の視点から消し、
 * 影と反射には残す（FirstPersonPrimitiveType = WorldSpaceRepresentation）。
 * カメラは、体（Body）の骨格に作ったソケットに付けてもらう。
 * 部位はBeginPlayで名前から探す。名前はMetaHumanの標準（Body、Face）を既定にしている。
 */
UCLASS()
class VELOURS_API AVMetaHumanVisualActor : public AActor, public IVFirstPersonVisual
{
	GENERATED_BODY()

public:
	AVMetaHumanVisualActor();

	// IVFirstPersonVisual
	virtual void SetFirstPersonView(bool bIsFirstPerson) override;
	virtual bool GetFirstPersonCameraAttachment(USceneComponent*& OutParent, FName& OutSocket) const override;

	bool IsFirstPersonView() const { return bFirstPersonView; }

protected:
	virtual void BeginPlay() override;

	// 部位のコンポーネントの名前
	UPROPERTY(EditDefaultsOnly, Category = "FirstPerson")
	FName BodyComponentName = TEXT("Body");

	UPROPERTY(EditDefaultsOnly, Category = "FirstPerson")
	FName HeadComponentName = TEXT("Face");

	// 体の骨格に作るカメラのソケット（headの子、目より少し前）
	UPROPERTY(EditDefaultsOnly, Category = "FirstPerson")
	FName CameraSocketName = TEXT("FPCamera");

	// 頭と一緒に消したいもの（帽子など）に付けるタグ。頭の子でなくても消える
	UPROPERTY(EditDefaultsOnly, Category = "FirstPerson")
	FName ExtraHiddenTag = TEXT("FPHidden");

private:
	void CacheComponents();
	void ApplyHeadVisibility();

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> BodyMesh;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPrimitiveComponent>> HeadPrimitives;

	bool bFirstPersonView = false;
};


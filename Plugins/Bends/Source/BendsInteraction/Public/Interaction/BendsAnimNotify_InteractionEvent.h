// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "BendsAnimNotify_InteractionEvent.generated.h"

/**
 * インタラクトのMontageに置き、インタラクターへイベントを送るAnimNotify。
 * 通常は「手が触れた瞬間」にContactを置く（ここで物が消え、結果が出る）。
 * FinishedとCancelledは、アビリティがMontageの終了と中断で送る。
 */
UCLASS(meta = (DisplayName = "Bends Interaction Event"))
class BENDSINTERACTION_API UBendsAnimNotify_InteractionEvent : public UAnimNotify
{
	GENERATED_BODY()

public:
	UBendsAnimNotify_InteractionEvent();

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;

	UPROPERTY(EditAnywhere, Category = "Bends", meta = (Categories = "Bends.Interaction.Event"))
	FGameplayTag EventTag;
};


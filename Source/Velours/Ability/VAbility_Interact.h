// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Ability/WvGameplayAbility.h"
#include "Task/WvAT_PlayMontageAndWaitForEvent.h"
#include "Game/BendsViewMode.h"
#include "VAbility_Interact.generated.h"

class UBendsInteractableComponent;
class UChooserTable;
class UAnimMontage;

/**
 * インタラクトのMontageを再生するアビリティ。
 * インタラクターが送るGameplay Event（AbilityTriggers）で起動する。
 *   1. Payload.OptionalObjectからインタラクト対象を取り、EventMagnitudeで視点を判断する
 *   2. 視点に合ったMontageを取り、Motion Warpingの目標を対象の位置にする
 *   3. Montageを再生する（手が触れる瞬間のContactは、Montage内のAnimNotifyが送る）
 *   4. 終わったらFinished、中断されたらCancelledを送り返す
 */
UCLASS()
class VELOURS_API UVAbility_Interact : public UWvGameplayAbility
{
	GENERATED_BODY()
	
public:
	UVAbility_Interact();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle, 
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, 
		const FGameplayEventData* TriggerEventData) override;

	UPROPERTY(EditDefaultsOnly, Category = "Interact")
	TObjectPtr<UChooserTable> MontageChooser;

private:
	UAnimMontage* ChooseMontage(const UBendsInteractableComponent& Target, EBendsViewMode ViewMode) const;
	void FinishWith(const FGameplayTag& EventTag, bool bWasCancelled);

	UFUNCTION()
	void OnPlayMontageCompleted_Event(FGameplayTag EventTag, FGameplayEventData EventData);

	UFUNCTION()
	void OnPlayMontageInterrupted_Event(FGameplayTag EventTag, FGameplayEventData EventData);

	TWeakObjectPtr<const UBendsInteractableComponent> Interactable;

	UPROPERTY()
	TObjectPtr<class UWvAT_PlayMontageAndWaitForEvent> MontageTask{ nullptr };
};

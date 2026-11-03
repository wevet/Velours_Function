// Copyright 2022 wevet works All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "GameplayTagContainer.h"
#include "Interaction/BendsInteractionTypes.h"
#include "BendsInteractableComponent.generated.h"

class UAnimMontage;
class UBendsInteractionEffect;

DECLARE_MULTICAST_DELEGATE_TwoParams(FBendsInteractedEvent, UBendsInteractableComponent* /*Interactable*/, const FBendsInteractionContext& /*Context*/);

/**
 * インタラクトできる物に付けるコンポーネント（拾う物、調べる物、扉など）。
 *
 * コンポーネントの位置が、狙う点、印を出す点、手を伸ばす先（Motion Warpingの目標）になる。
 * 物理の当たり判定は使わず、UBendsInteractorComponentが登録済みの対象から視線で選ぶ。
 *
 * 結果はEffectsに部品で設定する（目標の通知、フラグ、トリガー、ホールドの解除など）。
 * ゲーム側の個別の処理（扉を開ける等）はOnInteractedで受け取る。
 *
 * アニメーション:
 *   AbilityEventTagを設定すると、ポーンのASCにGameplay Eventを送り、アビリティがMontageを再生する。
 *   Payload.OptionalObjectがこのコンポーネント、EventMagnitudeが視点（FPS=0、TPS=1）。
 *   空なら即座に結果を出す（グレーボックス用）。
 *
 * 1回きり（bOneShot）の対象は、InteractionIdをフラグとして進行状況に記録する。
 * チェックポイントからやり直すと、その時点の状態に戻る。
 *
 * 使える条件:
 *   RequiredPhases … 今のフェーズがいずれかに一致する（親のタグでEP全体も指定できる）
 *   RequiredFlags  … すべてのフラグが条件を満たす
 */
UCLASS(ClassGroup = (Bends), meta = (BlueprintSpawnableComponent))
class BENDSINTERACTION_API UBendsInteractableComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UBendsInteractableComponent();

	// 条件を満たし、今インタラクトできるか
	bool CanInteract(const APawn* Instigator) const;

	// インタラクターから呼ぶ。使用済みにして、Effectを実行する
	void Interact(const FBendsInteractionContext& Context);

	void SetInteractionEnabled(bool bEnabled) { bInteractionEnabled = bEnabled; }
	bool IsInteractionEnabled() const { return bInteractionEnabled; }
	bool IsUsed() const { return bUsed; }

	FVector GetPromptLocation() const { return GetComponentLocation() + PromptOffset; }
	const TSoftObjectPtr<UAnimMontage>& GetMontage(EBendsViewMode ViewMode) const;

	FBendsInteractedEvent& OnInteracted() { return InteractedEvent; }

#pragma region Interaction
	// ID。1回きりの対象では必須（使用済みの記録に使う）
	UPROPERTY(EditAnywhere, Category = "Interaction", meta = (Categories = "Bends.Interaction.Id"))
	FGameplayTag InteractionId;

	// 印の横に出す名詞（例: 薬莢、携帯）
	UPROPERTY(EditAnywhere, Category = "Interaction")
	FText DisplayName;

	UPROPERTY(EditAnywhere, Category = "Interaction")
	EBendsInteractionMode Mode = EBendsInteractionMode::Press;

	UPROPERTY(EditAnywhere, Category = "Interaction", meta = (ClampMin = 0.1, EditCondition = "Mode == EBendsInteractionMode::Hold", EditConditionHides))
	float HoldDuration = 1.f;

	// 0ならインタラクターの設定を使う（cm）
	UPROPERTY(EditAnywhere, Category = "Interaction", meta = (ClampMin = 0.0))
	float MaxDistance = 0.f;

	// 印を出す位置のずれ（ワールド空間）
	UPROPERTY(EditAnywhere, Category = "Interaction")
	FVector PromptOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Interaction")
	bool bOneShot = true;

	// 使用済みになったら、オーナーを非表示にして当たり判定を切る（拾う物）
	UPROPERTY(EditAnywhere, Category = "Interaction")
	bool bHideOwnerWhenUsed = false;
#pragma endregion

#pragma region Condition
	UPROPERTY(EditAnywhere, Category = "Condition", meta = (Categories = "Bends.Phase"))
	FGameplayTagContainer RequiredPhases;

	UPROPERTY(EditAnywhere, Category = "Condition")
	TArray<FBendsInteractionFlagCondition> RequiredFlags;
#pragma endregion

#pragma region Animation
	// アビリティを起動するGameplay Eventのタグ（例: Ability.Interact.Pickup）。空なら即座に結果を出す
	UPROPERTY(EditAnywhere, Category = "Animation")
	FGameplayTag AbilityEventTag;

	UPROPERTY(EditAnywhere, Category = "Animation")
	TSoftObjectPtr<UAnimMontage> FirstPersonMontage;

	UPROPERTY(EditAnywhere, Category = "Animation")
	TSoftObjectPtr<UAnimMontage> ThirdPersonMontage;

	// Motion Warpingの目標の名前。位置はこのコンポーネント
	UPROPERTY(EditAnywhere, Category = "Animation")
	FName WarpTargetName = TEXT("Interact");
#pragma endregion

#pragma region Input
	UPROPERTY(EditAnywhere, Category = "Input")
	bool bLockMovement = true;

	UPROPERTY(EditAnywhere, Category = "Input")
	EBendsLookLock LookLock = EBendsLookLock::UseGlobal;
#pragma endregion

	UPROPERTY(EditAnywhere, Instanced, Category = "Result")
	TArray<TObjectPtr<UBendsInteractionEffect>> Effects;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void MarkUsed();
	void ApplyUsedState();

	FBendsInteractedEvent InteractedEvent;
	bool bInteractionEnabled = true;
	bool bUsed = false;
};


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
 * A component attached to interactive objects (items to pick up, items to inspect, doors, etc.).
 *
 * The component’s position serves as the target point, the point where a marker appears, and the destination of a reach (the target for Motion Warping).
 * Instead of using physics collision detection, the player selects a target by looking at objects with the UBendsInteractorComponent registered.
 *
 * Results are configured via components in Effects (target notification, flags, triggers, releasing a hold, etc.).
 * Game-side specific processing (such as opening a door) is handled via *OnInteracted*.
 *
 * Animation:
 *   Setting *AbilityEventTag* sends a Gameplay Event to the Pawn’s ASC, causing the ability to play a Montage.
 *   `Payload.OptionalObject` is this component, and `EventMagnitude` is the viewpoint (FPS=0, TPS=1).
 *   If empty, the result is returned immediately (for graybox use).
 *
 * For one-time (bOneShot) events, the InteractionId is recorded as a flag to track progress.
 * Restarting from a checkpoint returns the player to the state at that point.
 *
 * Usage Conditions:
 *   RequiredPhases … The current phase matches any of the specified phases (the entire EP can also be specified via the parent tag)
 *   RequiredFlags  … All flags meet the specified conditions
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
	// アビリティを起動するGameplay Eventのタグ（例: Ability.Interact）。空なら即座に結果を出す
	UPROPERTY(EditAnywhere, Category = "Animation")
	FGameplayTag AbilityEventTag;

	// どういう動作か（例: Bends.Interaction.Anim.Pickup）。アビリティがChooserでMontageを選ぶのに使う
	UPROPERTY(EditAnywhere, Category = "Animation", meta = (Categories = "Bends.Interaction.Anim"))
	FGameplayTag AnimationTag;

	// この物だけの特別な動作（薬莢の0.5秒の溜めなど）。設定するとChooserより優先する
	UPROPERTY(EditAnywhere, Category = "Animation")
	TSoftObjectPtr<UAnimMontage> MontageOverride;

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


// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Interaction/BendsInteractionTypes.h"
#include "BendsInteractorComponent.generated.h"


class APawn;
class APlayerController;
class APlayerCameraManager;
class UAbilitySystemComponent;
class UBendsInteractableComponent;
class UBendsInteractionPromptWidget;
struct FGameplayEventData;

DECLARE_MULTICAST_DELEGATE_TwoParams(FBendsInteractionFocusChanged, UBendsInteractableComponent* /*Old*/, UBendsInteractableComponent* /*New*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FBendsInteractionStateEvent, UBendsInteractableComponent* /*Interactable*/);

/**
 * プレイヤーのポーンに付けて、インタラクトを行うコンポーネント。
 *
 * 候補の選び方（ScanIntervalごと）:
 *   登録済みの対象から、視線の方向に一番近い物を選ぶ。遮蔽物があれば除く。
 *   FPS … 距離も遮蔽もカメラから測る
 *   TPS … 狙いの方向はカメラ、距離と遮蔽はキャラクターから測る。
 *         視線の先になければ、キャラクターの前方の近くから選ぶ（ProximityRadius）
 *   視点はIBendsViewModeProviderから取る。
 *
 * 入力:
 *   ポーンのインタラクトのアクションで、BeginInput（押下）とEndInput（解放）を呼ぶ。
 *   押す対象は押した瞬間、長押しの対象はHoldDurationに達した時点で実行する。
 *
 * 実行の流れ:
 *   1. 移動を止める（対象の設定）。視点はCVarか対象の設定で、止める/制限する/止めない
 *   2. 対象にAbilityEventTagがあれば、ポーンのASCにGameplay Eventを送る
 *   3. アビリティから送り返されるイベントを待つ
 *        Bends.Interaction.Event.Contact   … 結果を出す（対象のEffectを実行）
 *        Bends.Interaction.Event.Finished  … 終わる（Contactがまだなら、ここで結果を出す）
 *        Bends.Interaction.Event.Cancelled … 結果を出さずに終わる
 *   4. 入力の固定を戻す
 *   アビリティがない、ASCがない、BusyTimeoutを超えた場合は、警告を出して進める。
 *
 * 一時的な禁止は、理由のタグで足し引きする（Bends.Interaction.Block.*）。
 *
 * CVar:
 *   bends.Interaction.LookLock 0/1/2 … 動作中の視点（止めない/止める/範囲を制限）
 *   bends.Interaction.Debug    0/1   … 候補のデバッグ表示
 */
UCLASS(ClassGroup = (Bends), meta = (BlueprintSpawnableComponent))
class BENDSINTERACTION_API UBendsInteractorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBendsInteractorComponent();

	// 入力
	void BeginInput();
	void EndInput();

	// 一時的な禁止
	void AddBlockReason(const FGameplayTag& Reason);
	void RemoveBlockReason(const FGameplayTag& Reason);
	bool IsBlocked() const { return !BlockReasons.IsEmpty(); }

	UBendsInteractableComponent* GetFocused() const { return Focused.Get(); }
	UBendsInteractableComponent* GetActive() const { return Active.Get(); }
	float GetHoldProgress() const;
	bool IsBusy() const { return bBusy; }
	EBendsViewMode GetViewMode() const;

	FBendsInteractionFocusChanged& OnFocusChanged() { return FocusChangedEvent; }
	FBendsInteractionStateEvent& OnInteractionStarted() { return StartedEvent; }
	FBendsInteractionStateEvent& OnInteractionFinished() { return FinishedEvent; }

	// アビリティやAnimNotifyから、インタラクターへイベントを送り返す
	static void SendInteractionEvent(AActor* Avatar, const FGameplayTag& EventTag, const UObject* Interactable = nullptr);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, Category = "Interaction|Scan")
	FBendsInteractionScanSettings FirstPersonScan;

	UPROPERTY(EditAnywhere, Category = "Interaction|Scan")
	FBendsInteractionScanSettings ThirdPersonScan;

	UPROPERTY(EditAnywhere, Category = "Interaction|Scan", meta = (ClampMin = 0.0))
	float ScanInterval = 0.05f;

	UPROPERTY(EditAnywhere, Category = "Interaction|Scan")
	TEnumAsByte<ECollisionChannel> OcclusionChannel = ECC_Visibility;

	// 視点を制限するとき（Limit）の、動作開始時の向きからの角度（度）
	UPROPERTY(EditAnywhere, Category = "Interaction|Input", meta = (ClampMin = 0.0))
	float LookLimitYaw = 25.f;

	UPROPERTY(EditAnywhere, Category = "Interaction|Input", meta = (ClampMin = 0.0))
	float LookLimitPitch = 15.f;

	// アビリティから終わりのイベントが来ない場合に、強制的に終える時間（秒）
	UPROPERTY(EditAnywhere, Category = "Interaction|Input", meta = (ClampMin = 1.0))
	float BusyTimeout = 10.f;

	UPROPERTY(EditAnywhere, Category = "Interaction|UI")
	TSubclassOf<UBendsInteractionPromptWidget> PromptWidgetClass;

private:
	APawn* GetPawn() const;
	APlayerController* GetPlayerController() const;
	bool IsLocalPlayerPawn() const;

	void Scan();
	bool HasLineOfSight(const FVector& From, const UBendsInteractableComponent* Interactable, const APawn* Pawn) const;
	void SetFocus(UBendsInteractableComponent* NewFocus);

	void StartInteraction(UBendsInteractableComponent* Interactable);
	void ApplyContact();
	void FinishInteraction(bool bCancelled);

	void BindAbilityEvents(UAbilitySystemComponent* ASC);
	void UnbindAbilityEvents();
	void HandleContactEvent(const FGameplayEventData* Payload);
	void HandleFinishedEvent(const FGameplayEventData* Payload);
	void HandleCancelledEvent(const FGameplayEventData* Payload);
	bool IsEventForActive(const FGameplayEventData* Payload) const;

	EBendsLookLock ResolveLookLock(const UBendsInteractableComponent& Interactable) const;
	void ApplyInputLock(const UBendsInteractableComponent& Interactable);
	void ReleaseInputLock();

	void EnsurePromptWidget();

	UPROPERTY(Transient)
	TObjectPtr<UBendsInteractionPromptWidget> PromptWidget;

	FBendsInteractionFocusChanged FocusChangedEvent;
	FBendsInteractionStateEvent StartedEvent;
	FBendsInteractionStateEvent FinishedEvent;

	TWeakObjectPtr<UBendsInteractableComponent> Focused;
	TWeakObjectPtr<UBendsInteractableComponent> Active;
	EBendsViewMode ActiveViewMode = EBendsViewMode::FirstPerson;

	FGameplayTagContainer BlockReasons;

	float ScanTimer = 0.f;
	float HoldTime = 0.f;
	float BusyTimer = 0.f;
	bool bInputHeld = false;
	bool bBusy = false;
	bool bContactApplied = false;

	// GASのイベント
	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
	FDelegateHandle ContactHandle;
	FDelegateHandle FinishedHandle;
	FDelegateHandle CancelledHandle;

	// 入力の固定
	TWeakObjectPtr<APlayerController> LockedController;
	TWeakObjectPtr<APlayerCameraManager> LimitedCameraManager;
	float SavedYawMin = 0.f;
	float SavedYawMax = 0.f;
	float SavedPitchMin = 0.f;
	float SavedPitchMax = 0.f;
	bool bMoveLocked = false;
	bool bLookLocked = false;
	bool bLookLimited = false;
};


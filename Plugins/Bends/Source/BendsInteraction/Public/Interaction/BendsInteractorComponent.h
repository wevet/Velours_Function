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
 * A component attached to the player’s pawn that enables interaction.
 *
 * How to select candidates (per ScanInterval):
 *   Select the object closest to the direction of the player’s line of sight from among registered targets. Exclude objects if they are obscured.
 *   FPS … Distance and obstruction are measured from the camera.
 *   TPS … The direction of aim is measured from the camera; distance and obstruction are measured from the character.
 *         If nothing is in the line of sight, select from objects near the character’s front (ProximityRadius).
 *   The view is obtained from IBendsViewModeProvider.
 *
 * Input:
 *   Calls `BeginInput` (press) and `EndInput` (release) as part of the pawn’s interact action.
 *   For single-tap targets, execute immediately upon tapping; for long-press targets, execute when HoldDuration is reached.
 *
 * Execution Flow:
 *   1. Stop movement (based on target settings). Whether to stop, restrict, or not stop movement is determined by CVar or target settings
 *   2. If the target has an AbilityEventTag, send a Gameplay Event to the pawn’s ASC
 *   3. Wait for the event sent back from the ability
 *        Bends.Interaction.Event.Contact   … Produce a result (execute the target’s Effect)
 *        Bends.Interaction.Event.Finished  … End (if Contact hasn’t occurred yet, produce the result here)
 *        Bends.Interaction.Event.Cancelled … End without producing a result
 *   4. Restore input locking
 *   If there is no ability, no ASC, or the BusyTimeout has been exceeded, display a warning and proceed.
 *
 * Temporary blocking is controlled by adding or subtracting reason tags (Bends.Interaction.Block.*).
 *
 * CVar:
 *   bends.Interaction.LookLock 0/1/2 … Viewpoint during operation (do not lock/lock/restrict range)
 *   bends.Interaction.Debug    0/1   … Display debug information for candidates
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


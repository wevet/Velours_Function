// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/VBaseHuman.h"
#include "InputActionValue.h"
#include "MoverSimulationTypes.h"
#include "PlayerCharacter.generated.h"

class UWvSpringArmComponent;
class UQTEActionComponent;
class AWvPlayerController;
class UPostProcessComponent;
class UGameplayCameraComponent;

/**
 * 
 */
UCLASS()
class VELOURS_API APlayerCharacter : public AVBaseHuman, public IWvKeyableTargetInterface, public IMoverInputProducerInterface
{
	GENERATED_BODY()
	
public:
	APlayerCharacter(const FObjectInitializer& ObjectInitializer);
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	virtual void NotifyControllerChanged() override;

	// mover override
	virtual void ProduceInput_Implementation(int32 SimTimeMs, FMoverInputCmdContext& InputCmdResult) override;

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;



protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UWvSpringArmComponent> CameraBoom{ nullptr };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UCameraComponent> TPSCamera{ nullptr };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UQTEActionComponent> QTEActionComponent{ nullptr };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UPostProcessComponent> PostProcessBodyCamComponent{nullptr};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UGameplayCameraComponent> GameplayCameraComponent{nullptr};

	UFUNCTION(BlueprintImplementableEvent, Category = "PlayerCharacter|Input")
	void MoveCallback(const FVector2D& Value);

	// input
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "PlayerCharacter|Input", meta = (AllowPrivateAccess = "true"))
	class UInputMappingContext* DefaultMappingContext;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "PlayerCharacter|Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInputAction> LookAction;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "PlayerCharacter|Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "PlayerCharacter|Input")
	float MouseSensitibity{1.0f};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "PlayerCharacter|Input")
	bool bIsInvertMouse{ false };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "PlayerCharacter|Input")
	FYawHoldState YawHold;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PlayerCharacter|Input")
	float HeldYawDelta{0.f};

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);


public:
	FORCEINLINE class UQTEActionComponent* GetQTEActionComponent() const { return QTEActionComponent; }
	FORCEINLINE class UWvSpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE class UCameraComponent* GetTPSCamera() const { return TPSCamera; }


	UPROPERTY(BlueprintAssignable)
	FBoolOneParamDelegate FPSChangeDelegate;


#pragma region IWvKeyableTargetInterface
	virtual bool IsInputKeyDisable() const override;
	virtual void SetKeyInputDisable() override;
	virtual void SetKeyInputEnable() override;
#pragma endregion

	bool IsQTEActionPlaying() const;

	void NotifyRegisterMission(const int32 MissionIndex);

	UFUNCTION(BlueprintCallable, Category="Player|UI")
	bool GetOverlayMenuOpen() const;

	virtual bool IsFPSControlMode() const override;

	UFUNCTION(BlueprintCallable, Category = "Player|System")
	void SetFPSMode(const bool bIsEnableFPS);

protected:
	virtual void PostAbilitySystemInitialize() override;

private:
	void TryNotifyControllerAbilitySystemInitialized();

	void HandleStanceMode();
	void HandleTargetLock();
	void HandleRotationMode();


	void HandleJump(const bool bIsPress);
	void HandleSprinting(const bool bIsPress);
	void HandleWalking(const bool bIsPress);
	void HandleMeleeAction(const bool bIsPress);
	void HandleDriveAction(const bool bIsPress);
	void HandleAliveAction(const bool bIsPress);
	void HandleHoldAimAction(const bool bIsPress);
	void HandleFinisherAction(const FGameplayTag Tag, const bool bIsPress);
	bool HasFinisherAction(const FGameplayTag Tag) const;
	void HandleQTEAction(const bool bIsPress);


	void DoBulletAttack_Callback();
	void Clear_BulletTimer();
	FTimerHandle Bullet_TimerHandle;

	UFUNCTION()
	void GameplayTagTrigger_Callback(const FGameplayTag Tag, const bool bIsPress);

	UFUNCTION()
	void OnPluralInputEventTrigger_Callback(const FGameplayTag Tag, const bool bIsPress);

	UFUNCTION()
	void OnHoldingInputEventTrigger_Callback(const FGameplayTag Tag, const bool bIsPress);

	UFUNCTION()
	void OnDoubleClickInputEventTrigger_Callback(const FGameplayTag Tag, const bool bIsPress);

	UFUNCTION()
	void OnInputEventGameplayTagTrigger_UI_Callback(const FGameplayTag Tag, const bool bIsPress);

	UFUNCTION()
	void OnQTEBegin_Callback();

	UFUNCTION()
	void OnQTEEnd_Callback(const bool bIsSuccess);

	UPROPERTY(Transient)
	TObjectPtr<AWvPlayerController> P_Controller{ nullptr };

};

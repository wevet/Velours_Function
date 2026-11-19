// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Character/CharacterSystemTypes.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "LocomotionComponent.generated.h"

class UHitTargetComponent;
class ABasePawn;
class UMoverComponent;

namespace LocomotionDebug
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

	extern TAutoConsoleVariable<int32> CVarDebugLocomotionSystem;

#endif
}

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLocomotionOverlayChangeDelegate, const ELSOverlayState, PrevOverlay, const ELSOverlayState, CurrentOverlay);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLocomotionGaitChangeDelegate, const ELSGait, InGait);

UCLASS(BlueprintType)
class VELOURS_API ULocomotionStateDataAsset : public UDataAsset
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FName, FGameplayTag> MovementModeTagMap;


public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag AimingTag;

public:
	FGameplayTag FindMovementModeTag(const FName& MovementModeName) const;
};



UCLASS( ClassGroup=(Movement), meta=(BlueprintSpawnableComponent) )
class VELOURS_API ULocomotionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	ULocomotionComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	virtual void BeginPlay() override;


public:	
	UFUNCTION(BlueprintCallable, Category = "Locomotion")
	void SetStanceMode(const ELSStance NewLSStance);

	UFUNCTION(BlueprintCallable, Category = "Locomotion")
	void SetRotationMode(const ELSRotationMode NewLSRotationMode);

	UFUNCTION(BlueprintCallable, Category = "Locomotion")
	void SetOverlayState(const ELSOverlayState NewLSOverlayState);

	UFUNCTION(BlueprintCallable, Category = "Locomotion")
	void SetGaitMode(const ELSGait NewGait);

	UFUNCTION(BlueprintCallable, Category = "Locomotion")
	void SetLookAimTarget(const bool NewLookAtAimOffset, AActor* NewLookAtTarget);

	UFUNCTION(BlueprintCallable, Category = "Locomotion")
	void UpdateLookAimTargetComponent(UHitTargetComponent* NewLookAtTargetComponent);

	UFUNCTION(BlueprintCallable, Category = "Locomotion")
	ELSOverlayState GetOverlayState() const { return LocomotionEssencialVariables.OverlayState; }

	UPROPERTY(BlueprintAssignable)
	FLocomotionOverlayChangeDelegate OnOverlayChangeDelegate;

	UPROPERTY(BlueprintAssignable)
	FLocomotionGaitChangeDelegate OnGaitChangeDelegate;

	const FLocomotionEssencialVariables& GetLocomotionEssencialVariables() { return LocomotionEssencialVariables; }

	void OnMovementModeChanged(const FName& PreviousModeName, const FName& NewModeName, FGameplayTag& OutPrevMovementModeTag, FGameplayTag& OutNextMovementModeTag);

	void SetAiming(const bool bIsNewAiming);

	void DrawLocomotionDebug();

	bool CanSprint() const;

	void SprintCheck();

	void SetSprintPressed(const bool NewSprintPressed);

protected:
	UPROPERTY(Transient)
	FLocomotionEssencialVariables LocomotionEssencialVariables;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion|Config")
	TObjectPtr<class ULocomotionStateDataAsset> LocomotionStateDA;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion|Sprint")
	float SprintYawLimit{60.0f};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Locomotion|AimOffset")
	float LookAtInterpSpeed = 6.0f;

	UPROPERTY(EditAnywhere, Category = "Locomotion|Sprint", meta = (ClampMin = 0.0, ClampMax = 45.0))
	float SprintYawHysteresis = 10.f;

	UPROPERTY(EditAnywhere, Category = "Locomotion|Sprint", meta = (ClampMin = 0.0, ClampMax = 1.0))
	float SprintMinInputSize = 0.5f;

private:
	FVector GetMoverInputIntent() const;
	float GetMoverMaxAcceleration() const;

	void CalculateEssentialVariables(const float DeltaSeconds);

	FVector ChooseTargetPosition() const;

	const FVector ChooseVelocity();

	TWeakObjectPtr<class ABasePawn> Character;
	TWeakObjectPtr<UMoverComponent> MoverComponent;

	bool bIsOwnerPlayerController{false};
	bool bDoSprint{ false };
	bool bDoRunning{ false };
	bool bShouldSprint{ false };

	FVector PrevPos = FVector::ZeroVector;
	FVector PrevVel = FVector::ZeroVector;
};

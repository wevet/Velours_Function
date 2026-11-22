// Copyright 2022 wevet works All Rights Reserved.


#include "Component/LocomotionComponent.h"
#include "Component/HitTargetComponent.h"
#include "Character/BasePawn.h"
#include "Velours.h"

#include "Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"

#include "MoverComponent.h"
#include "MoverSimulationTypes.h"
#include "MoverDataModelTypes.h"
#include "DefaultMovementSet/Settings/CommonLegacyMovementSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LocomotionComponent)


DEFINE_LOG_CATEGORY_STATIC(LogVLocomotion, Log, All);

namespace LocomotionDebug
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	TAutoConsoleVariable<int32> CVarDebugLocomotionSystem(TEXT("wv.LocomotionSystem.Debug"), 0, TEXT("LocomotionSystem Debug .\n") TEXT("<=0: off\n") TEXT("  1: on\n") TEXT("  2: player ignore\n"), ECVF_Default);
#endif

}


FGameplayTag ULocomotionStateDataAsset::FindMovementModeTag(const FName& MovementModeName) const
{
	return MovementModeTagMap.FindRef(MovementModeName);
}



ULocomotionComponent::ULocomotionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void ULocomotionComponent::BeginPlay()
{
	Super::BeginPlay();

	Character = Cast<ABasePawn>(GetOwner());

	if (Character.IsValid())
	{
		bIsOwnerPlayerController = bool(Cast<APlayerController>(Character->GetController()));
		MoverComponent = Character->FindComponentByClass<UMoverComponent>();
	}

}

void ULocomotionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	CalculateEssentialVariables(DeltaTime);

	SprintCheck();

	DrawLocomotionDebug();
}

void ULocomotionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Character.Reset();
	Super::EndPlay(EndPlayReason);
}

void ULocomotionComponent::SetStanceMode(const ELSStance NewLSStance)
{
	LocomotionEssencialVariables.LSStance = NewLSStance;
}

void ULocomotionComponent::SetRotationMode(const ELSRotationMode NewLSRotationMode)
{
	if (LocomotionEssencialVariables.LSRotationMode == NewLSRotationMode)
	{
		return;
	}

	LocomotionEssencialVariables.LSRotationMode = NewLSRotationMode;

	if (OnRotationChangeDelegate.IsBound())
	{
		OnRotationChangeDelegate.Broadcast(NewLSRotationMode);
	}

	const FString CategoryName = *FString::Format(TEXT("ELSRotationMode => {0}"), { *GETENUMSTRING("/Script/Velours.ELSRotationMode", NewLSRotationMode) });
	UE_LOG(LogVLocomotion, Log, TEXT("[%s] : %s"), ANSI_TO_TCHAR(__FUNCTION__), *CategoryName);
}

void ULocomotionComponent::SetOverlayState(const ELSOverlayState NewLSOverlayState)
{
	if (LocomotionEssencialVariables.OverlayState == NewLSOverlayState)
	{
		return;
	}

	const ELSOverlayState PrevOverlay = LocomotionEssencialVariables.OverlayState;
	LocomotionEssencialVariables.OverlayState = NewLSOverlayState;
	if (OnOverlayChangeDelegate.IsBound())
	{
		OnOverlayChangeDelegate.Broadcast(PrevOverlay, NewLSOverlayState);
	}
}

void ULocomotionComponent::SetGaitMode(const ELSGait NewGait)
{
	if (LocomotionEssencialVariables.LSGait == NewGait)
	{
		return;
	}

	LocomotionEssencialVariables.LSGait = NewGait;

	if (OnGaitChangeDelegate.IsBound())
	{
		OnGaitChangeDelegate.Broadcast(NewGait);
	}

	const FString CategoryName = *FString::Format(TEXT("ELSGait => {0}"), { *GETENUMSTRING("/Script/Velours.ELSGait", NewGait) });
	UE_LOG(LogVLocomotion, Log, TEXT("[%s] : %s"), ANSI_TO_TCHAR(__FUNCTION__), *CategoryName);

}

void ULocomotionComponent::SetLookAimTarget(const bool NewLookAtAimOffset, AActor* NewLookAtTarget)
{
	LocomotionEssencialVariables.bLookAtAimOffset = NewLookAtAimOffset && IsValid(NewLookAtTarget);
	LocomotionEssencialVariables.LookAtTarget = NewLookAtTarget;
	LocomotionEssencialVariables.LookAtTargetComponent = (NewLookAtAimOffset && IsValid(NewLookAtTarget)) ?
		NewLookAtTarget->FindComponentByClass<UHitTargetComponent>() : nullptr;
}

void ULocomotionComponent::UpdateLookAimTargetComponent(UHitTargetComponent* NewLookAtTargetComponent)
{
	if (LocomotionEssencialVariables.bLookAtAimOffset)
	{
		LocomotionEssencialVariables.LookAtTargetComponent = NewLookAtTargetComponent;
	}

}


FVector ULocomotionComponent::GetMoverInputIntent() const
{
	const UMoverComponent* Mover = MoverComponent.Get();
	if (!Mover)
	{
		return FVector::ZeroVector;
	}

	// 1. 最後に使われた入力（BPのProduceInputが Add Data To Collection で入れたもの）
	if (const FCharacterDefaultInputs* Inputs = Mover->GetLastInputCmd().InputCollection.FindDataByType<FCharacterDefaultInputs>())
	{
		const FVector MoveInput = Inputs->GetMoveInput();
		if (!MoveInput.IsNearlyZero())
		{
			return MoveInput;
		}
	}

	// 2. 移動モードが記録した意図
	const FVector Intent = Mover->GetMovementIntent();
	if (!Intent.IsNearlyZero())
	{
		return Intent;
	}

	// 3. 同期された状態（他のプレイヤー）
	if (const FMoverDefaultSyncState* SyncState = Mover->GetSyncState().SyncStateCollection.FindDataByType<FMoverDefaultSyncState>())
	{
		return SyncState->MoveDirectionIntent;
	}
	return FVector::ZeroVector;
}

float ULocomotionComponent::GetMoverMaxAcceleration() const
{
	const UMoverComponent* Mover = MoverComponent.Get();
	const UCommonLegacyMovementSettings* Settings = Mover ? Mover->FindSharedSettings<UCommonLegacyMovementSettings>() : nullptr;
	return Settings ? Settings->Acceleration : 0.f;
}

const FVector ULocomotionComponent::ChooseVelocity()
{


	if (const UMoverComponent* Mover = MoverComponent.Get())
	{
		return Mover->GetVelocity();
	}

	{
		//if (LocomotionEssencialVariables.LSMovementMode == ELSMovementMode::Ragdoll)
		//{
		//	if (SkeletalMeshComponent.IsValid())
		//	{
		//		Vel = SkeletalMeshComponent->GetPhysicsLinearVelocity(PelvisBoneName);
		//	}
		//}
	}

	const FVector Pos = GetOwner()->GetActorLocation();
	const float DT = FMath::Max(GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.f, KINDA_SMALL_NUMBER);
	return (Pos - PrevPos) / DT;
}

void ULocomotionComponent::CalculateEssentialVariables(const float DeltaSeconds)
{
	if (!Character.IsValid())
	{
		return;
	}


	const FVector Pos = GetOwner()->GetActorLocation();
	const float DT = FMath::Max(DeltaSeconds, KINDA_SMALL_NUMBER);

	const FVector Vel = ChooseVelocity();

	LocomotionEssencialVariables.Velocity = Vel;

	LocomotionEssencialVariables.CharacterRotation = GetOwner()->GetActorRotation();

	const FVector InputIntent = GetMoverInputIntent();
	LocomotionEssencialVariables.WorldAcceleration2D = InputIntent * GetMoverMaxAcceleration() * FVector(1.0f, 1.0f, 0.0f);
	LocomotionEssencialVariables.LocalAcceleration2D = UKismetMathLibrary::Quat_UnrotateVector(
		Character->GetActorRotation().Quaternion(),
		LocomotionEssencialVariables.WorldAcceleration2D);

	LocomotionEssencialVariables.LocalVelocity2D = UKismetMathLibrary::Quat_UnrotateVector(
		Character->GetActorRotation().Quaternion(), 
		LocomotionEssencialVariables.Velocity * FVector(1.0f, 1.0f, 0.0f));

	const float AccThresh = K_MOVING_ACC_THRESHOLD;

	//UE_LOG(LogVLocomotion, Log, TEXT("InputIntent : %s"), *InputIntent.ToString());

	// Check if the Character is moving and set (last speed rotation) and (direction) only when it is moving . so that they do not return to 0 when the speed is 0.
	const FVector CurrentVector = FVector(Vel.X, Vel.Y, 0.0f);
	//LocomotionEssencialVariables.bWasMoving = (Vel.Size2D() > K_MOVING_THRESHOLD);
	LocomotionEssencialVariables.bWasMoving = UKismetMathLibrary::NotEqual_VectorVector(CurrentVector, FVector::ZeroVector, 1.0f);
	LocomotionEssencialVariables.HasAcceleration = (LocomotionEssencialVariables.LocalAcceleration2D.SizeSquared() > AccThresh * AccThresh);

	PrevPos = Pos;
	PrevVel = Vel;

	//  Update only while in motion
	if (LocomotionEssencialVariables.bWasMoving)
	{
		LocomotionEssencialVariables.LastVelocityRotation = UKismetMathLibrary::Conv_VectorToRotator(Vel);
		LocomotionEssencialVariables.Direction = UKismetMathLibrary::NormalizedDeltaRotator(
			LocomotionEssencialVariables.LastVelocityRotation,
			LocomotionEssencialVariables.CharacterRotation).Yaw;

	}

	LocomotionEssencialVariables.MovementInput = InputIntent;
	LocomotionEssencialVariables.bWasMovementInput = UKismetMathLibrary::NotEqual_VectorVector(
		LocomotionEssencialVariables.MovementInput, FVector::ZeroVector, 0.0001f);

	if (LocomotionEssencialVariables.bWasMovementInput)
	{
		LocomotionEssencialVariables.LastMovementInputRotation = UKismetMathLibrary::Conv_VectorToRotator(LocomotionEssencialVariables.MovementInput);
		LocomotionEssencialVariables.VelocityDifference = UKismetMathLibrary::NormalizedDeltaRotator(
			LocomotionEssencialVariables.LastMovementInputRotation,
			LocomotionEssencialVariables.LastVelocityRotation).Yaw;
	}

	const float PrevAimYaw = LocomotionEssencialVariables.LookingRotation.Yaw;
	const FRotator CurrentLockingRotation = LocomotionEssencialVariables.LookingRotation;
	if (LocomotionEssencialVariables.bLookAtAimOffset)
	{
		const FVector Start = Character->GetActorLocation();
		const FVector Target = ChooseTargetPosition();
		LocomotionEssencialVariables.LookingRotation = UKismetMathLibrary::FindLookAtRotation(Start, Target);
	}
	else
	{
		LocomotionEssencialVariables.LookingRotation = Character->GetControlRotation();
	}

	LocomotionEssencialVariables.LookingRotation = UKismetMathLibrary::RInterpTo(
		CurrentLockingRotation,
		LocomotionEssencialVariables.LookingRotation, 
		DeltaSeconds, 
		LookAtInterpSpeed);

	const float YawDelta = FRotator::NormalizeAxis(LocomotionEssencialVariables.LookingRotation.Yaw - PrevAimYaw);
	LocomotionEssencialVariables.AimYawRate = YawDelta / DT;
	LocomotionEssencialVariables.AimYawDelta = UKismetMathLibrary::NormalizedDeltaRotator(
		LocomotionEssencialVariables.LookingRotation,
		LocomotionEssencialVariables.CharacterRotation).Yaw;
}


FVector ULocomotionComponent::ChooseTargetPosition() const
{
	// 1st target Component
	// 2nd Owner ActorLocation
	if (LocomotionEssencialVariables.LookAtTargetComponent.IsValid())
	{
		auto Comp = LocomotionEssencialVariables.LookAtTargetComponent.Get();
		return Comp->GetComponentLocation();
	}
	return LocomotionEssencialVariables.LookAtTarget.IsValid() ? LocomotionEssencialVariables.LookAtTarget->GetActorLocation() : FVector::ZeroVector;
}

void ULocomotionComponent::OnMovementModeChanged(
	const FName& PreviousModeName,
	const FName& NewModeName,
	FGameplayTag& OutPrevMovementModeTag,
	FGameplayTag& OutNextMovementModeTag)
{

	if (LocomotionStateDA)
	{
		OutPrevMovementModeTag = LocomotionStateDA->FindMovementModeTag(PreviousModeName);
		OutNextMovementModeTag = LocomotionStateDA->FindMovementModeTag(NewModeName);

		UE_LOG(LogVLocomotion, Log, TEXT("[%s] : PreviousModeName => %s, NewModeName => %s"),
			*FString(__FUNCTION__),
			*PreviousModeName.ToString(),
			*NewModeName.ToString());
	}
	else
	{
		UE_LOG(LogVLocomotion, Error, TEXT("[%s] : nullptr LocomotionStateDA"), *FString(__FUNCTION__));
	}
}

void ULocomotionComponent::SetAiming(const bool bIsNewAiming)
{
	const bool bIsAiming = LocomotionEssencialVariables.bAiming;
	if (bIsAiming != bIsNewAiming)
	{
		LocomotionEssencialVariables.bAiming = bIsNewAiming;

		UE_LOG(LogBaseCharacter, Verbose, TEXT("[%s] : bAiming => %s"),
			*FString(__FUNCTION__), LocomotionEssencialVariables.bAiming ? TEXT("true") : TEXT("false"));

	}

}

bool ULocomotionComponent::CanSprint() const
{
	if (LocomotionEssencialVariables.LSRotationMode == ELSRotationMode::VelocityDirection)
	{
		return true;
	}

	//if (LocomotionEssencialVariables.bAiming)
	//{
	//	return false;
	//}

	// 今の入力から直接見る（LastMovementInputRotationの更新に頼らない）
	const FVector Input2D = FVector(LocomotionEssencialVariables.MovementInput.X, LocomotionEssencialVariables.MovementInput.Y, 0.f);

	//UE_LOG(LogVLocomotion, Warning, TEXT("[%s] Input2D.IsNearlyZero : %s"), *GetNameSafe(GetOwner()), *LocomotionEssencialVariables.MovementInput.ToString());

	// 入力がないときは、今の状態を保つ（止まった瞬間に走りへ戻さない）
	if (Input2D.IsNearlyZero(0.01f))
	{
		//UE_LOG(LogVLocomotion, Warning, TEXT("Input2D.IsNearlyZero"));
		return bDoSprint;
	}

	// スティックを少し倒しただけではスプリントしない
	if (Input2D.Size() < SprintMinInputSize)
	{
		//UE_LOG(LogVLocomotion, Warning, TEXT("Input2D.SprintMinInputSize"));
		return false;
	}

	// 入力の向きと視線の向きの差（ヨーだけ）
	const float InputYaw = Input2D.Rotation().Yaw;
	const float LookYaw = LocomotionEssencialVariables.LookingRotation.Yaw;
	const float DeltaYaw = FMath::Abs(FRotator::NormalizeAxis(InputYaw - LookYaw));

	// 前（0°）と斜め前（45°）はスプリント、横（90°）と後ろはしない
	// スプリント中は、戻る角度を少し広げて、境目でちらつかないようにする
	const float Limit = bDoSprint ? (SprintYawLimit + SprintYawHysteresis) : SprintYawLimit;

	//UE_LOG(LogVLocomotion, Warning, TEXT("DeltaYaw : %.2f, Limit : %.2f"), DeltaYaw, Limit);
	return DeltaYaw < Limit;
}

void ULocomotionComponent::SetSprintPressed(const bool NewSprintPressed)
{
	if (bShouldSprint == NewSprintPressed)
	{
		return;
	}

	//if (LocomotionEssencialVariables.bAiming)
	//{
	//	return;
	//}

	bShouldSprint = NewSprintPressed;

	UE_LOG(LogVLocomotion, Log, TEXT("[%s] bShouldSprint : %s"), *GetNameSafe(GetOwner()), bShouldSprint ? TEXT("true") : TEXT("false"));
}


void ULocomotionComponent::SprintCheck()
{
	if (bShouldSprint)
	{
		const bool bCanSprint = CanSprint();
		if (bCanSprint)
		{
			if (!bDoSprint)
			{
				bDoSprint = true;
				bDoRunning = false;
				SetGaitMode(ELSGait::Sprinting);
			}
		}
		else
		{
			const bool bWasJustWalked = (LocomotionEssencialVariables.LSGait == ELSGait::Walking);

			if (!bDoRunning || bWasJustWalked)
			{
				bDoRunning = true;
				bDoSprint = false;
				SetGaitMode(ELSGait::Running);
			}
		}

		//UE_LOG(LogVLocomotion, Log, TEXT("bCanSprint : %s"), bCanSprint ? TEXT("true") : TEXT("false"));
	}
	else
	{

		if (LocomotionEssencialVariables.LSGait == ELSGait::Walking)
			return;

		if (!bDoRunning)
		{
			bDoRunning = true;
			bDoSprint = false;
			SetGaitMode(ELSGait::Running);
		}

	}

}

void ULocomotionComponent::DrawLocomotionDebug()
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (!Character.IsValid())
	{
		return;
	}

	if (LocomotionDebug::CVarDebugLocomotionSystem.GetValueOnGameThread() > 0)
	{
		if (!bIsOwnerPlayerController)
		{
			return;
		}

		const UWorld* World = GetWorld();
		const FVector BaseLocation = Character->GetActorLocation();

		// draw character rotation orig
		{
			const FVector RelativeLocation = BaseLocation + FVector::UpVector * 80.f;
			DrawDebugDirectionalArrow(World, RelativeLocation, RelativeLocation + Character->GetActorRotation().Vector() * 100.f, 10.f, FColor::Black, false, 0.f, 0, 2.f);
		}

		// last movement input
		{
			const FVector RelativeLocation = BaseLocation + FVector::UpVector * 60.f;
			DrawDebugDirectionalArrow(World, RelativeLocation, RelativeLocation + LocomotionEssencialVariables.LastMovementInputRotation.Vector() * 100.f, 10.f, FColor::Red, false, 0.f, 0, 2.f);
		}

		// looking rotation
		{
			const FVector RelativeLocation = BaseLocation + FVector::UpVector * 40.f;
			DrawDebugDirectionalArrow(World, RelativeLocation, RelativeLocation + LocomotionEssencialVariables.LookingRotation.Vector() * 100.f, 10.f, FColor::Cyan, false, 0.f, 0, 2.f);
		}

		// movement input
		{
			const FVector RelativeLocation = BaseLocation + FVector::UpVector * 20.f;
			DrawDebugDirectionalArrow(World, RelativeLocation, RelativeLocation + LocomotionEssencialVariables.MovementInput * 100.f, 10.f, FColor::Blue, false, 0.f, 0, 2.f);
		}

		// draw character rotation
		{
			const FVector RelativeLocation = BaseLocation + FVector::UpVector * -20.f;
			DrawDebugDirectionalArrow(World, RelativeLocation, RelativeLocation + LocomotionEssencialVariables.CharacterRotation.Vector() * 100.f, 10.f, FColor::Green, false, 0.f, 0, 2.f);
		}

		// velocity
		{
			const FVector RelativeLocation = BaseLocation + FVector::UpVector * -40.f;
			DrawDebugDirectionalArrow(World, RelativeLocation, RelativeLocation + LocomotionEssencialVariables.LastVelocityRotation.Vector() * 100.f, 10.f, FColor::Magenta, false, 0.f, 0, 2.f);
		}

	}

#endif
}




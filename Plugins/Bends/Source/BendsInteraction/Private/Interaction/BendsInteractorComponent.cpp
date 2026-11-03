// Copyright 2022 wevet works All Rights Reserved.

#include "Interaction/BendsInteractorComponent.h"
#include "Interaction/BendsInteractableComponent.h"
#include "Interaction/BendsInteractionSubsystem.h"
#include "UI/BendsInteractionPromptWidget.h"
#include "BendsInteractionTags.h"
#include "BendsInteraction.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameplayPrediction.h"
#include "Camera/PlayerCameraManager.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BendsInteractorComponent)

namespace
{
	TAutoConsoleVariable<int32> CVarBendsInteractionLookLock(
		TEXT("bends.Interaction.LookLock"),
		0,
		TEXT("Look input while interacting. 0 = free, 1 = locked, 2 = limited around the start direction."),
		ECVF_Default);

	TAutoConsoleVariable<bool> CVarBendsInteractionDebug(
		TEXT("bends.Interaction.Debug"),
		false,
		TEXT("Draw interaction candidates."),
		ECVF_Cheat);
}

UBendsInteractorComponent::UBendsInteractorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	// TPSは距離をキャラクターから測るので、少し広めにする
	ThirdPersonScan.MaxDistance = 160.f;
	ThirdPersonScan.MaxViewAngle = 15.f;
	ThirdPersonScan.ProximityRadius = 120.f;
}

void UBendsInteractorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bBusy)
	{
		FinishInteraction(true);
	}
	if (PromptWidget)
	{
		PromptWidget->RemoveFromParent();
		PromptWidget = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

#pragma region Input
void UBendsInteractorComponent::BeginInput()
{
	bInputHeld = true;
	HoldTime = 0.f;

	UBendsInteractableComponent* Target = Focused.Get();
	if (!bBusy && Target && Target->Mode == EBendsInteractionMode::Press)
	{
		StartInteraction(Target);
	}
}

void UBendsInteractorComponent::EndInput()
{
	bInputHeld = false;
	HoldTime = 0.f;
}

void UBendsInteractorComponent::AddBlockReason(const FGameplayTag& Reason)
{
	if (Reason.IsValid())
	{
		BlockReasons.AddTag(Reason);
		SetFocus(nullptr);
	}
}

void UBendsInteractorComponent::RemoveBlockReason(const FGameplayTag& Reason)
{
	BlockReasons.RemoveTag(Reason);
}

float UBendsInteractorComponent::GetHoldProgress() const
{
	const UBendsInteractableComponent* Target = Focused.Get();
	if (!Target || Target->Mode != EBendsInteractionMode::Hold || Target->HoldDuration <= 0.f)
	{
		return 0.f;
	}
	return FMath::Clamp(HoldTime / Target->HoldDuration, 0.f, 1.f);
}
#pragma endregion

EBendsViewMode UBendsInteractorComponent::GetViewMode() const
{
	return IBendsViewModeProvider::GetViewModeOf(GetOwner());
}

void UBendsInteractorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!IsLocalPlayerPawn())
	{
		SetFocus(nullptr);
		return;
	}

	EnsurePromptWidget();

	if (bBusy)
	{
		BusyTimer += DeltaTime;
		if (BusyTimer >= BusyTimeout)
		{
			UE_LOG(LogBendsInteraction, Warning, TEXT("Interaction timed out: %s. Did the ability send the Finished event?"), *GetNameSafe(Active.IsValid() ? Active->GetOwner() : nullptr));
			ApplyContact();
			FinishInteraction(false);
		}
		return;
	}

	ScanTimer += DeltaTime;
	if (ScanTimer >= ScanInterval)
	{
		ScanTimer = 0.f;
		Scan();
	}

	UBendsInteractableComponent* Target = Focused.Get();
	if (bInputHeld && Target && Target->Mode == EBendsInteractionMode::Hold)
	{
		HoldTime += DeltaTime;
		if (HoldTime >= Target->HoldDuration)
		{
			StartInteraction(Target);
		}
	}
}

#pragma region Scan
void UBendsInteractorComponent::Scan()
{
	APawn* Pawn = GetPawn();
	APlayerController* PC = GetPlayerController();
	UWorld* World = GetWorld();
	UBendsInteractionSubsystem* Subsystem = World ? World->GetSubsystem<UBendsInteractionSubsystem>() : nullptr;

	if (!Pawn || !PC || !Subsystem || IsBlocked())
	{
		SetFocus(nullptr);
		return;
	}

	const EBendsViewMode ViewMode = GetViewMode();
	const FBendsInteractionScanSettings& Settings = (ViewMode == EBendsViewMode::FirstPerson) ? FirstPersonScan : ThirdPersonScan;

	FVector ViewLocation;
	FRotator ViewRotation;
	PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
	const FVector ViewDirection = ViewRotation.Vector();

	// FPSはカメラから、TPSはキャラクターから距離と遮蔽を測る
	const FVector BodyLocation = (ViewMode == EBendsViewMode::FirstPerson) ? ViewLocation : Pawn->GetPawnViewLocation();
	const FVector PawnForward = Pawn->GetActorForwardVector().GetSafeNormal2D();

	const float CosViewAngle = FMath::Cos(FMath::DegreesToRadians(Settings.MaxViewAngle));
	const float CosProximityAngle = FMath::Cos(FMath::DegreesToRadians(Settings.ProximityAngle));

	UBendsInteractableComponent* Best = nullptr;
	float BestDot = -1.f;
	UBendsInteractableComponent* Nearby = nullptr;
	float NearbyDistSq = TNumericLimits<float>::Max();

	const bool bDebug = CVarBendsInteractionDebug.GetValueOnGameThread();

	for (const TWeakObjectPtr<UBendsInteractableComponent>& Entry : Subsystem->GetInteractables())
	{
		UBendsInteractableComponent* Candidate = Entry.Get();
		if (!Candidate || !Candidate->CanInteract(Pawn))
		{
			continue;
		}

		const FVector Location = Candidate->GetComponentLocation();
		const float MaxDistance = Candidate->MaxDistance > 0.f ? Candidate->MaxDistance : Settings.MaxDistance;
		const float DistSq = FVector::DistSquared(BodyLocation, Location);
		if (DistSq > FMath::Square(MaxDistance))
		{
			continue;
		}

		const float Dot = FVector::DotProduct(ViewDirection, (Location - ViewLocation).GetSafeNormal());
		if (Dot >= CosViewAngle)
		{
			if (Dot > BestDot && HasLineOfSight(BodyLocation, Candidate, Pawn))
			{
				Best = Candidate;
				BestDot = Dot;
			}
		}
		else if (Settings.ProximityRadius > 0.f && DistSq <= FMath::Square(Settings.ProximityRadius))
		{
			const FVector ToCandidate = (Location - Pawn->GetActorLocation()).GetSafeNormal2D();
			if (FVector::DotProduct(PawnForward, ToCandidate) >= CosProximityAngle
				&& DistSq < NearbyDistSq
				&& HasLineOfSight(BodyLocation, Candidate, Pawn))
			{
				Nearby = Candidate;
				NearbyDistSq = DistSq;
			}
		}

#if ENABLE_DRAW_DEBUG
		if (bDebug)
		{
			DrawDebugSphere(World, Location, 5.f, 8, FColor::Yellow, false, ScanInterval * 1.5f);
		}
#endif
	}

	UBendsInteractableComponent* NewFocus = Best ? Best : Nearby;
	SetFocus(NewFocus);

#if ENABLE_DRAW_DEBUG
	if (bDebug && NewFocus)
	{
		DrawDebugSphere(World, NewFocus->GetComponentLocation(), 8.f, 12, FColor::Green, false, ScanInterval * 1.5f);
		DrawDebugLine(World, BodyLocation, NewFocus->GetComponentLocation(), FColor::Green, false, ScanInterval * 1.5f);
	}
#endif
}

bool UBendsInteractorComponent::HasLineOfSight(const FVector& From, const UBendsInteractableComponent* Interactable, const APawn* Pawn) const
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(BendsInteractionOcclusion), false);
	Params.AddIgnoredActor(Pawn);
	Params.AddIgnoredActor(Interactable->GetOwner());

	FHitResult Hit;
	return !GetWorld()->LineTraceSingleByChannel(Hit, From, Interactable->GetComponentLocation(), OcclusionChannel, Params);
}

void UBendsInteractorComponent::SetFocus(UBendsInteractableComponent* NewFocus)
{
	UBendsInteractableComponent* OldFocus = Focused.Get();
	if (OldFocus == NewFocus)
	{
		return;
	}
	Focused = NewFocus;
	HoldTime = 0.f;
	FocusChangedEvent.Broadcast(OldFocus, NewFocus);
}
#pragma endregion

#pragma region Interaction
void UBendsInteractorComponent::StartInteraction(UBendsInteractableComponent* Interactable)
{
	APawn* Pawn = GetPawn();
	if (bBusy || !Interactable || !Pawn || IsBlocked() || !Interactable->CanInteract(Pawn))
	{
		return;
	}

	bBusy = true;
	bContactApplied = false;
	bInputHeld = false;
	BusyTimer = 0.f;
	HoldTime = 0.f;
	Active = Interactable;
	ActiveViewMode = GetViewMode();

	SetFocus(nullptr);
	ApplyInputLock(*Interactable);
	StartedEvent.Broadcast(Interactable);

	if (Interactable->AbilityEventTag.IsValid())
	{
		UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn);
		if (ASC)
		{
			BindAbilityEvents(ASC);

			FGameplayEventData Payload;
			Payload.EventTag = Interactable->AbilityEventTag;
			Payload.Instigator = Pawn;
			Payload.Target = Interactable->GetOwner();
			Payload.OptionalObject = Interactable;
			Payload.EventMagnitude = (ActiveViewMode == EBendsViewMode::ThirdPerson) ? 1.f : 0.f;

			FScopedPredictionWindow PredictionWindow(ASC, true);
			const int32 TriggeredCount = ASC->HandleGameplayEvent(Interactable->AbilityEventTag, &Payload);
			if (TriggeredCount > 0)
			{
				return; // アビリティからのイベントを待つ
			}

			UE_LOG(LogBendsInteraction, Warning, TEXT("No ability triggered by %s. Applying the result immediately."), *Interactable->AbilityEventTag.ToString());
			UnbindAbilityEvents();
		}
		else
		{
			UE_LOG(LogBendsInteraction, Warning, TEXT("%s has no AbilitySystemComponent. Applying the result immediately."), *GetNameSafe(Pawn));
		}
	}

	// アニメーションなし
	ApplyContact();
	FinishInteraction(false);
}

void UBendsInteractorComponent::ApplyContact()
{
	if (!bBusy || bContactApplied)
	{
		return;
	}
	bContactApplied = true;

	UBendsInteractableComponent* Interactable = Active.Get();
	if (!Interactable)
	{
		return;
	}

	FBendsInteractionContext Context;
	Context.Instigator = GetPawn();
	Context.Interactable = Interactable;
	Context.Target = Interactable->GetOwner();
	Context.ViewMode = ActiveViewMode;
	Interactable->Interact(Context);
}

void UBendsInteractorComponent::FinishInteraction(bool bCancelled)
{
	if (!bBusy)
	{
		return;
	}

	UnbindAbilityEvents();
	ReleaseInputLock();

	UBendsInteractableComponent* Interactable = Active.Get();
	bBusy = false;
	BusyTimer = 0.f;
	Active.Reset();

	FinishedEvent.Broadcast(Interactable);
}
#pragma endregion

#pragma region Ability
void UBendsInteractorComponent::SendInteractionEvent(AActor* Avatar, const FGameplayTag& EventTag, const UObject* Interactable)
{
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Avatar);
	if (!ASC || !EventTag.IsValid())
	{
		return;
	}

	FGameplayEventData Payload;
	Payload.EventTag = EventTag;
	Payload.Instigator = Avatar;
	Payload.OptionalObject = Interactable;

	FScopedPredictionWindow PredictionWindow(ASC, true);
	ASC->HandleGameplayEvent(EventTag, &Payload);
}

void UBendsInteractorComponent::BindAbilityEvents(UAbilitySystemComponent* ASC)
{
	UnbindAbilityEvents();
	BoundASC = ASC;

	ContactHandle = ASC->GenericGameplayEventCallbacks.FindOrAdd(BendsInteractionTags::Event_Contact)
		.AddUObject(this, &UBendsInteractorComponent::HandleContactEvent);
	FinishedHandle = ASC->GenericGameplayEventCallbacks.FindOrAdd(BendsInteractionTags::Event_Finished)
		.AddUObject(this, &UBendsInteractorComponent::HandleFinishedEvent);
	CancelledHandle = ASC->GenericGameplayEventCallbacks.FindOrAdd(BendsInteractionTags::Event_Cancelled)
		.AddUObject(this, &UBendsInteractorComponent::HandleCancelledEvent);
}

void UBendsInteractorComponent::UnbindAbilityEvents()
{
	if (UAbilitySystemComponent* ASC = BoundASC.Get())
	{
		ASC->GenericGameplayEventCallbacks.FindOrAdd(BendsInteractionTags::Event_Contact).Remove(ContactHandle);
		ASC->GenericGameplayEventCallbacks.FindOrAdd(BendsInteractionTags::Event_Finished).Remove(FinishedHandle);
		ASC->GenericGameplayEventCallbacks.FindOrAdd(BendsInteractionTags::Event_Cancelled).Remove(CancelledHandle);
	}
	BoundASC.Reset();
	ContactHandle.Reset();
	FinishedHandle.Reset();
	CancelledHandle.Reset();
}

bool UBendsInteractorComponent::IsEventForActive(const FGameplayEventData* Payload) const
{
	// OptionalObjectが空のイベント（AnimNotifyから送ったもの）は、今の動作のものとして扱う
	return !Payload || !Payload->OptionalObject || Payload->OptionalObject == Active.Get();
}

void UBendsInteractorComponent::HandleContactEvent(const FGameplayEventData* Payload)
{
	if (IsEventForActive(Payload))
	{
		ApplyContact();
	}
}

void UBendsInteractorComponent::HandleFinishedEvent(const FGameplayEventData* Payload)
{
	if (IsEventForActive(Payload))
	{
		ApplyContact(); // MontageにContactの通知がない場合も、結果は出す
		FinishInteraction(false);
	}
}

void UBendsInteractorComponent::HandleCancelledEvent(const FGameplayEventData* Payload)
{
	if (IsEventForActive(Payload))
	{
		FinishInteraction(true);
	}
}
#pragma endregion

#pragma region InputLock
EBendsLookLock UBendsInteractorComponent::ResolveLookLock(const UBendsInteractableComponent& Interactable) const
{
	if (Interactable.LookLock != EBendsLookLock::UseGlobal)
	{
		return Interactable.LookLock;
	}
	switch (CVarBendsInteractionLookLock.GetValueOnGameThread())
	{
	case 1:  return EBendsLookLock::Lock;
	case 2:  return EBendsLookLock::Limit;
	default: return EBendsLookLock::Free;
	}
}

void UBendsInteractorComponent::ApplyInputLock(const UBendsInteractableComponent& Interactable)
{
	APlayerController* PC = GetPlayerController();
	if (!PC)
	{
		return;
	}
	LockedController = PC;

	if (Interactable.bLockMovement)
	{
		PC->SetIgnoreMoveInput(true);
		bMoveLocked = true;
	}

	const EBendsLookLock Look = ResolveLookLock(Interactable);
	if (Look == EBendsLookLock::Lock)
	{
		PC->SetIgnoreLookInput(true);
		bLookLocked = true;
	}
	else if (Look == EBendsLookLock::Limit && PC->PlayerCameraManager)
	{
		APlayerCameraManager* Camera = PC->PlayerCameraManager;
		SavedYawMin = Camera->ViewYawMin;
		SavedYawMax = Camera->ViewYawMax;
		SavedPitchMin = Camera->ViewPitchMin;
		SavedPitchMax = Camera->ViewPitchMax;

		const FRotator Rotation = PC->GetControlRotation();
		const float Pitch = FRotator::NormalizeAxis(Rotation.Pitch);

		Camera->ViewYawMin = Rotation.Yaw - LookLimitYaw;
		Camera->ViewYawMax = Rotation.Yaw + LookLimitYaw;
		Camera->ViewPitchMin = FMath::Max(SavedPitchMin, Pitch - LookLimitPitch);
		Camera->ViewPitchMax = FMath::Min(SavedPitchMax, Pitch + LookLimitPitch);

		LimitedCameraManager = Camera;
		bLookLimited = true;
	}
}

void UBendsInteractorComponent::ReleaseInputLock()
{
	// SetIgnore*Inputは回数で管理されるので、掛けた分だけ戻す
	if (APlayerController* PC = LockedController.Get())
	{
		if (bMoveLocked)
		{
			PC->SetIgnoreMoveInput(false);
		}
		if (bLookLocked)
		{
			PC->SetIgnoreLookInput(false);
		}
	}
	if (APlayerCameraManager* Camera = LimitedCameraManager.Get())
	{
		if (bLookLimited)
		{
			Camera->ViewYawMin = SavedYawMin;
			Camera->ViewYawMax = SavedYawMax;
			Camera->ViewPitchMin = SavedPitchMin;
			Camera->ViewPitchMax = SavedPitchMax;
		}
	}

	LockedController.Reset();
	LimitedCameraManager.Reset();
	bMoveLocked = false;
	bLookLocked = false;
	bLookLimited = false;
}
#pragma endregion

#pragma region Utility
APawn* UBendsInteractorComponent::GetPawn() const
{
	return Cast<APawn>(GetOwner());
}

APlayerController* UBendsInteractorComponent::GetPlayerController() const
{
	const APawn* Pawn = GetPawn();
	return Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
}

bool UBendsInteractorComponent::IsLocalPlayerPawn() const
{
	const APlayerController* PC = GetPlayerController();
	return PC && PC->IsLocalController();
}

void UBendsInteractorComponent::EnsurePromptWidget()
{
	if (PromptWidget || !PromptWidgetClass)
	{
		return;
	}
	if (APlayerController* PC = GetPlayerController())
	{
		PromptWidget = CreateWidget<UBendsInteractionPromptWidget>(PC, PromptWidgetClass);
		if (PromptWidget)
		{
			PromptWidget->SetInteractor(this);
			PromptWidget->AddToViewport();
		}
	}
}
#pragma endregion



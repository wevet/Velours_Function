// Copyright 2022 wevet works All Rights Reserved.


#include "Character/PlayerCharacter.h"
#include "Velours.h"
#include "WvPlayerController.h"
#include "Component/WvSpringArmComponent.h"
#include "Component/InventoryComponent.h"
#include "Component/HitTargetComponent.h"
#include "Component/WvSkeletalMeshComponent.h"
#include "Component/QTEActionComponent.h"
#include "Mission/MinimapMarkerComponent.h"
#include "GameExtension.h"
#include "Ability/WvAbilitySystemComponent.h"
#include "Component/WvFactionComponent.h"
#include "Component/WvRelationshipComponent.h"
#include "Component/LocomotionComponent.h"

#include "Item/BulletHoldWeaponActor.h"

// built in
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/PostProcessComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GroomComponent.h"
#include "GameFramework/GameplayCameraComponent.h"

// mover
#include "DefaultMovementSet/CharacterMoverComponent.h"
#include "MoveLibrary/MovementMixer.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(PlayerCharacter)

APlayerCharacter::APlayerCharacter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{

	MyTeamID = FGenericTeamId(1);
	CharacterTag = FGameplayTag::RequestGameplayTag(TAG_Character_Player.GetTag().GetTagName());

	QTEActionComponent = CreateDefaultSubobject<UQTEActionComponent>(TEXT("QTEActionComponent"));
	QTEActionComponent->bAutoActivate = 1;


	PostProcessBodyCamComponent = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcessBodyCam"));
	PostProcessBodyCamComponent->bAutoActivate = 0;
	PostProcessBodyCamComponent->SetupAttachment(SkeletalMeshComponent, NAME_None);
	PostProcessBodyCamComponent->SetVisibility(false);


	GameplayCameraComponent = CreateDefaultSubobject<UGameplayCameraComponent>(TEXT("GameplayCam"));
	GameplayCameraComponent->bAutoActivate = 0;
	GameplayCameraComponent->SetupAttachment(SkeletalMeshComponent, FName(TEXT("root")));
	GameplayCameraComponent->bRunInEditor = false;


#if WITH_EDITORONLY_DATA
	// https://forums.unrealengine.com/t/world-partion-current-pawn-vanishes-when-reaching-cell-loading-range-limit/255655/7
	bIsSpatiallyLoaded = false;
#endif

	if (WvRelationshipComponent)
	{
		WvRelationshipComponent->bRelationshipEnabled = false;
	}

	// set mini map tag
	MinimapMarkerComponent->MiniMapMakerTag = TAG_Game_Minimap_Player;
	MinimapMarkerComponent->SetVisibleMakerTag(false);

	AbilitySystemCreationPolicy = EAbilitySystemCreationPolicy::Always;
}

void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void APlayerCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);


}

void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{

	check(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{

		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Move);
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &APlayerCharacter::Move);
		}

		if (LookAction)
		{
			EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Look);
		}
	}
}

#if WITH_EDITOR
void APlayerCharacter::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
}
#endif

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	QTEActionComponent->AddTickPrerequisiteActor(this);

	CameraBoom = FindComponentByClass<UWvSpringArmComponent>();

	if (GameplayCameraComponent && SkeletalMeshComponent)
	{
		if (SkeletalMeshComponent->GetSkeletalMeshAsset())
		{
			const bool bIsAttachResult = GameplayCameraComponent->AttachToComponent(
				SkeletalMeshComponent,
				FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName(TEXT("root")));

			if (!bIsAttachResult)
			{
				UE_LOG(LogBaseCharacter, Error, TEXT("[%s] : GameplayCameraComponent attach failed"), *FString(__FUNCTION__));
			}
			else
			{
				UE_LOG(LogBaseCharacter, Verbose, TEXT("[%s] : GameplayCameraComponent attach success"), *FString(__FUNCTION__));
			}
		}

	}

	{
		auto Components = GetComponentsByTag(UCameraComponent::StaticClass(), TEXT("TPS Camera"));
		for (auto Comp : Components)
		{
			if (UCameraComponent* CamComp = Cast<UCameraComponent>(Comp))
			{
				TPSCamera = CamComp;
			}

		}
	}
	
	

	QTEActionComponent->QTEBeginDelegate.AddUniqueDynamic(this, &ThisClass::OnQTEBegin_Callback);
	QTEActionComponent->QTEEndDelegate.AddUniqueDynamic(this, &ThisClass::OnQTEEnd_Callback);

}

void APlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (AWvPlayerController* PC = Cast<AWvPlayerController>(Controller))
	{
		PC->OnInputEventGameplayTagTrigger_Game.AddUniqueDynamic(this, &ThisClass::GameplayTagTrigger_Callback);
		PC->OnPluralInputEventTrigger.AddUniqueDynamic(this, &ThisClass::OnPluralInputEventTrigger_Callback);
		PC->OnHoldingInputEventTrigger.AddUniqueDynamic(this, &ThisClass::OnHoldingInputEventTrigger_Callback);
		PC->OnDoubleClickInputEventTrigger.AddUniqueDynamic(this, &ThisClass::OnDoubleClickInputEventTrigger_Callback);

		PC->OnInputEventGameplayTagTrigger_UI.AddUniqueDynamic(this, &ThisClass::OnInputEventGameplayTagTrigger_UI_Callback);

		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			FModifyContextOptions Options;
			Subsystem->AddMappingContext(DefaultMappingContext, 0, Options);
		}
	}
}

void APlayerCharacter::UnPossessed()
{
	if (AWvPlayerController* PC = Cast<AWvPlayerController>(Controller))
	{
		PC->OnInputEventGameplayTagTrigger_Game.RemoveDynamic(this, &ThisClass::GameplayTagTrigger_Callback);
		PC->OnPluralInputEventTrigger.RemoveDynamic(this, &ThisClass::OnPluralInputEventTrigger_Callback);
		PC->OnHoldingInputEventTrigger.RemoveDynamic(this, &ThisClass::OnHoldingInputEventTrigger_Callback);
		PC->OnDoubleClickInputEventTrigger.RemoveDynamic(this, &ThisClass::OnDoubleClickInputEventTrigger_Callback);

		PC->OnInputEventGameplayTagTrigger_UI.RemoveDynamic(this, &ThisClass::OnInputEventGameplayTagTrigger_UI_Callback);

		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			FModifyContextOptions Options;
			Subsystem->RemoveMappingContext(DefaultMappingContext, Options);
		}
	}

	Super::UnPossessed();
}

void APlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{

	QTEActionComponent->QTEBeginDelegate.RemoveDynamic(this, &ThisClass::OnQTEBegin_Callback);
	QTEActionComponent->QTEEndDelegate.RemoveDynamic(this, &ThisClass::OnQTEEnd_Callback);

	Super::EndPlay(EndPlayReason);
}

void APlayerCharacter::NotifyControllerChanged()
{
	TryNotifyControllerAbilitySystemInitialized();
}

void APlayerCharacter::ProduceInput_Implementation(int32 SimTimeMs, FMoverInputCmdContext& InputCmdResult)
{

	//Super::ProduceInput(SimTimeMs, InputCmdResult);


	FCharacterDefaultInputs& Inputs = InputCmdResult.InputCollection.FindOrAddMutableDataByType<FCharacterDefaultInputs>();

	const bool bIsFPSControlMode = IsFPSControlMode();

	if (!bIsFPSControlMode)
	{
		const FRotator ControlRotation = GetControlRotation();
		const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);

		const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		const FVector MoveDirection = (Forward * MovementInput.Y + Right * MovementInput.X).GetClampedToMaxSize(1.0f);

		Inputs.SetMoveInput(EMoveInputType::DirectionalIntent, MoveDirection);
		Inputs.OrientationIntent = MoveDirection;
		Inputs.ControlRotation = ControlRotation;
		return;
	}


	const float DeltaSeconds = SimTimeMs * 0.001f;
	const float ControlYaw = Inputs.ControlRotation.Yaw;

	const FLocomotionEssencialVariables& LocomotionEssencialVariables = LocomotionComponent->GetLocomotionEssencialVariables();
	const bool IsMoving = CharacterMoverComponent->GetVelocity().Size2D() > 0.0f;

	// 据え置きは「静止 かつ Aim/Strafe」のときだけ。
	// 移動中に効かせるとロコモーションの旋回が粘って気持ち悪くなる。
	const bool bShouldHold = IsMoving && (LocomotionEssencialVariables.bAiming ||
		LocomotionEssencialVariables.LSRotationMode == ELSRotationMode::LookingDirection);

	if (bShouldHold)
	{
		const float Yaw = YawHold.Update(ControlYaw, DeltaSeconds);
		Inputs.OrientationIntent = FRotator(0.f, Yaw, 0.f).Vector();
	}
	else
	{
		// 移動中は Super が入れた OrientationIntent をそのまま使い、
		// 据え置き状態だけ現在値に合わせておく
		YawHold.SnapTo(ControlYaw);
	}

	// ABP 側が読む乖離量（S Mover CustomInputs に載せる）
	HeldYawDelta = YawHold.GetHeldDelta(ControlYaw);
}

void APlayerCharacter::PostAbilitySystemInitialize()
{
	TryNotifyControllerAbilitySystemInitialized();
	//UE_LOG(LogBaseCharacter, Log, TEXT("[%s]"), *FString(__FUNCTION__));
}

void APlayerCharacter::TryNotifyControllerAbilitySystemInitialized()
{
	if (AWvPlayerController* PC = Cast<AWvPlayerController>(Controller))
	{
		UE_LOG(LogBaseCharacter, Log, TEXT("[%s] : Success CtrlName => %s"), *FString(__FUNCTION__), *GetNameSafe(PC));
		PC->PostAbilitySystemInitialize(AbilitySystemComponent);

		P_Controller = PC;
	}
	else
	{
		if (IsValid(Controller))
		{
			UE_LOG(LogBaseCharacter, Error, TEXT("[%s] : Failed CtrlName => %s"), *FString(__FUNCTION__), *GetNameSafe(Controller));
		}
	}

}

bool APlayerCharacter::GetOverlayMenuOpen() const
{
	if (IsValid(P_Controller))
	{
		return P_Controller->GetOverlayMenuOpen();
	}
	return false;
}


#pragma region IWvKeyableTargetInterface
void APlayerCharacter::SetKeyInputEnable()
{
	UWvAbilitySystemComponent* ASC = GetWvAbilitySystemComponent();
	if (ASC)
	{
		ASC->RemoveGameplayTag(TAG_Game_Input_Disable, 1);
	}

}

void APlayerCharacter::SetKeyInputDisable()
{
	UWvAbilitySystemComponent* ASC = GetWvAbilitySystemComponent();
	if (ASC)
	{
		ASC->AddGameplayTag(TAG_Game_Input_Disable, 1);
	}
}

bool APlayerCharacter::IsInputKeyDisable() const
{
	UWvAbilitySystemComponent* ASC = GetWvAbilitySystemComponent();
	if (ASC)
	{
		return ASC->HasMatchingGameplayTag(TAG_Game_Input_Disable);
	}
	return false;
}
#pragma endregion


void APlayerCharacter::HandleJump(const bool bIsPress)
{
	//const auto CMC = GetWvCharacterMovementComponent();

	if (bIsPress)
	{
	}
	else
	{
	}
}

void APlayerCharacter::HandleSprinting(const bool bIsPress)
{
	if (bIsPress)
	{
	}
	else
	{
	}
}

void APlayerCharacter::HandleWalking(const bool bIsPress)
{
	if (bIsPress)
	{
	}
}

void APlayerCharacter::HandleMeleeAction(const bool bIsPress)
{
	// @TODO
	if (IsFPSControlMode())
	{
		return;
	}

	const UInventoryComponent* Inventory = GetInventoryComponent();

	if (bIsPress)
	{
		if (Super::IsMeleeAttacking() && !Inventory->CanAimingWeapon())
		{
			//const auto Tag = TAG_Character_StateMelee.GetTag().GetTagName();
			//UE_LOG(LogTemp, Warning, TEXT("Returns nothing as tags are added. %s => [%s]"), *Tag.ToString(), *FString(__FUNCTION__));
			return;
		}

		if (Inventory->CanAimingWeapon())
		{
			DoBulletAttack();
			return;
		}

		Super::DoAttack();
	}
	else
	{
		if (Inventory->CanAimingWeapon())
		{
			Clear_BulletTimer();
		}
	}
}

void APlayerCharacter::HandleDriveAction(const bool bIsPress)
{
	if (bIsPress)
	{
	}
}

void APlayerCharacter::HandleAliveAction(const bool bIsPress)
{
	if (bIsPress)
	{
	}
}

void APlayerCharacter::HandleHoldAimAction(const bool bIsPress)
{
	if (bIsPress)
	{
	}
	else
	{
	}
}

void APlayerCharacter::HandleQTEAction(const bool bIsPress)
{
	if (IsQTEActionPlaying())
	{
		if (bIsPress)
		{
			QTEActionComponent->InputPress();
		}

	}

}

bool APlayerCharacter::IsQTEActionPlaying() const
{
	const UWvAbilitySystemComponent* ASC = GetWvAbilitySystemComponent();
	return ASC && ASC->HasMatchingGameplayTag(TAG_Character_Action_QTE);
}


void APlayerCharacter::HandleFinisherAction(const FGameplayTag Tag, const bool bIsPress)
{
	if (bIsPress)
	{
		const bool bIsEquipBulletWeapon = GetInventoryComponent()->CanAimingWeapon();
		if (bIsEquipBulletWeapon)
		{
			UE_LOG(LogTemp, Warning, TEXT("Cannot be used if equipped with a pistol or rifle. => %s"), *FString(__FUNCTION__));
			return;
		}

		//BuildFinisherAbility(Tag);
	}
}

void APlayerCharacter::HandleStanceMode()
{
}

void APlayerCharacter::HandleTargetLock()
{
}

void APlayerCharacter::HandleRotationMode()
{
}

bool APlayerCharacter::HasFinisherAction(const FGameplayTag Tag) const
{
	FGameplayTagContainer Container;
	Container.AddTag(TAG_Weapon_Finisher);
	Container.AddTag(TAG_Weapon_HoldUp);
	Container.AddTag(TAG_Weapon_KnockOut);
	return Container.HasTag(Tag);
}

void APlayerCharacter::DoBulletAttack_Callback()
{
	const ABulletHoldWeaponActor* BulletWeapon = Cast<ABulletHoldWeaponActor>(GetInventoryComponent()->GetEquipWeapon());
	if (!BulletWeapon->IsAvailable())
	{
		Clear_BulletTimer();
		return;
	}
	//Super::DoBulletAttack();
}

void APlayerCharacter::Clear_BulletTimer()
{
	FTimerManager& TM = GetWorld()->GetTimerManager();
	if (TM.IsTimerActive(Bullet_TimerHandle))
	{
		TM.ClearTimer(Bullet_TimerHandle);
	}

}


void APlayerCharacter::GameplayTagTrigger_Callback(const FGameplayTag Tag, const bool bIsPress)
{
	if (IsInputKeyDisable())
	{
		return;
	}

	if (Tag == TAG_Character_ActionJump)
	{
		HandleJump(bIsPress);
	}
	else if (Tag == TAG_Character_ActionDash)
	{
		HandleSprinting(bIsPress);
	}
	else if (Tag == TAG_Character_ActionWalk)
	{
		HandleWalking(bIsPress);
	}
	else if (Tag == TAG_Character_ActionDrive)
	{
		HandleDriveAction(bIsPress);
	}
	else if (Tag == TAG_Character_Player_Melee)
	{
		HandleMeleeAction(bIsPress);
	}
	else if (Tag == TAG_Character_StateAlive_Trigger)
	{
		HandleAliveAction(bIsPress);
	}
	else if (Tag == TAG_Character_ActionAimChange)
	{
		HandleHoldAimAction(bIsPress);
	}

	if (bIsPress)
	{
		UE_LOG(LogBaseCharacter, Log, TEXT("[%s] : TagName => %s"), *FString(__FUNCTION__), *Tag.ToString());
	}

}

void APlayerCharacter::OnPluralInputEventTrigger_Callback(const FGameplayTag Tag, const bool bIsPress)
{
	if (IsInputKeyDisable())
	{
		return;
	}

	if (Tag == TAG_Character_ActionCrouch)
	{
		HandleStanceMode();
	}
	else if (Tag == TAG_Character_TargetLock)
	{
		HandleTargetLock();
	}
	else if (Tag == TAG_Character_ActionStrafeChange)
	{
		HandleRotationMode();
	}
	else if (Tag == TAG_Character_Action_QTE_Pressed)
	{
		HandleQTEAction(bIsPress);
	}
	else if (HasFinisherAction(Tag) && !Super::IsVehicleDriving())
	{
		HandleFinisherAction(Tag, bIsPress);
	}

	if (bIsPress)
	{
		UE_LOG(LogBaseCharacter, Log, TEXT("[%s] : TagName => %s"), *FString(__FUNCTION__), *Tag.ToString());
	}
}

void APlayerCharacter::OnHoldingInputEventTrigger_Callback(const FGameplayTag Tag, const bool bIsPress)
{
	if (IsInputKeyDisable())
	{
		return;
	}

	UE_LOG(LogBaseCharacter, Warning, TEXT("[%s]"), *FString(__FUNCTION__));
}

void APlayerCharacter::OnDoubleClickInputEventTrigger_Callback(const FGameplayTag Tag, const bool bIsPress)
{
	if (IsInputKeyDisable())
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[%s]"), *FString(__FUNCTION__));
}

void APlayerCharacter::OnInputEventGameplayTagTrigger_UI_Callback(const FGameplayTag Tag, const bool bIsPress)
{

	if (IsInputKeyDisable())
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[%s]"), *FString(__FUNCTION__));
}


void APlayerCharacter::OnQTEBegin_Callback()
{

}

void APlayerCharacter::OnQTEEnd_Callback(const bool bIsSuccess)
{


}

void APlayerCharacter::NotifyRegisterMission(const int32 MissionIndex)
{
}

#pragma region InputAction

void APlayerCharacter::Move(const FInputActionValue& Value)
{

	if (IsInputKeyDisable())
	{
		MovementInput = FVector2D::ZeroVector;
		return;
	}

	MovementInput = Value.Get<FVector2D>();
}

void APlayerCharacter::Look(const FInputActionValue& Value)
{
	if (IsInputKeyDisable())
	{
		return;
	}

	FVector2D LookAxisVector = Value.Get<FVector2D>();

	const float LocalX = LookAxisVector.X * MouseSensitibity;
	const float LocalY = LookAxisVector.Y * MouseSensitibity;

	AddControllerYawInput(LocalX);
	AddControllerPitchInput(bIsInvertMouse ? LocalY * -1.0f : LocalY);

	MoveCallback(LookAxisVector);
}


#pragma endregion

void APlayerCharacter::SetFPSMode(const bool bIsEnableFPS)
{
	UWvAbilitySystemComponent* ASC = GetWvAbilitySystemComponent();
	if (ASC)
	{
		if (bIsEnableFPS)
		{
			ASC->AddGameplayTag(TAG_Game_FPS);
			ASC->RemoveGameplayTag(TAG_Game_TPS);

			PostProcessBodyCamComponent->SetVisibility(true);
		}
		else
		{
			ASC->AddGameplayTag(TAG_Game_TPS);
			ASC->RemoveGameplayTag(TAG_Game_FPS);

			PostProcessBodyCamComponent->SetVisibility(false);
		}
	}

	if (FPSChangeDelegate.IsBound())
	{
		FPSChangeDelegate.Broadcast(bIsEnableFPS);
	}

}

bool APlayerCharacter::IsFPSControlMode() const
{
	UWvAbilitySystemComponent* ASC = GetWvAbilitySystemComponent();
	if (ASC)
	{
		return ASC->HasMatchingGameplayTag(TAG_Game_FPS);
	}
	return false;
}



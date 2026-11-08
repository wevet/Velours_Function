// Copyright 2022 wevet works All Rights Reserved.

#include "Component/VFirstPersonMotionComponent.h"
#include "Game/BendsViewMode.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Engine.h"
#include "HAL/IConsoleManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(VFirstPersonMotionComponent)

namespace FirstPersonMotionCommand
{
	TAutoConsoleVariable<bool> CVarFPMotionEnable(
		TEXT("bends.FPMotion.Enable"), true,
		TEXT("Enable the first-person aim/view offset."), ECVF_Default);

	TAutoConsoleVariable<float> CVarFPMotionLagScale(
		TEXT("bends.FPMotion.LagScale"), 1.f,
		TEXT("Multiplier for the view smoothing time. 0 = no lag."), ECVF_Default);

	TAutoConsoleVariable<float> CVarFPMotionDeadZoneScale(
		TEXT("bends.FPMotion.DeadZoneScale"), 1.f,
		TEXT("Multiplier for the aim dead zone."), ECVF_Default);

	TAutoConsoleVariable<bool> CVarFPMotionDebug(
		TEXT("bends.FPMotion.Debug"), false,
		TEXT("Show the aim/view offset on screen."), ECVF_Cheat);
}

UVFirstPersonMotionComponent::UVFirstPersonMotionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	// アニメーション（頭のソケットの位置）が決まった後、カメラの更新より前に動く
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;

	// 構えているときは、ずれを小さく、追いつきを速く
	AimingSettings.DeadZoneYaw = 0.5f;
	AimingSettings.DeadZonePitch = 0.3f;
	AimingSettings.SmoothTime = 0.04f;
	AimingSettings.MaxOffsetYaw = 2.f;
	AimingSettings.MaxOffsetPitch = 1.5f;
}

void UVFirstPersonMotionComponent::BeginPlay()
{
	Super::BeginPlay();
	ResetView();
}

void UVFirstPersonMotionComponent::SetCamera(UCameraComponent* InCamera)
{
	Camera = InCamera;
	if (InCamera)
	{
		// 回転はこのコンポーネントが決める。位置は付け先（頭のソケット）のまま
		InCamera->bUsePawnControlRotation = false;
		InCamera->SetUsingAbsoluteRotation(true);
	}
	ResetView();
}

void UVFirstPersonMotionComponent::ResetView()
{
	if (const APlayerController* PC = GetPlayerController())
	{
		AimRotation = PC->GetControlRotation();
	}
	ViewRotation = AimRotation;
	OffsetYaw = OffsetPitch = 0.f;
	OffsetYawVelocity = OffsetPitchVelocity = 0.f;

	AimingAlpha = bAiming ? 1.f : 0.f;

	ApplyToCamera();
	UpdateAimData();
}

FRotator UVFirstPersonMotionComponent::GetAimOffset() const
{
	// 狙い - 見る回転
	return FRotator(-OffsetPitch, -OffsetYaw, 0.f);
}

void UVFirstPersonMotionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const APlayerController* PC = GetPlayerController();
	const bool bFirstPerson = IsFirstPerson();
	if (!PC || !Camera.IsValid() || !bFirstPerson)
	{
		bWasFirstPerson = bFirstPerson;

		AimData.bActive = false;
		return;
	}

	// 一人称に入った瞬間は、ずれをなくしてから始める
	if (!bWasFirstPerson)
	{
		bWasFirstPerson = true;
		ResetView();
	}

	// 構えの切り替えを滑らかにする
	const float AimTarget = bAiming ? 1.f : 0.f;
	AimingAlpha = (AimingBlendTime > 0.f)
		? FMath::FInterpConstantTo(AimingAlpha, AimTarget, DeltaTime, 1.f / AimingBlendTime)
		: AimTarget;

	UpdateView(PC->GetControlRotation(), DeltaTime);
	ApplyToCamera();

	UpdateAimData();

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (FirstPersonMotionCommand::CVarFPMotionDebug.GetValueOnGameThread())
	{
		DrawDebug();
	}
#endif
}

void UVFirstPersonMotionComponent::UpdateView(const FRotator& Aim, float DeltaTime)
{
	const FRotator PreviousAim = AimRotation;
	AimRotation = Aim;

	if (!FirstPersonMotionCommand::CVarFPMotionEnable.GetValueOnGameThread())
	{
		OffsetYaw = OffsetPitch = 0.f;
		OffsetYawVelocity = OffsetPitchVelocity = 0.f;
		ViewRotation = AimRotation;
		return;
	}

	const FVFirstPersonViewSettings Settings = GetBlendedSettings();
	const float DeadZoneScale = FMath::Max(0.f, FirstPersonMotionCommand::CVarFPMotionDeadZoneScale.GetValueOnGameThread());
	const float SmoothTime = Settings.SmoothTime * FMath::Max(0.f, FirstPersonMotionCommand::CVarFPMotionLagScale.GetValueOnGameThread());

	// 狙いが動いた分だけ、見る回転との差が開く（見る回転はその場に残る）
	OffsetYaw -= FRotator::NormalizeAxis(AimRotation.Yaw - PreviousAim.Yaw);
	OffsetPitch -= FRotator::NormalizeAxis(AimRotation.Pitch - PreviousAim.Pitch);

	// 差の上限
	OffsetYaw = FMath::Clamp(OffsetYaw, -Settings.MaxOffsetYaw, Settings.MaxOffsetYaw);
	OffsetPitch = FMath::Clamp(OffsetPitch, -Settings.MaxOffsetPitch, Settings.MaxOffsetPitch);

	// デッドゾーンの外にはみ出た分だけを、バネで縮める。内側はそのまま残す
	auto ResolveAxis = [SmoothTime, DeltaTime](float& Offset, float& Velocity, float DeadZone)
		{
			const float Target = FMath::Clamp(Offset, -DeadZone, DeadZone);
			if (SmoothTime <= KINDA_SMALL_NUMBER)
			{
				Offset = Target;
				Velocity = 0.f;
				return;
			}
			CriticalSpring(Offset, Velocity, Target, SmoothTime, DeltaTime);
		};

	ResolveAxis(OffsetYaw, OffsetYawVelocity, Settings.DeadZoneYaw * DeadZoneScale);
	ResolveAxis(OffsetPitch, OffsetPitchVelocity, Settings.DeadZonePitch * DeadZoneScale);

	ViewRotation = FRotator(AimRotation.Pitch + OffsetPitch, AimRotation.Yaw + OffsetYaw, 0.f);
}

void UVFirstPersonMotionComponent::ApplyToCamera() const
{
	if (UCameraComponent* Cam = Camera.Get())
	{
		Cam->SetWorldRotation(ViewRotation);
	}
}

FVFirstPersonViewSettings UVFirstPersonMotionComponent::GetBlendedSettings() const
{
	FVFirstPersonViewSettings Result;
	const float A = AimingAlpha;
	Result.DeadZoneYaw = FMath::Lerp(DefaultSettings.DeadZoneYaw, AimingSettings.DeadZoneYaw, A);
	Result.DeadZonePitch = FMath::Lerp(DefaultSettings.DeadZonePitch, AimingSettings.DeadZonePitch, A);
	Result.SmoothTime = FMath::Lerp(DefaultSettings.SmoothTime, AimingSettings.SmoothTime, A);
	Result.MaxOffsetYaw = FMath::Lerp(DefaultSettings.MaxOffsetYaw, AimingSettings.MaxOffsetYaw, A);
	Result.MaxOffsetPitch = FMath::Lerp(DefaultSettings.MaxOffsetPitch, AimingSettings.MaxOffsetPitch, A);
	return Result;
}

void UVFirstPersonMotionComponent::CriticalSpring(float& Value, float& Velocity, float Target, float SmoothTime, float DeltaTime)
{
	// 臨界減衰のバネの近似（行き過ぎずに目標へ近づく）
	const float Omega = 2.f / SmoothTime;
	const float X = Omega * DeltaTime;
	const float Exp = 1.f / (1.f + X + 0.48f * X * X + 0.235f * X * X * X);
	const float Change = Value - Target;
	const float Temp = (Velocity + Omega * Change) * DeltaTime;
	Velocity = (Velocity - Omega * Temp) * Exp;
	Value = Target + (Change + Temp) * Exp;
}

APlayerController* UVFirstPersonMotionComponent::GetPlayerController() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	return Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
}

bool UVFirstPersonMotionComponent::IsFirstPerson() const
{
	return IBendsViewModeProvider::GetViewModeOf(GetOwner()) == EBendsViewMode::FirstPerson;
}

void UVFirstPersonMotionComponent::DrawDebug() const
{
	if (GEngine)
	{
		const FRotator Offset = GetAimOffset();
		GEngine->AddOnScreenDebugMessage(reinterpret_cast<uint64>(this), 0.f, FColor::Cyan,
			FString::Printf(TEXT("FPMotion AimOffset Yaw=%.2f Pitch=%.2f Aiming=%.2f"), Offset.Yaw, Offset.Pitch, AimingAlpha));
	}
}


void UVFirstPersonMotionComponent::UpdateAimData()
{
	const UCameraComponent* Cam = Camera.Get();
	if (!Cam)
	{
		AimData.bActive = false;
		return;
	}

	// 起点はカメラの位置（頭のソケット）
	const FVector Origin = Cam->GetComponentLocation();

	const FQuat ViewQuat = ViewRotation.Quaternion();
	const FQuat AimQuat = AimRotation.Quaternion();

	AimData.bActive = true;
	AimData.AimOffset = GetAimOffset();

	AimData.HeadLookAtTarget = FTransform(ViewQuat, Origin + ViewQuat.GetForwardVector() * HeadTargetDistance);

	const FVector AimPoint = Origin + AimQuat.GetForwardVector() * ArmTargetDistance;
	AimData.RightArmTarget = FTransform(AimQuat, AimPoint + AimQuat.RotateVector(RightArmTargetOffset));
	AimData.LeftArmTarget = FTransform(AimQuat, AimPoint + AimQuat.RotateVector(LeftArmTargetOffset));
}


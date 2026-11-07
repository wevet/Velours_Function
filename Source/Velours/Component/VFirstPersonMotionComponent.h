// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VFirstPersonMotionComponent.generated.h"

class UCameraComponent;
class APlayerController;


USTRUCT(BlueprintType)
struct VELOURS_API FVFirstPersonAimData
{
	GENERATED_BODY()

	// 一人称で動いているか。falseのときはソルバーを切る
	UPROPERTY(BlueprintReadOnly, Category = "FirstPersonAim")
	bool bActive = false;

	// 頭と上の背骨が向く先（見る方向）。CustomAimSolverのLookAtLocationへ
	UPROPERTY(BlueprintReadOnly, Category = "FirstPersonAim")
	FTransform HeadLookAtTarget = FTransform::Identity;

	// 腕が向く先（狙いの方向）。ArmTargetLocationOverridesの右、左へ
	UPROPERTY(BlueprintReadOnly, Category = "FirstPersonAim")
	FTransform RightArmTarget = FTransform::Identity;

	UPROPERTY(BlueprintReadOnly, Category = "FirstPersonAim")
	FTransform LeftArmTarget = FTransform::Identity;

	// 狙い - 見る回転（度）。腕の揺れや、UIの照準のずれに使う
	UPROPERTY(BlueprintReadOnly, Category = "FirstPersonAim")
	FRotator AimOffset = FRotator::ZeroRotator;
};

/**
* 見る回転の追いかけ方
*/
USTRUCT(BlueprintType)
struct VELOURS_API FVFirstPersonViewSettings
{
	GENERATED_BODY()

	// 狙いの変化を無視する角度（度）。この内側では、カメラは動かず狙いだけが動く
	UPROPERTY(EditAnywhere, Category = "View", meta = (ClampMin = 0.0, ClampMax = 30.0))
	float DeadZoneYaw = 2.5f;

	UPROPERTY(EditAnywhere, Category = "View", meta = (ClampMin = 0.0, ClampMax = 30.0))
	float DeadZonePitch = 1.5f;

	// 追いつくまでのおおよその時間（秒）。0で遅れなし
	UPROPERTY(EditAnywhere, Category = "View", meta = (ClampMin = 0.0, ClampMax = 1.0))
	float SmoothTime = 0.08f;

	// 狙いとカメラの差の上限（度）。素早く振り向いても、照準が画面から外れないように
	UPROPERTY(EditAnywhere, Category = "View", meta = (ClampMin = 0.0, ClampMax = 45.0))
	float MaxOffsetYaw = 8.f;

	UPROPERTY(EditAnywhere, Category = "View", meta = (ClampMin = 0.0, ClampMax = 45.0))
	float MaxOffsetPitch = 5.f;
};

/**
 * A component that creates a “discrepancy between aim and viewpoint” in first-person view (Unrecord-style free aiming).
 *
 * Aim rotation … Controller rotation (direct mouse input). The direction the body and arms are facing
 * View rotation … The aim rotation is followed by a spring mechanism, leaving a dead zone. The camera’s orientation
 *
 * The camera’s position remains tied to the head socket; this component determines only the rotation
 * (Set the camera to `bUsePawnControlRotation=false` and use absolute rotation).
 * Runs after animation updates (`TG_PostUpdateWork`) and sets the rotation before the camera is updated.
 *
 * Only activates in first-person view (when IBendsViewModeProvider is set to FirstPerson).
 * When aiming (SetAiming), switch to a different configuration.
 *
 * CVar:
 *   bends.FPMotion.Enable    0/1 … If disabled, the camera directly follows the aiming rotation
 *   bends.FPMotion.LagScale  … Multiplier applied to SmoothTime (0 means no lag)
 *   bends.FPMotion.DeadZoneScale … Multiplier applied to the dead zone
 *   bends.FPMotion.Debug     0/1 … Displays the difference between the aim and the camera on screen
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class VELOURS_API UVFirstPersonMotionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UVFirstPersonMotionComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 回転を設定するカメラ（一人称のカメラ）
	void SetCamera(UCameraComponent* InCamera);

	// 構えの状態。構えている間はAimingSettingsを使う
	void SetAiming(bool bInAiming) { bAiming = bInAiming; }

	// 見る回転を狙いの回転に揃える（一人称への切り替え、テレポート、カットシーンの後など）
	void ResetView();

	FRotator GetAimRotation() const { return AimRotation; }
	FRotator GetViewRotation() const { return ViewRotation; }

	// 狙いが見る回転からどれだけずれているか（Yaw、Pitch。度）。段階2で腕の目標に使う
	FRotator GetAimOffset() const;

	const FVFirstPersonAimData& GetAimData() const { return AimData; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "FirstPersonMotion")
	FVFirstPersonViewSettings DefaultSettings;

	UPROPERTY(EditAnywhere, Category = "FirstPersonMotion")
	FVFirstPersonViewSettings AimingSettings;

	// 構えの切り替えにかける時間（秒）
	UPROPERTY(EditAnywhere, Category = "FirstPersonMotion", meta = (ClampMin = 0.0))
	float AimingBlendTime = 0.15f;

	// 頭の目標までの距離（cm）。遠いほど、頭の向きが距離の変化に鈍くなる
	UPROPERTY(EditAnywhere, Category = "FirstPersonMotion|Target", meta = (ClampMin = 10.0))
	float HeadTargetDistance = 500.f;

	// 腕の目標までの距離（cm）
	UPROPERTY(EditAnywhere, Category = "FirstPersonMotion|Target", meta = (ClampMin = 10.0))
	float ArmTargetDistance = 500.f;

	// 腕の目標のずれ（狙いの向きの空間、cm）。両手が交差するのを避けたいときに使う
	UPROPERTY(EditAnywhere, Category = "FirstPersonMotion|Target")
	FVector RightArmTargetOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "FirstPersonMotion|Target")
	FVector LeftArmTargetOffset = FVector::ZeroVector;

private:
	APlayerController* GetPlayerController() const;
	bool IsFirstPerson() const;

	FVFirstPersonViewSettings GetBlendedSettings() const;
	void UpdateView(const FRotator& Aim, float DeltaTime);
	void ApplyToCamera() const;
	void DrawDebug() const;

	// 臨界減衰のバネ（行き過ぎない）
	static void CriticalSpring(float& Value, float& Velocity, float Target, float SmoothTime, float DeltaTime);

	TWeakObjectPtr<UCameraComponent> Camera;

	FRotator AimRotation = FRotator::ZeroRotator;

	// 見る回転は、角度の折り返しを気にしないように、狙いとの差で持つ
	FRotator ViewRotation = FRotator::ZeroRotator;
	float OffsetYaw = 0.f;    // 見る回転 - 狙いの回転
	float OffsetPitch = 0.f;
	float OffsetYawVelocity = 0.f;
	float OffsetPitchVelocity = 0.f;

	float AimingAlpha = 0.f;
	bool bAiming = false;
	bool bWasFirstPerson = false;

	void UpdateAimData();

	FVFirstPersonAimData AimData;
};


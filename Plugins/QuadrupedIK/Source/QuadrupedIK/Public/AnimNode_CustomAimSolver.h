// Copyright 2022 wevet works All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "CustomIKData.h"
#include "AnimNode_CustomIKControlBase.h"
#include "AnimNode_CustomAimSolver.generated.h"

class FPrimitiveDrawInterface;
class USkeletalMeshComponent;


UENUM(BlueprintType)
enum class EAimSolverType : uint8
{
	LookAt,
	HandAim,
	CCD,
	//Tail,
};


USTRUCT(BlueprintType)
struct FAimJointRotationOverride
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRotator Clamp = FRotator(0.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Multiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float UpRotClamp = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bOverrideClamp = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bOverrideMultiplier = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bOverrideUpRotClamp = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HingeAxis")
	bool bOverrideHingeAxis = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HingeAxis", meta = (EditCondition = "bOverrideHingeAxis"))
	FVector HingeAxisLS = FVector(1.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HingeAxis")
	bool bUseHingeAngleLimit = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HingeAxis", meta = (EditCondition = "bUseHingeAngleLimit"))
	FVector2D HingeAngleLimit = FVector2D(-90.0f, 90.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HingeAxis")
	bool bLockJoint = false;
};

USTRUCT(BlueprintType)
struct FAimRobotArmSettings
{
	GENERATED_BODY()

	/** ON でロボットアーム用のヒンジ拘束 CCD に切り替わる */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEnable = false;

	/** PerJoint 指定が無い関節に使う共通ヒンジ軸(ボーンローカル) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bEnable"))
	FVector DefaultHingeAxisLS = FVector(1.0f, 0.0f, 0.0f);

	/** 1イテレーションあたりの最大回転量。振動抑制 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bEnable", ClampMin = "1.0", ClampMax = "180.0"))
	float MaxAnglePerIterationDeg = 30.0f;
};


USTRUCT()
struct QUADRUPEDIK_API FAnimNode_CustomAimSolver : public FAnimNode_CustomIKControlBase
{
	GENERATED_BODY()



public:

#pragma region InputData
	UPROPERTY(EditAnywhere, Category = InputData, meta = (PinHiddenByDefault))
	FBoneReference EndSplineBone;

	UPROPERTY(EditAnywhere, Category = InputData, meta = (PinHiddenByDefault))
	FBoneReference StartSplineBone;

	UPROPERTY(EditAnywhere, Category = InputData, meta = (PinShownByDefault))
	FTransform LookAtLocation{ FTransform::Identity };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InputData, meta = (DisplayName = "Hands Input (optional)", PinHiddenByDefault))
	TArray<FCustomBone_ArmsData> AimingHandLimbs;
#pragma endregion


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ReachingAndSeparateAim, meta = (PinHiddenByDefault))
	bool bIsNsewPoleMethod = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ReachingAndSeparateAim, meta = (DisplayName = "Reach instead of aiming ? (Only for arms)", PinHiddenByDefault))
	bool bIsReachInstead = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ReachingAndSeparateAim, meta = (DisplayName = "Hand aiming/reaching use the override target transforms ?", PinHiddenByDefault))
	bool bIsUseSeparateTargets = false;


#pragma region Debug
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug)
	FTransform DebugLookAtTransform{ FTransform::Identity };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, EditFixedSize, Category = Debug)
	TArray<FTransform> DebugHandTransforms;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug)
	bool bIsHeadDrawGizmos{ false };

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug)
	bool bDisplayHorizontalAngleDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug, meta = (ClampMin = "8", ClampMax = "128", PinHiddenByDefault))
	int32 HorizontalDebugArcSegments = 32;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug, meta = (ClampMin = "10.0", PinHiddenByDefault))
	float HorizontalDebugArcRadius = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug)
	FColor HorizontalDebugRawColor = FColor::Yellow;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug)
	FColor HorizontalDebugClampedColor = FColor::Green;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug)
	FColor HorizontalDebugForwardColor = FColor::Cyan;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug)
	FColor HorizontalDebugArcColor = FColor::Orange;
#pragma endregion


	UPROPERTY(Transient)
	TArray<FTransform> ElbowBoneTransformArray = TArray<FTransform>();


	UPROPERTY(Transient)
	TArray<FTransform> HandDefaultTransformArray = TArray<FTransform>();
	FTransform HeadOrigTransform{ FTransform::Identity };

	FTransform OriginTransform{ FTransform::Identity };
	bool bIsFocusDebugtarget = true;

	TArray<FBoneTransform> HeadTransforms;

protected:
#pragma region BasicSettings

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (PinHiddenByDefault))
	EAimSolverType AimSolverType{ EAimSolverType::LookAt };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (PinHiddenByDefault))
	bool bIsUseNaturalMethod = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (PinHiddenByDefault))
	bool bIsHeadUseSeparateClamp = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (PinHiddenByDefault))
	bool bIsOverrideHeadRotation = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (PinHiddenByDefault, EditCondition = "bIsOverrideHeadRotation"))
	float HeadEndLookAtClamp = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (PinHiddenByDefault, DisplayName = "TipBone Direction", EditCondition = "bIsOverrideHeadRotation"))
	FVector CCDTipBoneDirectionVector = FVector(0.0f, 1.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (PinShownByDefault))
	bool bIsEnableSolver = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (PinHiddenByDefault, EditCondition = "AimSolverType == EAimSolverType::CCD"))
	float Precision{ 1.0 };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (PinHiddenByDefault, EditCondition = "AimSolverType == EAimSolverType::CCD"))
	int32 MaxIteration{ 15 };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (PinHiddenByDefault))
	bool bIsEnableInterpolation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (PinHiddenByDefault, EditCondition = "bIsEnableInterpolation"))
	EIKInterpLocationType InterpLocationType = EIKInterpLocationType::DivisiveLocation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (PinHiddenByDefault, EditCondition = "bIsEnableInterpolation"))
	EIKInterpRotationType RotationInterpType = EIKInterpRotationType::DivisiveRotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (PinHiddenByDefault, EditCondition = "bIsEnableInterpolation"))
	float InterpolationSpeed = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (PinHiddenByDefault, DisplayName = "Character UpVector"))
	FVector CharacterDirectionVectorCS = FVector(0.0f, 0.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (PinHiddenByDefault, DisplayName = "Character ForwardDirection"))
	FVector ForwardDirectionVector = FVector(0.0f, 1.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (DisplayName = "Use Reference Forward Axis Logic", PinHiddenByDefault))
	bool bIsUseReferenceForwardAxis = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (DisplayName = "Reference Forward Axis", PinHiddenByDefault))
	FVector ReferenceConstantForwardAxis = FVector(0, 1, 0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (DisplayName = "Use Middle joint Axis", PinHiddenByDefault))
	bool bUseSpecificIntermediateAxis = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (DisplayName = "Middle joint Axis", PinHiddenByDefault, EditCondition = "bUseSpecificIntermediateAxis"))
	FAimBoneAxisSetting IntermediateLookAtAxis;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (DisplayName = "Work outside gameplay (For Sequencer)", PinShownByDefault))
	bool bIsWorkOutsidePIE = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (PinHiddenByDefault))
	bool bDisplayLineTrace{ false };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Smoothing, meta = (PinHiddenByDefault))
	bool bIsAllowSmoothingEnableSolver = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Smoothing, meta = (PinHiddenByDefault))
	float FinalAlphaInterpSpeed = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Smoothing, meta = (PinHiddenByDefault))
	float SolverDisableCompleteThreshold = 0.01f;

	bool bIsPrevEnableSolver = true;
	bool bSolverFadeOutOnly = false;
	float SolverBlendAlphaSmoothed = 0.0f;
#pragma endregion


#pragma region AlphaSettings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AlphaSettings, meta = (PinHiddenByDefault))
	bool bUseAnimCurveAlpha = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AlphaSettings, meta = (PinHiddenByDefault, EditCondition = "bUseAnimCurveAlpha"))
	FName IKAlphaCurveName = TEXT("AimIKAlpha");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AlphaSettings, meta = (PinHiddenByDefault))
	bool bUseDistanceAlpha = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AlphaSettings, meta = (PinHiddenByDefault, EditCondition = "bUseDistanceAlpha"))
	FName DistanceCheckBoneName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AlphaSettings, meta = (PinHiddenByDefault, EditCondition = "bUseDistanceAlpha"))
	FRuntimeFloatCurve DistanceAlphaCurve;
#pragma endregion


#pragma region ReachingAndSeparateAim
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ReachingAndSeparateAim, meta = (DisplayName = "Overrided Arm Aim Target Array", PinHiddenByDefault))
	FCustomBone_Overrided_Location_Data ArmTargetLocationOverrides;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ReachingAndSeparateAim, meta = (PinHiddenByDefault))
	EArmTwistIKType ArmTwistAxis = EArmTwistIKType::PoseAxisTwist;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ReachingAndSeparateAim, meta = (PinHiddenByDefault))
	bool bIsOverrideHandRotation = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ReachingAndSeparateAim, meta = (DisplayName = "If multi-arm aiming", PinHiddenByDefault))
	bool bIsAggregateHandBody = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ReachingAndSeparateAim, meta = (DisplayName = "If separate reaching mode", PinHiddenByDefault))
	bool bIsLetArmTwistWithHand = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ReachingAndSeparateAim, meta = (PinHiddenByDefault))
	bool bIsEnableHandInterpolation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ReachingAndSeparateAim, meta = (PinHiddenByDefault))
	bool bIsIgnoreElbowModification = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ReachingAndSeparateAim, meta = (PinHiddenByDefault))
	bool bIsIgnoreSeparateHandSolving = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ReachingAndSeparateAim, meta = (PinHiddenByDefault, EditCondition = "AimSolverType != EAimSolverType::CCD"))
	float LimbsClamp = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ReachingAndSeparateAim, meta = (EditCondition = "bIsEnableHandInterpolation", PinHiddenByDefault))
	float HandInterpolationSpeed = 10.0f;

	/// <summary>
	/// if -1 to automation handjoint
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ReachingAndSeparateAim, meta = (PinHiddenByDefault))
	int32 MainArmIndex = INDEX_NONE;
#pragma endregion


#pragma region ClampSettings
	/// <summary>
	/// use LookAt Only function
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ClampSettings, meta = (PinHiddenByDefault, EditCondition = "AimSolverType != EAimSolverType::CCD"))
	float LookAtRadius = 110.0f;

	/// <summary>
	/// use LookAt Only function
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ClampSettings, meta = (PinHiddenByDefault, EditCondition = "AimSolverType != EAimSolverType::CCD"))
	float LookAtClamp = 70.0f;

	/// <summary>
	/// use LookAt Only function
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ClampSettings, meta = (PinHiddenByDefault, EditCondition = "AimSolverType != EAimSolverType::CCD"))
	FRotator InnerBodyClamp = FRotator(0, 0, 0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ClampSettings, meta = (PinHiddenByDefault))
	FVector2D VerticalRangeAngles = FVector2D(-75, 75);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ClampSettings, meta = (PinHiddenByDefault))
	FVector2D HorizontalRangeAngles = FVector2D(-90, 90);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ClampSettings, meta = (PinHiddenByDefault))
	bool bUseNearMinDistanceClamp = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ClampSettings, meta = (PinHiddenByDefault))
	bool bIsNotLimitMethod{ false };

	/// <summary>
	/// if clamp position root position
	/// if true is etc em2400 HELI
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ClampSettings, meta = (PinHiddenByDefault))
	bool bUseRootAsClampOrigin{ false };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ClampSettings, meta = (PinHiddenByDefault, ClampMin = "0.0", EditCondition = "bUseNearMinDistanceClamp"))
	float NearMinDistance = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ClampSettings, meta = (PinHiddenByDefault, ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bUseNearMinDistanceClamp"))
	float NearMinDistanceForwardOnlyAlpha = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ClampSettings)
	bool bUsePerJointRotationOverrides = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ClampSettings, meta = (EditCondition = "bUsePerJointRotationOverrides"))
	TMap<FName, FAimJointRotationOverride> PerJointRotationOverrides;

	/// <summary>
	/// Set the value at the root end to a small number (e.g., around 0.05) and the value at the tip end to a large number (e.g., 1.0)
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ClampSettings, meta = (DisplayName = "Bone Clamp Curve (0 = End Bone, 1 = Head Bone)", PinHiddenByDefault))
	FRuntimeFloatCurve LookBendingCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ClampSettings, meta = (DisplayName = "Body Rotation Multiplier Curve (0 = End Bone, 1 = Head Bone)", PinHiddenByDefault))
	FRuntimeFloatCurve LookMultiplierCurve;
#pragma endregion


#pragma region MultiplierSettings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MultiplierSettings, meta = (PinHiddenByDefault))
	bool bIsEnableDipMultiplier{ true };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MultiplierSettings, meta = (PinHiddenByDefault, EditCondition = "bIsEnableDipMultiplier"))
	float DownwardDipMultiplier = -0.01f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MultiplierSettings, meta = (PinHiddenByDefault, EditCondition = "bIsEnableDipMultiplier"))
	float InvertedDipMultiplier = -0.01f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MultiplierSettings, meta = (PinHiddenByDefault, EditCondition = "bIsEnableDipMultiplier"))
	float VerticalDipTreshold = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MultiplierSettings, meta = (PinHiddenByDefault, EditCondition = "bIsEnableDipMultiplier"))
	float SideMoveMultiplier = 0.01f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MultiplierSettings, meta = (PinHiddenByDefault, EditCondition = "bIsEnableDipMultiplier"))
	float SideDownMultiplier = -0.01f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MultiplierSettings, meta = (PinHiddenByDefault, EditCondition = "bIsEnableDipMultiplier"))
	float UpRotClamp = 0.5f;
#pragma endregion



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CCD, meta = (PinHiddenByDefault, EditCondition = "AimSolverType == EAimSolverType::CCD"))
	FRotator BaseAngle = FRotator(0, 0, 0);

	/// <summary>
	/// @TODO
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CCD, meta = (PinHiddenByDefault, EditCondition = "AimSolverType == EAimSolverType::CCD"))
	FAimRobotArmSettings RobotArmSettings;

	/** CCD 後に先端ボーンの yaw を Effector へ合わせる。四足の頭用。RobotArm では false */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CCD, meta = (PinHiddenByDefault, EditCondition = "AimSolverType == EAimSolverType::CCD"))
	bool bAlignTipYawToTarget = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CCD, meta = (PinHiddenByDefault, EditCondition = "AimSolverType == EAimSolverType::CCD"))
	bool bUseTipReferenceBone = false;

	UPROPERTY(EditAnywhere, Category = CCD, meta = (PinHiddenByDefault, EditCondition = "bUseTipReferenceBone && AimSolverType == EAimSolverType::CCD"))
	FBoneReference TipReferenceBone;



#pragma region Detection
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Detection, meta = (PinHiddenByDefault))
	bool bIsAdaptiveTerrainFollowBone{ false };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Detection, meta = (PinHiddenByDefault))
	bool bIsAdaptiveTerrain = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Detection, meta = (PinHiddenByDefault, EditCondition = "bIsAdaptiveTerrainFollowBone"))
	FName TraceJointName{ NAME_None };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Detection, meta = (PinHiddenByDefault, EditCondition = "bIsAdaptiveTerrainFollowBone"))
	FName TraceChildBoneName{ NAME_None };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Detection, meta = (PinHiddenByDefault, EditCondition = "bIsAdaptiveTerrainFollowBone"))
	float AdditionalOffsetHeight{ 0.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Detection, meta = (PinHiddenByDefault, EditCondition = "bIsAdaptiveTerrainFollowBone || bIsAdaptiveTerrain"))
	EIKRaycastType RaycastTraceType = EIKRaycastType::LineTrace;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Detection, meta = (PinHiddenByDefault, EditCondition = "bIsAdaptiveTerrainFollowBone || bIsAdaptiveTerrain"))
	TEnumAsByte<ETraceTypeQuery> TraceChannel = ETraceTypeQuery::TraceTypeQuery1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Detection, meta = (PinHiddenByDefault, EditCondition = "bIsAdaptiveTerrainFollowBone || bIsAdaptiveTerrain"))
	float Trace_Radius = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Detection, meta = (PinHiddenByDefault, EditCondition = "bIsAdaptiveTerrainFollowBone || bIsAdaptiveTerrain"))
	float TraceUpHeight = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Detection, meta = (PinHiddenByDefault, EditCondition = "bIsAdaptiveTerrainFollowBone || bIsAdaptiveTerrain"))
	float TraceDownHeight = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Detection, meta = (UIMin = 0.5f, UIMax = 0.9f, ClampMin = 0.5f, ClampMax = 0.9f, PinHiddenByDefault, EditCondition = "bIsAdaptiveTerrainFollowBone || bIsAdaptiveTerrain"))
	float NormalDotThreshold = 0.5f;
#pragma endregion


#pragma region Surface
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Surface, meta = (PinHiddenByDefault))
	bool bUseEndSurfaceNormal = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Surface, meta = (PinHiddenByDefault, EditCondition = "bUseEndSurfaceNormal"))
	float EndSurfaceTraceUpHeight = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Surface, meta = (PinHiddenByDefault, EditCondition = "bUseEndSurfaceNormal"))
	float EndSurfaceTraceDownHeight = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Surface, meta = (PinHiddenByDefault, EditCondition = "bUseEndSurfaceNormal"))
	float EndSurfaceNormalAlignAlpha = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Surface, meta = (PinHiddenByDefault, EditCondition = "bUseEndSurfaceNormal"))
	float EndSurfaceMinSlopeAngle = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Surface, meta = (PinHiddenByDefault, EditCondition = "bUseEndSurfaceNormal"))
	FVector EndSurfaceLocalForwardAxis = FVector::ForwardVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Surface, meta = (PinHiddenByDefault, EditCondition = "bUseEndSurfaceNormal"))
	FVector EndSurfaceLocalUpAxis = FVector::UpVector;
#pragma endregion



public:
	FAnimNode_CustomAimSolver();
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	virtual int32 GetLODThreshold() const override { return LODThreshold; }
	virtual void ConditionalDebugDraw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* PreviewSkelMeshComp) const;

#if WITH_EDITOR
	void ResizeDebugLocations(const int32 NewSize);
#endif


	EAimSolverType GetAimSolverType() const;

protected:
	virtual void UpdateInternal(const FAnimationUpdateContext& Context) override;
	virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;


private:


	const FVector GetCorrectedEffectorLocationFromTraceByName(
		FComponentSpacePoseContext& Output,
		const FName& TraceBoneName,
		const FName& ChildBoneName,
		const FVector& InEffectorWorldLocation,
		const float TraceUpOffset,
		const float TraceDownOffset,
		const float AdditionalOffsetZ);

	FQuat MakeSurfaceAlignedRotationPreserveForward(
		const FTransform& CurrentTransform,
		const FVector& SurfaceNormalWS,
		const FVector& LocalForwardAxis,
		const FVector& LocalUpAxis,
		const float InNormalAlpha) const;

	void ApplyLineTrace(
		const FAnimationUpdateContext& Context,
		const FVector& StartLocation,
		const FVector& EndLocation,
		FHitResult& OutHitResult,
		const FLinearColor& DebugColor,
		const bool bRenderTrace);


	const FBoneTransform LookAt_Processor(const FBoneContainer& RequiredBones, FComponentSpacePoseContext& Output, FCSPose<FCompactPose>& MeshBases, const FVector& OffsetVector, const int32 InIndex);

	void SolveAimIKSystem(FCSPose<FCompactPose>& MeshBases, FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms);

	FVector AnimationLocationLerp(const FVector& StartPosition, const FVector& EndPosition, const float DeltaSeconds) const;

	//void DrawHorizontalAngleDebugArc(USkeletalMeshComponent* MeshComp) const;

	void DrawHorizontalAngleDebugArc(
		USkeletalMeshComponent* MeshComp,
		FPrimitiveDrawInterface* PDI = nullptr,
		struct FAnimInstanceProxy* Proxy = nullptr) const;

private:


	bool bIsUpArmTwistTechnique = false;
	FTransform MainHandDefaultTransform{ FTransform::Identity };
	FTransform MainHandNewTransform{ FTransform::Identity };

	int32 NumValidSpines = 0;

	float ComponentScale = 1.0f;
	float HitResultHeight = 0.0f;

	UPROPERTY(Transient)
	FHitResult AimHitResult;

	UPROPERTY(Transient)
	FHitResult EndSurfaceHitResult;

	UPROPERTY(Transient)
	TArray<AActor*> IgnoreActors;


	float HeadActualAlpha = 1.0f;
	float AnimCurveAlpha = 1.0f;
	float DistanceAlpha = 1.0f;


	FVector LerpedLookatLocation = FVector::ZeroVector;

	FVector RefConstantForwardTemp = FVector(0, 1, 0);

	FTransform SavedLookAtTransform{ FTransform::Identity };
	FRotator LimbRotationOffset{ FRotator::ZeroRotator };

	bool bIsArmsEnable{ false };


	float CachedDeltaSeconds = 0.f;

	float SmoothFactor = 10.0f;
	bool bIsEveryFootDontHaveChild = false;

	bool bSolveShouldFail = false;
	bool bIsDebugHandsInitialized = false;


	UPROPERTY(Transient)
	TArray<float> LastShoulderAngles = TArray<float>();

	TArray<FBoneReference> HandBoneArray;
	TArray<FBoneReference> ElbowBoneArray;
	TArray<FBoneReference> ShoulderBoneArray;
	TArray<FBoneReference> ActualShoulderBoneArray;
	TArray<FArmSolverWorkArea> ArmSolverWorkArea;


	UPROPERTY(Transient)
	TArray<FVector> TraceStartList = TArray<FVector>();

	UPROPERTY(Transient)
	TArray<FVector> TraceEndList = TArray<FVector>();

	UPROPERTY(Transient)
	TArray<float> TraceRadiusList = TArray<float>();


	TArray<FBoneTransform> RestBoneTransforms;
	TArray<FBoneTransform> AnimatedBoneTransforms;
	TArray<FBoneTransform> Original_AnimatedBoneTransforms;
	TArray<FCompactPoseBoneIndex> CombinedIndices;


	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> owning_skel{ nullptr };


	// runtime cache
	FVector DebugClampOriginWS = FVector::ZeroVector;
	FVector DebugClampForwardWS = FVector::ForwardVector;
	FVector DebugClampRawTargetWS = FVector::ZeroVector;
	FVector DebugClampResultTargetWS = FVector::ZeroVector;

	float DebugHorizontalRawDeg = 0.0f;
	float DebugHorizontalClampedDeg = 0.0f;
	float DebugHorizontalClampMinDeg = 0.0f;
	float DebugHorizontalClampMaxDeg = 0.0f;

};


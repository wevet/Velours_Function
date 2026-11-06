// Copyright 2022 wevet works All Rights Reserved.

#include "AnimNode_CustomAimSolver.h"
#include "QuadrupedIKLibrary.h"
#include "QuadrupedIK.h"
#include "WvAnimCollisionQuery.h"

#include "Animation/AnimInstanceProxy.h"
#include "AnimationRuntime.h"
#include "Algo/Reverse.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/EngineTypes.h"



#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNode_CustomAimSolver)

DECLARE_CYCLE_STAT(TEXT("CustomAimSolver Eval"), STAT_CustomAimSolver_EvalSKelControl, STATGROUP_Anim);

namespace AimHelper
{
	const FName GetBoneNameFromCompactIndex(const FBoneContainer& BoneContainer, const FCompactPoseBoneIndex BoneIndex)
	{
		const int32 SkeletonIndex = BoneContainer.GetSkeletonIndex(BoneIndex);
		return BoneContainer.GetReferenceSkeleton().GetBoneName(SkeletonIndex);
	}

	FRotator GetHandYaw(
		const bool bIsHandArm,
		FCustomBone_ArmsData& HandData,
		const FTransform& Body_Transform,
		const FTransform& OrigArmTransform,
		const FTransform& OrigHandTransform,
		const FTransform& CurArmTransform,
		const FTransform& CurHandTransform,
		const FTransform& UnmodifiedHandTransform,
		const FVector& Up_Vector_CS)
	{
		FTransform Composed_Rot = FTransform::Identity;
		FTransform Arm_Forward_Point = CurHandTransform;
		Arm_Forward_Point.SetLocation(CurHandTransform.GetLocation() +
			(CurHandTransform.GetLocation() - CurArmTransform.GetLocation()).GetSafeNormal() * 1000);

		const FTransform Inverse_Point_Hand = CurHandTransform.Inverse() * UnmodifiedHandTransform;
		Arm_Forward_Point = Arm_Forward_Point * Inverse_Point_Hand;
		const FVector Arm_Hand_Dir = (CurHandTransform.GetLocation() - CurArmTransform.GetLocation()).GetSafeNormal();
		const FVector Hand_Point_Dir = (Arm_Forward_Point.GetLocation() - CurHandTransform.GetLocation()).GetSafeNormal();
		const FVector Arm_Hand_Cross_Dir = FVector::CrossProduct(Arm_Hand_Dir, Up_Vector_CS);

		const float DiffAngle_Forward = UKismetMathLibrary::DegAcos(FVector::DotProduct(Hand_Point_Dir, Arm_Hand_Dir));
		const float DiffAngle_Sideward = UKismetMathLibrary::DegAcos(FVector::DotProduct(Hand_Point_Dir, Up_Vector_CS));
		const float DirectionSign = FMath::Sign(FVector::CrossProduct(Hand_Point_Dir, Arm_Hand_Cross_Dir).Z);

		Composed_Rot.SetLocation(CurArmTransform.GetLocation());
		FTransform Current_Reference_Rot = FTransform::Identity;

		{
			Current_Reference_Rot.SetRotation(
				(CurHandTransform.GetLocation() - CurArmTransform.GetLocation()).GetSafeNormal().ToOrientationQuat());
		}

		FTransform Orig_Reference_Rot = FTransform::Identity;

		{
			Orig_Reference_Rot.SetRotation(
				(OrigHandTransform.GetLocation() - OrigArmTransform.GetLocation()).GetSafeNormal().ToOrientationQuat());
		}

		float Forward_Alpha = FMath::Clamp(DiffAngle_Forward, 90, 180);
		Forward_Alpha = UKismetMathLibrary::NormalizeToRange(Forward_Alpha, 90, 180);
		Forward_Alpha = Forward_Alpha > 0 ? 1.0f : 0.0f;

		FTransform OffsetTransformMod = FTransform::Identity;
		OffsetTransformMod.SetRotation(FRotator(0.0f, 0.0f, -180).Quaternion());

		FTransform Orig_Hand_Rel = OrigHandTransform * Orig_Reference_Rot.Inverse();
		FTransform Orig_Arm_Rel = OrigArmTransform * Orig_Reference_Rot.Inverse();

		FTransform Current_Hand_Rel = CurHandTransform * Current_Reference_Rot.Inverse();
		FTransform Current_Arm_Rel = CurArmTransform * Current_Reference_Rot.Inverse();

		Composed_Rot.SetRotation((Orig_Hand_Rel.Inverse() * Orig_Arm_Rel).GetRotation());
		Composed_Rot.SetRotation((Current_Hand_Rel.Inverse() * Current_Arm_Rel).GetRotation().Inverse() * Composed_Rot.GetRotation());

		float Composed_Roll = Composed_Rot.Rotator().Roll;
		float Angular_Offset1 = FMath::Clamp(DiffAngle_Sideward, 0.0f, 90.0f);
		Angular_Offset1 = UKismetMathLibrary::NormalizeToRange(Angular_Offset1, 0.0f, 90.0f);

		float Angular_Offset2 = FMath::Clamp(DiffAngle_Sideward, 90.0f, 180.0f);
		Angular_Offset2 = 1 - UKismetMathLibrary::NormalizeToRange(Angular_Offset2, 90.0f, 180.0f);
		float SideMultiplier = FMath::Min(Angular_Offset1, Angular_Offset2);

		if (DiffAngle_Forward < 90.0f)
		{
			//	Composed_Roll = Composed_Roll;
		}
		else
		{
			Composed_Roll = -Composed_Roll + HandData.TwistOffsetReverse;
		}

		if (bIsHandArm)
		{
			if ((DiffAngle_Forward < 85.0f || DiffAngle_Forward > 115.0f))
			{
				HandData.LastHandRotation.Roll = Composed_Roll;
			}
			else
			{
				return FRotator(0.0f, 0.0f, HandData.LastHandRotation.Roll);
			}
		}
		return FRotator(0.0f, 0.0f, Composed_Roll);
	}


	const FVector ClampRotateVector(
		const FVector& InputPosition,
		const FVector& ForwardDirection,
		const FVector& OriginLocation,
		const float V_ClampMin,
		const float V_ClampMax,
		const float H_ClampMin,
		const float H_ClampMax,
		const bool bIsUseNaturalRotations)
	{
		const float Magnitude = (OriginLocation - InputPosition).Size();
		const FVector Rot1_V = (ForwardDirection).GetSafeNormal();
		const FVector Rot2_V = (InputPosition - OriginLocation).GetSafeNormal();
		const FVector Rot3_V = Rot2_V;
		const float Degrees = UKismetMathLibrary::DegAcos(FVector::DotProduct(Rot1_V, Rot2_V));
		const float Degrees_Vertical = UKismetMathLibrary::DegAcos(FVector::DotProduct(Rot1_V, Rot3_V));

		const FVector Angle_Cross_Result = FVector::CrossProduct(Rot2_V, Rot1_V);
		const float Dir = FVector::DotProduct(Angle_Cross_Result, FVector::CrossProduct(FVector::UpVector, Rot1_V));
		const float Alpha_Dir_Vertical = (Dir / 2) + 0.5f;
		const float Degrees_Horizontal = UKismetMathLibrary::DegAcos(FVector::DotProduct(Rot1_V, Rot3_V));
		FVector Angle_Cross_Result_Horizontal = FVector::CrossProduct(Rot2_V, Rot1_V);
		float Dir_Horizontal = FVector::DotProduct(Angle_Cross_Result_Horizontal, FVector::UpVector);

		float Alpha_Dir_Horizontal = (Dir_Horizontal / 2) + 0.5f;
		float Max_Vertical_Angle = V_ClampMax;
		float Min_Vertical_Angle = V_ClampMin;
		Max_Vertical_Angle = bIsUseNaturalRotations ? Max_Vertical_Angle : FMath::Clamp(Max_Vertical_Angle, -85, 85);
		Min_Vertical_Angle = bIsUseNaturalRotations ? Min_Vertical_Angle : FMath::Clamp(Min_Vertical_Angle, -85, 85);

		const float Horizontal_Degree_Priority = (FMath::Lerp(
			FMath::Abs(H_ClampMin),
			FMath::Abs(H_ClampMax),
			FMath::Clamp(Alpha_Dir_Horizontal, 0.0f, 1.0f)));

		const float Vertical_Degree_Priority = (FMath::Lerp(
			FMath::Abs(Min_Vertical_Angle),
			FMath::Abs(Max_Vertical_Angle),
			FMath::Clamp(Alpha_Dir_Vertical, 0.0f, 1.0f)));

		const float Selected_Clamp_Value = FMath::Lerp(Vertical_Degree_Priority, Horizontal_Degree_Priority, FMath::Clamp(FMath::Abs(Dir_Horizontal), 0.0f, 1.0f));

		float Alpha = (Selected_Clamp_Value / (FMath::Max(Selected_Clamp_Value, Degrees)));
		Alpha = FMath::Clamp(Alpha, 0.0f, 1.0f);

		const FVector OutputRot = FMath::Lerp(Rot1_V, Rot2_V, Alpha);
		return (OriginLocation + (OutputRot.GetSafeNormal() * Magnitude));
	}


	const FVector ClampRotateVector_NoLimit(
		const FVector& InputPosition,
		const FVector& ForwardDirection,
		const FVector& OriginLocation,
		const float V_ClampMin,
		const float V_ClampMax,
		const float H_ClampMin,
		const float H_ClampMax,
		const bool bIsUseNaturalRotations)
	{
		const FVector ToTarget = InputPosition - OriginLocation;
		const float Magnitude = ToTarget.Size();

		if (Magnitude <= KINDA_SMALL_NUMBER)
		{
			return InputPosition;
		}

		const FVector Forward = ForwardDirection.GetSafeNormal();
		if (Forward.IsNearlyZero())
		{
			return InputPosition;
		}

		const FVector WorldUp = FVector::UpVector;

		// Forward から水平右方向を作る
		FVector Right = FVector::CrossProduct(WorldUp, Forward).GetSafeNormal();
		if (Right.IsNearlyZero())
		{
			// Forward がほぼ Up/Down のときの保険
			FVector FallbackUp = FVector::ForwardVector;
			Right = FVector::CrossProduct(FallbackUp, Forward).GetSafeNormal();
		}

		FVector Up = FVector::CrossProduct(Forward, Right).GetSafeNormal();
		if (Up.IsNearlyZero())
		{
			Up = WorldUp;
		}

		const FVector Dir = ToTarget / Magnitude;

		// ローカル基底上の成分
		const float X = FVector::DotProduct(Dir, Forward);
		const float Y = FVector::DotProduct(Dir, Right);
		const float Z = FVector::DotProduct(Dir, Up);

		// signed horizontal / vertical angles
		float HorizontalDeg = FMath::RadiansToDegrees(FMath::Atan2(Y, X));
		const float PlanarLen = FMath::Sqrt(FMath::Max(X * X + Y * Y, 0.0f));
		float VerticalDeg = FMath::RadiansToDegrees(FMath::Atan2(Z, PlanarLen));


		HorizontalDeg = FMath::ClampAngle(HorizontalDeg, H_ClampMin, H_ClampMax);
		VerticalDeg = FMath::ClampAngle(VerticalDeg, V_ClampMin, V_ClampMax);

		const float HorizontalRad = FMath::DegreesToRadians(HorizontalDeg);
		const float VerticalRad = FMath::DegreesToRadians(VerticalDeg);

		const float CosV = FMath::Cos(VerticalRad);
		const float SinV = FMath::Sin(VerticalRad);
		const float CosH = FMath::Cos(HorizontalRad);
		const float SinH = FMath::Sin(HorizontalRad);

		FVector ClampedDir = Forward * (CosV * CosH) + Right * (CosV * SinH) + Up * SinV;
		ClampedDir.Normalize();
		return OriginLocation + ClampedDir * Magnitude;
	}


	float GetSignedHorizontalAngleDeg(const FVector& ForwardDirection, const FVector& OriginLocation, const FVector& TargetLocation)
	{
		const FVector Forward = ForwardDirection.GetSafeNormal();
		if (Forward.IsNearlyZero())
		{
			return 0.0f;
		}

		const FVector WorldUp = FVector::UpVector;

		// 今の符号系に合わせて Forward x Up
		FVector Right = FVector::CrossProduct(Forward, WorldUp).GetSafeNormal();
		if (Right.IsNearlyZero())
		{
			Right = FVector::RightVector;
		}

		const FVector ToTarget = (TargetLocation - OriginLocation).GetSafeNormal();
		const FVector FlatDir = FVector::VectorPlaneProject(ToTarget, WorldUp).GetSafeNormal();
		if (FlatDir.IsNearlyZero())
		{
			return 0.0f;
		}

		const float X = FVector::DotProduct(FlatDir, Forward);
		const float Y = FVector::DotProduct(FlatDir, Right);

		return FMath::RadiansToDegrees(FMath::Atan2(Y, X));
	}


	FTransform SetArmYaw(
		const bool InvertTwist,
		const bool bIsRightHand,
		const float Roll,
		const FTransform& BodyTransform,
		const FTransform& OriginalArmTransform,
		const FTransform& OriginalHandTransform,
		const FTransform& CurrentArmTransform,
		const FTransform& CurrentHandTransform)
	{

		FTransform ComposedRot = CurrentArmTransform;
		FTransform ReferenceRot = FTransform::Identity;
		ReferenceRot.SetRotation((CurrentHandTransform.GetLocation() - CurrentArmTransform.GetLocation()).GetSafeNormal().ToOrientationQuat());
		ComposedRot.SetRotation(ReferenceRot.GetRotation().Inverse() * ComposedRot.GetRotation());

		int32 InvTwist = 1;

		if (InvertTwist)
		{
			InvTwist = -1;
		}
		ComposedRot.SetRotation(FRotator(0.0f, 0.0f, Roll * InvTwist).Quaternion() * ComposedRot.GetRotation());
		ComposedRot.SetRotation(ReferenceRot.GetRotation() * ComposedRot.GetRotation());
		return ComposedRot;
	}


	void Solve_Modified_TwoBoneIK(
		const FVector& RootPos,
		const FVector& JointPos,
		const FVector& EndPos,
		const FVector& JointTarget,
		const FVector& Effector,
		const FVector& ThighEffector,
		FVector& OutJointPos,
		FVector& OutEndPos,
		float UpperLimbLength,
		float LowerLimbLength,
		bool bAllowStretching,
		float StartStretchRatio,
		float MaxStretchScale)
	{
		// This is our reach goal.
		FVector DesiredPos = Effector;
		FVector DesiredDelta = DesiredPos - RootPos;
		float DesiredLength = DesiredDelta.Size();

		// Find lengths of upper and lower limb in the ref skeleton.
		// Use actual sizes instead of ref skeleton, so we take into account translation and scaling from other bone controllers.
		float MaxLimbLength = LowerLimbLength + UpperLimbLength;

		// Check to handle case where DesiredPos is the same as RootPos.
		FVector	DesiredDir;
		if (DesiredLength < KINDA_SMALL_NUMBER)
		{
			DesiredLength = KINDA_SMALL_NUMBER;
			DesiredDir = FVector(1.0f, 0.0f, 0.0f);
		}
		else
		{
			DesiredDir = DesiredDelta.GetSafeNormal();
		}

		// Get joint target (used for defining plane that joint should be in).
		FVector JointTargetDelta = JointTarget - RootPos;
		const float JointTargetLengthSqr = JointTargetDelta.SizeSquared();

		// Same check as above, to cover case when JointTarget position is the same as RootPos.
		FVector JointPlaneNormal, JointBendDir;
		if (JointTargetLengthSqr < FMath::Square(KINDA_SMALL_NUMBER))
		{
			JointBendDir = FVector(0, 1, 0);
			JointPlaneNormal = FVector(0, 0, 1);
		}
		else
		{
			JointPlaneNormal = DesiredDir ^ JointTargetDelta;
			// If we are trying to point the limb in the same direction that we are supposed to displace the joint in, 
			// we have to just pick 2 random vector perp to DesiredDir and each other.
			if (JointPlaneNormal.SizeSquared() < FMath::Square(KINDA_SMALL_NUMBER))
			{
				DesiredDir.FindBestAxisVectors(JointPlaneNormal, JointBendDir);
			}
			else
			{
				JointPlaneNormal.Normalize();
				// Find the final member of the reference frame by removing any component of JointTargetDelta along DesiredDir.
				// This should never leave a zero vector, because we've checked DesiredDir and JointTargetDelta are not parallel.
				JointBendDir = JointTargetDelta - ((JointTargetDelta | DesiredDir) * DesiredDir);
				JointBendDir.Normalize();
			}
		}

		if (bAllowStretching)
		{
			const float ScaleRange = MaxStretchScale - StartStretchRatio;
			if (ScaleRange > KINDA_SMALL_NUMBER && MaxLimbLength > KINDA_SMALL_NUMBER)
			{
				const float ReachRatio = DesiredLength / MaxLimbLength;
				const float ScalingFactor = (MaxStretchScale - 1.f) * FMath::Clamp((ReachRatio - StartStretchRatio) / ScaleRange, 0.f, 1.f);
				if (ScalingFactor > KINDA_SMALL_NUMBER)
				{
					LowerLimbLength *= (1.f + ScalingFactor);
					UpperLimbLength *= (1.f + ScalingFactor);
					MaxLimbLength *= (1.f + ScalingFactor);
				}
			}
		}

		OutEndPos = DesiredPos;
		OutJointPos = JointPos;

		// If we are trying to reach a goal beyond the length of the limb, clamp it to something solvable and extend limb fully.
		if (DesiredLength >= MaxLimbLength)
		{
			OutEndPos = RootPos + (MaxLimbLength * DesiredDir);
			OutJointPos = RootPos + (UpperLimbLength * DesiredDir);
		}
		else
		{
			// So we have a triangle we know the side lengths of. We can work out the angle between DesiredDir and the direction of the upper limb
			// using the sin rule:
			const float TwoAB = 2.f * UpperLimbLength * DesiredLength;

			const float CosAngle = (TwoAB != 0.f) ? ((UpperLimbLength * UpperLimbLength) + (DesiredLength * DesiredLength) - (LowerLimbLength * LowerLimbLength)) / TwoAB : 0.f;

			// If CosAngle is less than 0, the upper arm actually points the opposite way to DesiredDir, so we handle that.
			const bool bReverseUpperBone = (CosAngle < 0.f);

			// Angle between upper limb and DesiredDir
			// ACos clamps internally so we dont need to worry about out-of-range values here.
			const float Angle = FMath::Acos(CosAngle);

			// Now we calculate the distance of the joint from the root -> effector line.
			// This forms a right-angle triangle, with the upper limb as the hypotenuse.
			const float JointLineDist = UpperLimbLength * FMath::Sin(Angle);

			// And the final side of that triangle - distance along DesiredDir of perpendicular.
			// ProjJointDistSqr can't be neg, because JointLineDist must be <= UpperLimbLength because appSin(Angle) is <= 1.
			const float ProjJointDistSqr = (UpperLimbLength * UpperLimbLength) - (JointLineDist * JointLineDist);
			// although this shouldn't be ever negative, sometimes Xbox release produces -0.f, causing ProjJointDist to be NaN
			// so now I branch it. 						
			float ProjJointDist = (ProjJointDistSqr > 0.f) ? FMath::Sqrt(ProjJointDistSqr) : 0.f;
			if (bReverseUpperBone)
			{
				ProjJointDist *= -1.f;
			}
			// So now we can work out where to put the joint!
			OutJointPos = RootPos + (ProjJointDist * DesiredDir) + (JointLineDist * JointBendDir);
		}
	}


	void Solve_Modified_TwoBoneIK_3(
		FTransform& InOutRootTransform,
		FTransform& InOutJointTransform,
		FTransform& InOutEndTransform,
		const FVector& JointTarget,
		const FVector& Effector,
		const FVector& ThighEffector,
		float UpperLimbLength,
		float LowerLimbLength,
		bool bAllowStretching,
		float StartStretchRatio,
		float MaxStretchScale)
	{

		const FVector RootPos = InOutRootTransform.GetLocation();
		const FVector JointPos = InOutJointTransform.GetLocation();
		const FVector EndPos = InOutEndTransform.GetLocation();
		const FTransform Const_RootPos = InOutRootTransform;

		// IK solver
		FVector OutJointPos, OutEndPos;
		AimHelper::Solve_Modified_TwoBoneIK(
			RootPos,
			JointPos,
			EndPos,
			JointTarget,
			Effector,
			ThighEffector,
			OutJointPos,
			OutEndPos,
			UpperLimbLength,
			LowerLimbLength,
			bAllowStretching,
			StartStretchRatio,
			MaxStretchScale);

		// Update transform for upper bone.
		{
			// Get difference in direction for old and new joint orientations
			const FVector OldDir = (JointPos - RootPos).GetSafeNormal();
			const FVector NewDir = (OutJointPos - RootPos).GetSafeNormal();
			const FQuat DeltaRotation = FQuat::FindBetweenNormals(OldDir, NewDir);
			InOutRootTransform.SetTranslation(RootPos);
		}

		// update transform for middle bone
		{
			// Get difference in direction for old and new joint orientations
			const FVector OldDir = (RootPos - JointPos).GetSafeNormal();
			const FVector NewDir = (RootPos - OutJointPos).GetSafeNormal();
			const FQuat DeltaRotation = FQuat::FindBetweenNormals(OldDir, NewDir);
			InOutJointTransform.SetRotation(DeltaRotation * InOutJointTransform.GetRotation());
			InOutJointTransform.SetTranslation(OutJointPos);
		}

		// Update transform for end bone.
		// currently not doing anything to rotation
		// keeping input rotation
		// Set correct location for end bone.
		{
			const FVector OldDir = (EndPos - JointPos).GetSafeNormal();
			const FVector NewDir = (OutEndPos - OutJointPos).GetSafeNormal();
			const FQuat DeltaRotation = FQuat::FindBetweenNormals(OldDir, NewDir);
			InOutEndTransform.SetRotation(DeltaRotation * InOutEndTransform.GetRotation());
			InOutEndTransform.SetTranslation(OutEndPos);
		}
	}


	void Solve_Modified_TwoBoneIK_4(
		FTransform& InOutRootTransform,
		FTransform& InOutJointTransform,
		FTransform& InOutEndTransform,
		const FVector& JointTarget,
		const FVector& Effector,
		const FVector& ThighEffector,
		bool bAllowStretching,
		float StartStretchRatio,
		float MaxStretchScale)
	{
		const float LowerLimbLength = (InOutEndTransform.GetLocation() - InOutJointTransform.GetLocation()).Size();
		const float UpperLimbLength = (InOutJointTransform.GetLocation() - InOutRootTransform.GetLocation()).Size();
		AimHelper::Solve_Modified_TwoBoneIK_3(
			InOutRootTransform,
			InOutJointTransform,
			InOutEndTransform,
			JointTarget,
			Effector,
			ThighEffector,
			UpperLimbLength,
			LowerLimbLength,
			bAllowStretching,
			StartStretchRatio,
			MaxStretchScale);

	}


	void Solve_Modified_Direct_TwoBoneIK(
		const FVector& RootPos,
		const FVector& JointPos,
		const FVector& EndPos,
		const FVector& JointTarget,
		const FVector& Effector,
		FVector& OutJointPos,
		FVector& OutEndPos,
		float UpperLimbLength,
		float LowerLimbLength,
		bool bAllowStretching,
		float StartStretchRatio,
		float MaxStretchScale)
	{
		// This is our reach goal.
		FVector DesiredPos = Effector;
		FVector DesiredDelta = DesiredPos - RootPos;
		float DesiredLength = DesiredDelta.Size();

		// Find lengths of upper and lower limb in the ref skeleton.
		// Use actual sizes instead of ref skeleton, so we take into account translation and scaling from other bone controllers.
		float MaxLimbLength = LowerLimbLength + UpperLimbLength;

		// Check to handle case where DesiredPos is the same as RootPos.
		FVector	DesiredDir;
		if (DesiredLength < KINDA_SMALL_NUMBER)
		{
			DesiredLength = KINDA_SMALL_NUMBER;
			DesiredDir = FVector(1, 0, 0);
		}
		else
		{
			DesiredDir = DesiredDelta.GetSafeNormal();
		}

		// Get joint target (used for defining plane that joint should be in).
		FVector JointTargetDelta = JointTarget - RootPos;
		const float JointTargetLengthSqr = JointTargetDelta.SizeSquared();

		// Same check as above, to cover case when JointTarget position is the same as RootPos.
		FVector JointPlaneNormal, JointBendDir;
		if (JointTargetLengthSqr < FMath::Square(KINDA_SMALL_NUMBER))
		{
			JointBendDir = FVector(0, 1, 0);
			JointPlaneNormal = FVector(0, 0, 1);
		}
		else
		{
			JointPlaneNormal = DesiredDir ^ JointTargetDelta;
			// If we are trying to point the limb in the same direction that we are supposed to displace the joint in, 
			// we have to just pick 2 random vector perp to DesiredDir and each other.
			if (JointPlaneNormal.SizeSquared() < FMath::Square(KINDA_SMALL_NUMBER))
			{
				DesiredDir.FindBestAxisVectors(JointPlaneNormal, JointBendDir);
			}
			else
			{
				JointPlaneNormal.Normalize();

				// Find the final member of the reference frame by removing any component of JointTargetDelta along DesiredDir.
				// This should never leave a zero vector, because we've checked DesiredDir and JointTargetDelta are not parallel.
				JointBendDir = JointTargetDelta - ((JointTargetDelta | DesiredDir) * DesiredDir);
				JointBendDir.Normalize();
			}
		}

		//UE_LOG(LogAnimationCore, Log, TEXT("UpperLimb : %0.2f, LowerLimb : %0.2f, MaxLimb : %0.2f"), UpperLimbLength, LowerLimbLength, MaxLimbLength);

		if (bAllowStretching)
		{
			const float ScaleRange = MaxStretchScale - StartStretchRatio;
			if (ScaleRange > KINDA_SMALL_NUMBER && MaxLimbLength > KINDA_SMALL_NUMBER)
			{
				const float ReachRatio = DesiredLength / MaxLimbLength;
				const float ScalingFactor = (MaxStretchScale - 1.f) * FMath::Clamp((ReachRatio - StartStretchRatio) / ScaleRange, 0.f, 1.f);
				if (ScalingFactor > KINDA_SMALL_NUMBER)
				{
					LowerLimbLength *= (1.f + ScalingFactor);
					UpperLimbLength *= (1.f + ScalingFactor);
					MaxLimbLength *= (1.f + ScalingFactor);
				}
			}
		}
		OutEndPos = DesiredPos;
		OutJointPos = JointPos;

		// If we are trying to reach a goal beyond the length of the limb, clamp it to something solvable and extend limb fully.
		if (DesiredLength >= MaxLimbLength)
		{
			OutEndPos = RootPos + (MaxLimbLength * DesiredDir);
			OutJointPos = RootPos + (UpperLimbLength * DesiredDir);
		}
		else
		{
			// So we have a triangle we know the side lengths of. We can work out the angle between DesiredDir and the direction of the upper limb
			// using the sin rule:
			const float TwoAB = 2.f * UpperLimbLength * DesiredLength;
			const float CosAngle = (TwoAB != 0.f) ? ((UpperLimbLength * UpperLimbLength) + (DesiredLength * DesiredLength) - (LowerLimbLength * LowerLimbLength)) / TwoAB : 0.f;
			// If CosAngle is less than 0, the upper arm actually points the opposite way to DesiredDir, so we handle that.
			const bool bReverseUpperBone = (CosAngle < 0.f);
			// Angle between upper limb and DesiredDir
			// ACos clamps internally so we dont need to worry about out-of-range values here.
			const float Angle = FMath::Acos(CosAngle);
			// Now we calculate the distance of the joint from the root -> effector line.
			// This forms a right-angle triangle, with the upper limb as the hypotenuse.
			const float JointLineDist = UpperLimbLength * FMath::Sin(Angle);
			// And the final side of that triangle - distance along DesiredDir of perpendicular.
			// ProjJointDistSqr can't be neg, because JointLineDist must be <= UpperLimbLength because appSin(Angle) is <= 1.
			const float ProjJointDistSqr = (UpperLimbLength * UpperLimbLength) - (JointLineDist * JointLineDist);
			// although this shouldn't be ever negative, sometimes Xbox release produces -0.f, causing ProjJointDist to be NaN
			// so now I branch it. 						
			float ProjJointDist = (ProjJointDistSqr > 0.f) ? FMath::Sqrt(ProjJointDistSqr) : 0.f;
			if (bReverseUpperBone)
			{
				ProjJointDist *= -1.f;
			}
			// So now we can work out where to put the joint!
			OutJointPos = RootPos + (ProjJointDist * DesiredDir) + (JointLineDist * JointBendDir);
		}
	}


	void Solve_Modified_Direct_TwoBoneIK_3(
		const FTransform& ComponentTransform,
		const float WristRotation,
		FTransform& InOutRootTransform,
		FTransform& InOutJointTransform,
		FTransform& InOutEndTransform,
		const FVector& JointTarget,
		const FVector& Effector,
		float UpperLimbLength,
		float LowerLimbLength,
		bool bAllowStretching,
		float StartStretchRatio,
		float MaxStretchScale,
		const bool bIsUpArmTwistTech)
	{
		FVector OutJointPos, OutEndPos;
		FVector RootPos = InOutRootTransform.GetLocation();
		FVector JointPos = InOutJointTransform.GetLocation();
		FVector EndPos = InOutEndTransform.GetLocation();

		// IK solver
		Solve_Modified_Direct_TwoBoneIK(
			RootPos,
			JointPos,
			EndPos,
			JointTarget,
			Effector,
			OutJointPos,
			OutEndPos,
			UpperLimbLength,
			LowerLimbLength,
			bAllowStretching,
			StartStretchRatio,
			MaxStretchScale);

		// update transform for middle bone
		{
			// Get difference in direction for old and new joint orientations
			const FVector OldDir = (EndPos - JointPos).GetSafeNormal();
			const FVector NewDir = (OutEndPos - OutJointPos).GetSafeNormal();

			const FRotator Rot_Ref_01 = UQuadrupedIKLibrary::CustomLookRotation((EndPos - JointPos).GetSafeNormal(), FVector::UpVector);
			const FRotator Rot_Ref_02 = UQuadrupedIKLibrary::CustomLookRotation((OutEndPos - OutJointPos).GetSafeNormal(), FVector::UpVector);

			const FQuat Delta1 = Rot_Ref_02.Quaternion() * Rot_Ref_01.Quaternion().Inverse();
			const FQuat FBNormals = FQuat::FindBetweenNormals(OldDir, NewDir);
			FQuat DeltaRotation;

			if (bIsUpArmTwistTech)
			{
				DeltaRotation = Delta1.GetNormalized();
			}
			else
			{
				DeltaRotation = FBNormals.GetNormalized();
			}

			// Rotate our Joint quaternion by this delta rotation
			InOutJointTransform.SetRotation(DeltaRotation * InOutJointTransform.GetRotation());
			// And put joint where it should be.
			InOutJointTransform.SetTranslation(OutJointPos);
		}

		// Update transform for upper bone.
		{
			const FVector OldDir = (JointPos - RootPos).GetSafeNormal();
			const FVector NewDir = (OutJointPos - RootPos).GetSafeNormal();

			const FRotator Rot_Ref_01 = UQuadrupedIKLibrary::CustomLookRotation((JointPos - RootPos).GetSafeNormal(), FVector::UpVector);
			const FRotator Rot_Ref_02 = UQuadrupedIKLibrary::CustomLookRotation((OutJointPos - RootPos).GetSafeNormal(), FVector::UpVector);
			const FQuat Delta1 = Rot_Ref_02.Quaternion() * Rot_Ref_01.Quaternion().Inverse();
			const FQuat FBNormals = FQuat::FindBetweenNormals(OldDir, NewDir);

			FQuat DeltaRotation;

			if (bIsUpArmTwistTech)
			{
				DeltaRotation = Delta1.GetNormalized();
			}
			else
			{
				DeltaRotation = FBNormals.GetNormalized();
			}

			InOutRootTransform.SetRotation(DeltaRotation * InOutRootTransform.GetRotation());
			InOutRootTransform.SetTranslation(RootPos);
		}

		InOutEndTransform.SetTranslation(OutEndPos);
	}


	void Solve_Modified_Direct_TwoBoneIK_4(
		const FTransform ComponentTransform,
		const float WristRotation,
		FTransform& InOutRootTransform,
		FTransform& InOutJointTransform,
		FTransform& InOutEndTransform,
		const FVector& JointTarget,
		const FVector& Effector,
		const bool bAllowStretching,
		const float StartStretchRatio,
		const float MaxStretchScale,
		const bool bIsUpArmTwistTech)
	{
		const float LowerLimbLength = (InOutEndTransform.GetLocation() - InOutJointTransform.GetLocation()).Size();
		const float UpperLimbLength = (InOutJointTransform.GetLocation() - InOutRootTransform.GetLocation()).Size();
		Solve_Modified_Direct_TwoBoneIK_3(
			ComponentTransform,
			WristRotation,
			InOutRootTransform,
			InOutJointTransform,
			InOutEndTransform,
			JointTarget,
			Effector,
			UpperLimbLength,
			LowerLimbLength,
			bAllowStretching,
			StartStretchRatio,
			MaxStretchScale,
			bIsUpArmTwistTech);
	}



	FTransform LookAt_Processor_Helper(
		FTransform& ComponentBoneTransform,
		const FVector& HeadLocation,
		const FVector& OffsetVector,
		const FAxis& LookAtAxis,
		const float LookatClamp,
		const FRotator& InnerBodyClamp,
		bool bIsUseNaturalMethod/* = true*/,
		float UpRotationClamp/* = 1*/,
		float Intensity/* = 1*/)
	{
		const FVector TargetLocationInComponentSpace = OffsetVector;
		const FVector LookAtVector = LookAtAxis.GetTransformedAxis(ComponentBoneTransform).GetSafeNormal();
		const FVector TargetDir = (TargetLocationInComponentSpace - HeadLocation).GetSafeNormal();
		FVector HorizontalTargetDir = TargetDir;
		FVector OppositeTargetDir = (HeadLocation - TargetLocationInComponentSpace).GetSafeNormal();
		//OppositeTargetDir.Y = 0;

		const FQuat BaseRotation = ComponentBoneTransform.GetRotation();

		const float AimClampInRadians = FMath::DegreesToRadians(FMath::Min(LookatClamp, 180.f));

		const float Dot1 = FMath::Clamp(FVector::DotProduct(LookAtVector, OppositeTargetDir), -1.0f, 1.0f);
		const float DiffAngle = FMath::Acos(Dot1);

		FVector SelectedVector = HorizontalTargetDir;

		if (DiffAngle > AimClampInRadians)
		{
			FVector DeltaTarget = OppositeTargetDir - LookAtVector;
			DeltaTarget *= (AimClampInRadians / DiffAngle);
			OppositeTargetDir = LookAtVector + DeltaTarget;
			OppositeTargetDir.Normalize();
		}

		const float AimClampInRadiansForward = FMath::DegreesToRadians(FMath::Min(LookatClamp, 180.f));
		const float Dot2 = FMath::Clamp(FVector::DotProduct(LookAtVector, HorizontalTargetDir), -1.0f, 1.0f);
		const float DiffAngleForward = FMath::Acos(Dot2);

		if (DiffAngleForward > AimClampInRadiansForward)
		{
			FVector DeltaTargetForward = HorizontalTargetDir - LookAtVector;
			DeltaTargetForward *= (AimClampInRadiansForward / DiffAngleForward);
			HorizontalTargetDir = LookAtVector + DeltaTargetForward;
			HorizontalTargetDir.Normalize();
		}

		SelectedVector = HorizontalTargetDir;
		SelectedVector.Z = SelectedVector.Z * UpRotationClamp;

		FQuat NormalizedDelta = FQuat::FindBetweenNormals(LookAtVector, SelectedVector);

		if (!bIsUseNaturalMethod)
		{
			const FRotator RotRef1 = UKismetMathLibrary::FindLookAtRotation(FVector::ZeroVector, SelectedVector * 100);
			const FRotator RotRef2 = UKismetMathLibrary::FindLookAtRotation(FVector::ZeroVector, LookAtVector * 100);
			NormalizedDelta = RotRef1.Quaternion() * RotRef2.Quaternion().Inverse();
		}

		const FQuat NormalizedDeltaRef = FQuat::FindBetweenNormals(LookAtVector, LookAtVector);
		FVector RotEuler = NormalizedDelta.Euler();
		RotEuler.X = FMath::ClampAngle(RotEuler.X, -LookatClamp, LookatClamp);
		RotEuler.Y = FMath::ClampAngle(RotEuler.Y, -LookatClamp, LookatClamp);
		RotEuler.Z = FMath::ClampAngle(RotEuler.Z, -LookatClamp, LookatClamp);
		NormalizedDelta = FQuat::MakeFromEuler(RotEuler);
		NormalizedDelta = FQuat::Slerp(NormalizedDeltaRef, NormalizedDelta, Intensity);
		FTransform WSRotationTransform = ComponentBoneTransform;
		FRotator NormalizedDeltaRot = NormalizedDelta.Rotator();

		FRotator InnerBodyClampAbs = InnerBodyClamp;
		InnerBodyClampAbs.Pitch = FMath::Abs(InnerBodyClampAbs.Pitch);
		InnerBodyClampAbs.Yaw = FMath::Abs(InnerBodyClampAbs.Yaw);
		InnerBodyClampAbs.Roll = FMath::Abs(InnerBodyClampAbs.Roll);


		if (InnerBodyClampAbs.Pitch > 0)
		{
			NormalizedDeltaRot.Pitch = FMath::ClampAngle(NormalizedDeltaRot.Pitch, -InnerBodyClampAbs.Pitch, InnerBodyClampAbs.Pitch);
		}

		if (InnerBodyClampAbs.Roll > 0)
		{
			NormalizedDeltaRot.Roll = FMath::ClampAngle(NormalizedDeltaRot.Roll, -InnerBodyClampAbs.Roll, InnerBodyClampAbs.Roll);
		}

		if (InnerBodyClampAbs.Yaw > 0)
		{
			NormalizedDeltaRot.Yaw = FMath::ClampAngle(NormalizedDeltaRot.Yaw, -InnerBodyClampAbs.Yaw, InnerBodyClampAbs.Yaw);
		}

		WSRotationTransform.SetRotation(NormalizedDeltaRot.Quaternion());
		ComponentBoneTransform.SetRotation(WSRotationTransform.GetRotation() * BaseRotation);
		return ComponentBoneTransform;
	}


	FTransform LookAt_Processor_Helper_WithBaseRotation(
		FTransform& ComponentBoneTransform,
		const FQuat& BaseRotation,
		const FVector& HeadLocation,
		const FVector& OffsetVector,
		const FAxis& LookAtAxis,
		const float LookatClamp,
		const FRotator& InnerBodyClamp,
		bool bIsUseNaturalMethod = true,
		float UpRotationClamp = 1.0f,
		float Intensity = 1.0f,
		bool bIsIntermediateJoint = false)
	{
		const FVector TargetLocationInComponentSpace = OffsetVector;
		const FVector LookAtVector = LookAtAxis.GetTransformedAxis(ComponentBoneTransform).GetSafeNormal();

		const FVector BoneLocation = ComponentBoneTransform.GetLocation();
		const FVector TargetDir = (TargetLocationInComponentSpace - BoneLocation).GetSafeNormal();

		FVector HorizontalTargetDir = TargetDir;
		FVector OppositeTargetDir = (BoneLocation - TargetLocationInComponentSpace).GetSafeNormal();

		const float AimClampInRadians = FMath::DegreesToRadians(FMath::Min(LookatClamp, 180.f));

		const float Dot1 = FMath::Clamp(FVector::DotProduct(LookAtVector, OppositeTargetDir), -1.0f, 1.0f);
		const float DiffAngle = FMath::Acos(Dot1);

		FVector SelectedVector = HorizontalTargetDir;

		if (DiffAngle > AimClampInRadians)
		{
			FVector DeltaTarget = OppositeTargetDir - LookAtVector;
			DeltaTarget *= (AimClampInRadians / DiffAngle);
			OppositeTargetDir = LookAtVector + DeltaTarget;
			OppositeTargetDir.Normalize();
		}

		const float Dot2 = FMath::Clamp(FVector::DotProduct(LookAtVector, HorizontalTargetDir), -1.0f, 1.0f);
		const float DiffAngleForward = FMath::Acos(Dot2);

		if (DiffAngleForward > AimClampInRadians)
		{
			FVector DeltaTargetForward = HorizontalTargetDir - LookAtVector;
			DeltaTargetForward *= (AimClampInRadians / DiffAngleForward);
			HorizontalTargetDir = LookAtVector + DeltaTargetForward;
			HorizontalTargetDir.Normalize();
		}

		SelectedVector = HorizontalTargetDir;
		SelectedVector.Z *= UpRotationClamp;

		FQuat NormalizedDelta = FQuat::FindBetweenNormals(LookAtVector, SelectedVector);

		if (!bIsUseNaturalMethod)
		{
			const FRotator RotRef1 = UKismetMathLibrary::FindLookAtRotation(FVector::ZeroVector, SelectedVector * 100.0f);
			const FRotator RotRef2 = UKismetMathLibrary::FindLookAtRotation(FVector::ZeroVector, LookAtVector * 100.0f);
			NormalizedDelta = RotRef1.Quaternion() * RotRef2.Quaternion().Inverse();
		}

		const FQuat IdentityDelta = FQuat::Identity;

		FVector RotEuler = NormalizedDelta.Euler();
		RotEuler.X = FMath::ClampAngle(RotEuler.X, -LookatClamp, LookatClamp);
		RotEuler.Y = FMath::ClampAngle(RotEuler.Y, -LookatClamp, LookatClamp);
		RotEuler.Z = FMath::ClampAngle(RotEuler.Z, -LookatClamp, LookatClamp);

		NormalizedDelta = FQuat::MakeFromEuler(RotEuler);
		NormalizedDelta = FQuat::Slerp(IdentityDelta, NormalizedDelta, Intensity).GetNormalized();

		FRotator DeltaRot = NormalizedDelta.Rotator();

		FRotator ClampLimit = InnerBodyClamp;
		ClampLimit.Pitch = FMath::Abs(ClampLimit.Pitch);
		ClampLimit.Yaw = FMath::Abs(ClampLimit.Yaw);
		ClampLimit.Roll = FMath::Abs(ClampLimit.Roll);

		if (ClampLimit.Pitch > 0.0f)
		{
			DeltaRot.Pitch = FMath::ClampAngle(DeltaRot.Pitch, -ClampLimit.Pitch, ClampLimit.Pitch);
		}

		if (ClampLimit.Roll > 0.0f)
		{
			DeltaRot.Roll = FMath::ClampAngle(DeltaRot.Roll, -ClampLimit.Roll, ClampLimit.Roll);
		}

		if (ClampLimit.Yaw > 0.0f)
		{
			DeltaRot.Yaw = FMath::ClampAngle(DeltaRot.Yaw, -ClampLimit.Yaw, ClampLimit.Yaw);
		}
		const FQuat ClampedDeltaQuat = DeltaRot.Quaternion().GetNormalized();
		ComponentBoneTransform.SetRotation((ClampedDeltaQuat * BaseRotation).GetNormalized());
		return ComponentBoneTransform;
	}


	void Evaluate_ConsecutiveBoneRotations_LookAt(
		FComponentSpacePoseContext& Output,
		const FBoneReference& RootBoneInput,
		const FBoneReference& TipBoneInput,
		const FRotator& InnerBodyClamp,
		const FTransform& EffectorTransform,
		const FAxis& LookAtAxis,
		const FAxis& IntermediateLookAtAxis,
		FRuntimeFloatCurve& InLookBendingCurve,
		FRuntimeFloatCurve& InLookMultiplierCurve,
		const bool bUseIntermediateAxis,
		const bool bIsEnableDipMultiplier,
		const float LookatClamp,
		const float VerticalDipTreshold,
		const float DownwardDipMultiplier,
		const float InvertedDipMultiplier,
		const float SideMultiplier,
		const float SideDownMultiplier,
		const FTransform PelvisLocTarget,
		const float UpRotClamp,
		const bool bIsUseNaturalRotation,
		const bool bIsSeparateHeadClamp,
		const float HeadClampValue,
		const FTransform& HeadTransf,
		const bool bUsePerJointRotationOverrides,
		const TMap<FName, FAimJointRotationOverride>& PerJointRotationOverrides,
		TArray<FBoneTransform>& OutBoneTransforms)
	{
		const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();

		FTransform CSEffectorTransform = EffectorTransform;
		FAnimationRuntime::ConvertBoneSpaceTransformToCS(Output.AnimInstanceProxy->GetComponentTransform(),
			Output.Pose,
			CSEffectorTransform, RootBoneInput.GetCompactPoseIndex(BoneContainer),
			EBoneControlSpace::BCS_WorldSpace);

		FVector CSEffectorLocation = CSEffectorTransform.GetLocation();

		// Gather all bone indices between root and tip.
		TArray<FCompactPoseBoneIndex> BoneIndices;

		{
			const FCompactPoseBoneIndex RootIndex = RootBoneInput.GetCompactPoseIndex(BoneContainer);
			FCompactPoseBoneIndex BoneIndex = TipBoneInput.GetCompactPoseIndex(BoneContainer);
			do
			{
				BoneIndices.Insert(BoneIndex, 0);
				BoneIndex = Output.Pose.GetPose().GetParentBoneIndex(BoneIndex);
			} while (BoneIndex != RootIndex);
			BoneIndices.Insert(BoneIndex, 0);
		}

		const int32 NumTransforms = BoneIndices.Num();

		// Maximum length of skeleton segment at full extension
		float MaximumReach = 0;
		// Gather chain links. These are non zero length bones.
		TArray<FCCDIK_Modified_ChainLink> Chain;
		Chain.Reserve(NumTransforms);

		// Start with Root Bone
		{
			const FCompactPoseBoneIndex& RootBoneIndex = BoneIndices[0];
			const FTransform& BoneCSTransform = Output.Pose.GetComponentSpaceTransform(RootBoneIndex);
			const FTransform& BoneLocalTransform = Output.Pose.GetComponentSpaceTransform(RootBoneIndex);
			OutBoneTransforms[0] = FBoneTransform(RootBoneIndex, BoneCSTransform);
			Chain.Add(FCCDIK_Modified_ChainLink(BoneCSTransform.GetLocation(), BoneLocalTransform.GetLocation(), BoneCSTransform.GetRotation(), 0.f, RootBoneIndex, 0));
		}



		// Go through remaining transforms
		for (int32 TransformIndex = 1; TransformIndex < NumTransforms; TransformIndex++)
		{
			const FCompactPoseBoneIndex& BoneIndex = BoneIndices[TransformIndex];
			const FTransform& BoneCSTransform = Output.Pose.GetComponentSpaceTransform(BoneIndex);
			FTransform BoneLocalTransform = Output.Pose.GetLocalSpaceTransform(BoneIndex);
			const FVector BoneCSPosition = BoneCSTransform.GetLocation();
			OutBoneTransforms[TransformIndex] = FBoneTransform(BoneIndex, BoneCSTransform);
			// Calculate the combined length of this segment of skeleton
			const float BoneLength = FVector::Dist(BoneCSPosition, OutBoneTransforms[TransformIndex - 1].Transform.GetLocation());
			if (!FMath::IsNearlyZero(BoneLength))
			{
				Chain.Add(FCCDIK_Modified_ChainLink(BoneCSPosition, BoneLocalTransform.GetLocation(), BoneCSTransform.GetRotation(), BoneLength, BoneIndex, TransformIndex));
				MaximumReach += BoneLength;
			}
			else
			{
				// Mark this transform as a zero length child of the last link.
				// It will inherit position and delta rotation from parent link.
				FCCDIK_Modified_ChainLink& ParentLink = Chain[Chain.Num() - 1];
				ParentLink.ChildZeroLengthTransformIndices.Add(TransformIndex);
			}
		}

		const int32 NumChainLinks = Chain.Num();
		FTransform HeadLocation = Output.Pose.GetComponentSpaceTransform(BoneIndices[BoneIndices.Num() - 1]);
		const FTransform PelvisLocation = Output.Pose.GetComponentSpaceTransform(BoneIndices[0]);

		float UpLength = 0;
		UpLength = -(CSEffectorLocation.Z - Chain[NumChainLinks - 1].Position.Z);
		UpLength = FMath::Clamp(UpLength, -(FMath::Abs(Chain[NumChainLinks - 1].Position.Z * 0.5f)), 0);
		const FVector DiffPelvis = (PelvisLocation.GetLocation() - PelvisLocTarget.GetLocation());
		FTransform PelvisRefFullTransform;

		TArray<FQuat> BaseRotations;
		BaseRotations.SetNumZeroed(NumTransforms);

		for (int32 TransformIndex = 0; TransformIndex < NumTransforms; ++TransformIndex)
		{
			const FCompactPoseBoneIndex& BoneIndex = BoneIndices[TransformIndex];
			BaseRotations[TransformIndex] = Output.Pose.GetComponentSpaceTransform(BoneIndex).GetRotation();
		}

		for (int32 LinkIndex = 0; LinkIndex < NumChainLinks; LinkIndex++)
		{
			const FCCDIK_Modified_ChainLink& ChainLink = Chain[LinkIndex];
			const float NewValue = InLookBendingCurve.GetRichCurve()->Eval((float)LinkIndex / (float)NumChainLinks);
			float MultiplierValue = InLookMultiplierCurve.GetRichCurve()->Eval((float)LinkIndex / (float)NumChainLinks);
			FVector TempTargetLoc = CSEffectorLocation;

			FAxis ActiveAxis = LookAtAxis;

			const bool bIsIntermediateJoint = (LinkIndex > 0 && LinkIndex < NumChainLinks - 1);
			const bool bIsFirstJoint = (LinkIndex == 0);
			const bool bIsLastJoint = (LinkIndex == NumChainLinks - 1);

			if (bIsIntermediateJoint && bUseIntermediateAxis)
			{
				ActiveAxis = IntermediateLookAtAxis;
			}

			// if last index to head joint pos
			if (bIsLastJoint)
			{
				TempTargetLoc = HeadTransf.GetLocation();
			}

			const FVector RootPos = Chain[0].Position;
			const float TargetDist = FVector::Dist(RootPos, CSEffectorLocation);
			const float ReachRatio = (MaximumReach > KINDA_SMALL_NUMBER) ? (TargetDist / MaximumReach) : 0.0f;
			const float ReachAlpha = FMath::Clamp(ReachRatio, 0.0f, 1.0f);

			const FRotator OriginalRot = OutBoneTransforms[ChainLink.TransformIndex].Transform.Rotator();
			float LookAtModifiedClamp = LookatClamp * NewValue;
			float UpLookRatio = UpRotClamp;
			FRotator InnerBodyClampVal = InnerBodyClamp;

			if (bIsSeparateHeadClamp && bIsLastJoint)
			{
				LookAtModifiedClamp = HeadClampValue;
				InnerBodyClampVal = FRotator(0, 0, 0);
				UpLookRatio = 1.0f;
				MultiplierValue = 1.0f;
			}

			const FName BoneName = AimHelper::GetBoneNameFromCompactIndex(BoneContainer, BoneIndices[ChainLink.TransformIndex]);

			if (bUsePerJointRotationOverrides)
			{
				if (const FAimJointRotationOverride* Override = PerJointRotationOverrides.Find(BoneName))
				{
					if (Override->bOverrideMultiplier)
					{
						MultiplierValue = Override->Multiplier;
					}

					if (Override->bOverrideUpRotClamp)
					{
						UpLookRatio = Override->UpRotClamp;
					}

					if (Override->bOverrideClamp)
					{
						InnerBodyClampVal = Override->Clamp;
					}
				}
			}

			if (bIsFirstJoint)
			{
				PelvisRefFullTransform = LookAt_Processor_Helper(
					OutBoneTransforms[0].Transform,
					HeadLocation.GetLocation(),
					TempTargetLoc,
					LookAtAxis,
					80.0f,
					InnerBodyClamp,
					bIsUseNaturalRotation,
					1.0f,
					1.0f);

			}
			else
			{
				OutBoneTransforms[ChainLink.TransformIndex].Transform = LookAt_Processor_Helper(
					OutBoneTransforms[ChainLink.TransformIndex].Transform,
					HeadLocation.GetLocation(),
					TempTargetLoc,
					ActiveAxis,
					LookAtModifiedClamp,
					InnerBodyClamp,
					bIsUseNaturalRotation,
					ReachAlpha,
					MultiplierValue);
			}

			FTransform OldParent = FTransform::Identity;
			FTransform CurrentParent = FTransform::Identity;

			if (ChainLink.TransformIndex > 0)
			{
				CurrentParent = OutBoneTransforms[ChainLink.TransformIndex - 1].Transform;
				const FCompactPoseBoneIndex& Parent_BoneIndex = BoneIndices[ChainLink.TransformIndex - 1];
				OldParent = Output.Pose.GetComponentSpaceTransform(Parent_BoneIndex);
			}

			//Get Delta result
			FTransform ParentDifference = OldParent.Inverse() * CurrentParent;
			OutBoneTransforms[ChainLink.TransformIndex].Transform.SetLocation(
				(OutBoneTransforms[ChainLink.TransformIndex].Transform * ParentDifference).GetLocation());
		}


		if (bIsEnableDipMultiplier)
		{
			const FRotator InvertedPelvisRot = FRotator(PelvisRefFullTransform.GetRotation() * PelvisLocation.GetRotation().Inverse());
			const float Side_Angle = -InvertedPelvisRot.Yaw;
			float VerticalAngle = FMath::Abs(InvertedPelvisRot.Roll);
			VerticalAngle = FMath::Clamp(VerticalAngle - VerticalDipTreshold, 0, 1000);

			float VerticalDirectionVal = 1;

			if ((NumChainLinks - 1) > 0)
			{
				VerticalDirectionVal = CSEffectorLocation.Z > OutBoneTransforms[NumChainLinks - 1].Transform.GetLocation().Z ? 1 : -1;
			}

			const float SideDownVal = FMath::Abs(Side_Angle);
			const FVector RightDir = FVector::CrossProduct(LookAtAxis.Axis, FVector::UpVector);

			for (int32 LinkIndex = 0; LinkIndex < NumChainLinks; LinkIndex++)
			{
				const FCCDIK_Modified_ChainLink& ChainLink = Chain[LinkIndex];

				if (VerticalDirectionVal == 1)
				{
					OutBoneTransforms[ChainLink.TransformIndex].Transform.AddToTranslation(FVector(0, 0, 1) * DownwardDipMultiplier * VerticalAngle);
				}
				else
				{
					OutBoneTransforms[ChainLink.TransformIndex].Transform.AddToTranslation(FVector(0, 0, 1) * InvertedDipMultiplier * VerticalAngle);
				}
				OutBoneTransforms[ChainLink.TransformIndex].Transform.AddToTranslation(RightDir * SideMultiplier * Side_Angle);
				OutBoneTransforms[ChainLink.TransformIndex].Transform.AddToTranslation(FVector(0, 0, 1) * SideDownMultiplier * SideDownVal);
			}

		}

	}


	void Evaluate_ConsecutiveBoneRotations_CCD(
		FComponentSpacePoseContext& Output,
		const FBoneReference& StartSplineBone,
		const FBoneReference& EndSplineBone,
		const FRotator& InBaseAngle,
		const FTransform& EffectorTransform,
		const FAxis& LookAtAxis,
		FRuntimeFloatCurve& InLookBendingCurve,
		FRuntimeFloatCurve& InLookMultiplierCurve,
		const bool bIsEnableDipMultiplier,
		const float VerticalDipTreshold,
		const float DownwardDipMultiplier,
		const float InvertedDipMultiplier,
		const float SideMultiplier,
		const float SideDownMultiplier,
		const bool bIsAlterPelvis,
		const FTransform PelvisLocTarget,
		const float UpRotClamp,
		const bool bIsUseNaturalRotation,
		const bool bIsHeadUseSeparateClamp,
		const FTransform& HeadTransf,
		const float Precision,
		const int32 MaxIteration,
		const bool bUsePerJointRotationOverrides,
		const TMap<FName, FAimJointRotationOverride>& PerJointRotationOverrides,
		const bool bUseTipReferenceBone,
		const FBoneReference& TipReferenceBone,
		const FAimRobotArmSettings& RobotArm,
		const bool bAlignTipYawToTarget,
		TArray<FBoneTransform>& OutBoneTransforms)
	{
		const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();

		FTransform CSEffectorTransform = EffectorTransform;
		FAnimationRuntime::ConvertBoneSpaceTransformToCS(
			Output.AnimInstanceProxy->GetComponentTransform(),
			Output.Pose,
			CSEffectorTransform, StartSplineBone.GetCompactPoseIndex(BoneContainer), EBoneControlSpace::BCS_WorldSpace);

		const FVector CSEffectorLocation = CSEffectorTransform.GetLocation();

		FVector TipReferenceOffsetLS = FVector::ZeroVector;
		if (bUseTipReferenceBone && TipReferenceBone.IsValidToEvaluate(BoneContainer))
		{
			const FCompactPoseBoneIndex EndIndex = EndSplineBone.GetCompactPoseIndex(BoneContainer);
			const FCompactPoseBoneIndex RefIndex = TipReferenceBone.GetCompactPoseIndex(BoneContainer);
			const FTransform EndCS = Output.Pose.GetComponentSpaceTransform(EndIndex);
			const FTransform RefCS = Output.Pose.GetComponentSpaceTransform(RefIndex);
			TipReferenceOffsetLS = EndCS.InverseTransformPosition(RefCS.GetLocation());
		}

		TArray<FCompactPoseBoneIndex> BoneIndices;
		{
			const FCompactPoseBoneIndex RootIndex = StartSplineBone.GetCompactPoseIndex(BoneContainer);
			FCompactPoseBoneIndex BoneIndex = EndSplineBone.GetCompactPoseIndex(BoneContainer);

			do
			{
				BoneIndices.Insert(BoneIndex, 0);
				BoneIndex = Output.Pose.GetPose().GetParentBoneIndex(BoneIndex);
			} while (BoneIndex != RootIndex);

			BoneIndices.Insert(BoneIndex, 0);
		}

		const int32 NumTransforms = BoneIndices.Num();
		if (NumTransforms < 2)
		{
			return;
		}


		TArray<FTransform> CurrentTransforms;
		CurrentTransforms.SetNumUninitialized(NumTransforms);

		TArray<FQuat> BaseRotations;
		BaseRotations.SetNumUninitialized(NumTransforms);

		TArray<float> SegmentLengths;
		SegmentLengths.SetNumZeroed(NumTransforms);

		float MaximumReach = 0.0f;

		for (int32 i = 0; i < NumTransforms; ++i)
		{
			const FCompactPoseBoneIndex BoneIndex = BoneIndices[i];
			const FTransform BoneCS = Output.Pose.GetComponentSpaceTransform(BoneIndex);

			CurrentTransforms[i] = BoneCS;
			BaseRotations[i] = BoneCS.GetRotation();
			OutBoneTransforms[i] = FBoneTransform(BoneIndex, BoneCS);

			if (i > 0)
			{
				SegmentLengths[i] = FVector::Dist(CurrentTransforms[i - 1].GetLocation(), CurrentTransforms[i].GetLocation());
				MaximumReach += SegmentLengths[i];
			}
		}

		const FVector RootInitialPos = CurrentTransforms[0].GetLocation();
		const FVector TipInitialPos = CurrentTransforms.Last().GetLocation();

		// 仮想 tip がある場合のみ、末端ボーン自身も解決対象に含める
		const bool bHasVirtualTip = (bUseTipReferenceBone && TipReferenceBone.IsValidToEvaluate(BoneContainer));
		const int32 LastSolvingLinkIndex = bHasVirtualTip ? (NumTransforms - 1) : (NumTransforms - 2);

		// 入力ポーズからの累積回転角(rad)。可動域クランプ用
		TArray<float> AccumHingeAngles;
		AccumHingeAngles.SetNumZeroed(NumTransforms);

		const float MaxStepRad = FMath::DegreesToRadians(FMath::Max(RobotArm.MaxAnglePerIterationDeg, 1.0f));

		auto GetVirtualTipPos = [&](const FTransform& EndTransform) -> FVector
			{
				if (!bHasVirtualTip)
				{
					return EndTransform.GetLocation();
				}

				return EndTransform.TransformPosition(TipReferenceOffsetLS);
			};

		auto UpdateChildPositionsFrom = [&](int32 StartIndex)
			{
				for (int32 ChildIndex = StartIndex + 1; ChildIndex < NumTransforms; ++ChildIndex)
				{
					const FVector ParentPos = CurrentTransforms[ChildIndex - 1].GetLocation();
					const FVector OldParentPos = Output.Pose.GetComponentSpaceTransform(BoneIndices[ChildIndex - 1]).GetLocation();
					const FVector OldChildPos = Output.Pose.GetComponentSpaceTransform(BoneIndices[ChildIndex]).GetLocation();

					FVector OldDir = (OldChildPos - OldParentPos).GetSafeNormal();
					if (OldDir.IsNearlyZero())
					{
						OldDir = FVector::ForwardVector;
					}

					const FVector NewDir = CurrentTransforms[ChildIndex - 1].GetRotation().RotateVector(
						BaseRotations[ChildIndex - 1].Inverse().RotateVector(OldDir)).GetSafeNormal();

					CurrentTransforms[ChildIndex].SetLocation(ParentPos + NewDir * SegmentLengths[ChildIndex]);
				}
			};

		auto ClampDeltaRot = [](FRotator InRot, const FRotator& Limit)
			{
				FRotator Out = InRot;
				const FRotator AbsLimit(FMath::Abs(Limit.Pitch), FMath::Abs(Limit.Yaw), FMath::Abs(Limit.Roll));

				if (AbsLimit.Pitch > 0.f)
				{
					Out.Pitch = FMath::ClampAngle(Out.Pitch, -AbsLimit.Pitch, AbsLimit.Pitch);
				}
				if (AbsLimit.Yaw > 0.f)
				{
					Out.Yaw = FMath::ClampAngle(Out.Yaw, -AbsLimit.Yaw, AbsLimit.Yaw);
				}
				if (AbsLimit.Roll > 0.f)
				{
					Out.Roll = FMath::ClampAngle(Out.Roll, -AbsLimit.Roll, AbsLimit.Roll);
				}

				return Out;
			};

		// ヒンジ軸まわりの符号付き角度を求める
		auto SolveHingeAngle = [](const FVector& JointPos, const FVector& TipPos, const FVector& TargetPos, const FVector& AxisCS) -> float
			{
				const FVector ToTipFlat = FVector::VectorPlaneProject(TipPos - JointPos, AxisCS).GetSafeNormal();
				const FVector ToTargetFlat = FVector::VectorPlaneProject(TargetPos - JointPos, AxisCS).GetSafeNormal();

				if (ToTipFlat.IsNearlyZero() || ToTargetFlat.IsNearlyZero())
				{
					return 0.0f;
				}

				return FMath::Atan2(
					FVector::DotProduct(FVector::CrossProduct(ToTipFlat, ToTargetFlat), AxisCS),
					FVector::DotProduct(ToTipFlat, ToTargetFlat));
			};

		// 親の回転を子孫へ剛体伝播させる
		auto PropagateToChildren = [&](int32 LinkIndex, const FVector& PivotPos, const FQuat& Delta)
			{
				for (int32 ChildIndex = LinkIndex + 1; ChildIndex < NumTransforms; ++ChildIndex)
				{
					FTransform& Child = CurrentTransforms[ChildIndex];
					Child.SetLocation(PivotPos + Delta.RotateVector(Child.GetLocation() - PivotPos));
					Child.SetRotation((Delta * Child.GetRotation()).GetNormalized());
				}
			};

		const float PrecisionSq = FMath::Square(FMath::Max(Precision, 0.001f));

		for (int32 Iter = 0; Iter < MaxIteration; ++Iter)
		{
			const FVector CurrentTipPos = GetVirtualTipPos(CurrentTransforms.Last());
			if (FVector::DistSquared(CurrentTipPos, CSEffectorLocation) <= PrecisionSq)
			{
				break;
			}

			for (int32 LinkIndex = LastSolvingLinkIndex; LinkIndex >= 0; --LinkIndex)
			{
				const bool bIsLastJoint = (LinkIndex == LastSolvingLinkIndex);
				const bool bIsIntermediateJoint = (LinkIndex > 0 && LinkIndex < LastSolvingLinkIndex);

				FVector JointPos = CurrentTransforms[LinkIndex].GetLocation();
				FVector TipPos = GetVirtualTipPos(CurrentTransforms.Last());
				FVector ToTip = (TipPos - JointPos).GetSafeNormal();

				if (ToTip.IsNearlyZero())
				{
					continue;
				}

				FVector TargetPos = CSEffectorLocation;
				// 四足の頭用特別扱い。RobotArm では常に Effector を狙う
				if (bIsLastJoint && !RobotArm.bEnable)
				{
					TargetPos = HeadTransf.GetLocation();
				}

				FVector ToTarget = (TargetPos - JointPos).GetSafeNormal();
				if (ToTarget.IsNearlyZero())
				{
					continue;
				}

				const FName BoneName = AimHelper::GetBoneNameFromCompactIndex(BoneContainer, BoneIndices[LinkIndex]);
				const FAimJointRotationOverride* Override = bUsePerJointRotationOverrides ? PerJointRotationOverrides.Find(BoneName) : nullptr;

				float NewValue = InLookBendingCurve.GetRichCurve()->Eval((NumTransforms > 1) ? (float)LinkIndex / (float)(NumTransforms - 1) : 1.0f);
				float MultiplierValue = InLookMultiplierCurve.GetRichCurve()->Eval((NumTransforms > 1) ? (float)LinkIndex / (float)(NumTransforms - 1) : 1.0f);

				FQuat WeightedDelta = FQuat::Identity;
				FVector PivotPos = JointPos;


				if (RobotArm.bEnable)
				{
					if (Override && Override->bLockJoint)
					{
						continue;
					}

					if (Override && Override->bOverrideMultiplier)
					{
						MultiplierValue = Override->Multiplier;
					}

					const FVector HingeAxisLS = (Override && Override->bOverrideHingeAxis)
						? Override->HingeAxisLS
						: RobotArm.DefaultHingeAxisLS;

					const FVector HingeAxisCS = CurrentTransforms[LinkIndex].GetRotation().RotateVector(HingeAxisLS).GetSafeNormal();

					if (HingeAxisCS.IsNearlyZero())
					{
						continue;
					}

					float Step = SolveHingeAngle(JointPos, TipPos, TargetPos, HingeAxisCS);
					Step *= FMath::Clamp(MultiplierValue, 0.0f, 1.0f);
					Step = FMath::Clamp(Step, -MaxStepRad, MaxStepRad);

					if (Override && Override->bUseHingeAngleLimit)
					{
						const float MinRad = FMath::DegreesToRadians(FMath::Min(Override->HingeAngleLimit.X, Override->HingeAngleLimit.Y));
						const float MaxRad = FMath::DegreesToRadians(FMath::Max(Override->HingeAngleLimit.X, Override->HingeAngleLimit.Y));

						const float NewAccum = FMath::Clamp(AccumHingeAngles[LinkIndex] + Step, MinRad, MaxRad);
						Step = NewAccum - AccumHingeAngles[LinkIndex];
						AccumHingeAngles[LinkIndex] = NewAccum;
					}
					else
					{
						AccumHingeAngles[LinkIndex] += Step;
					}

					if (FMath::IsNearlyZero(Step, 1.e-5f))
					{
						continue;
					}

					WeightedDelta = FQuat(HingeAxisCS, Step);
				}
				else
				{

					const float DotToTarget = FVector::DotProduct(ToTip, ToTarget);

					FQuat DeltaQuat = FQuat::Identity;
					if (DotToTarget < -0.98f)
					{
						FVector StableAxis = FVector::CrossProduct(LookAtAxis.Axis.GetSafeNormal(), ToTip);
						if (StableAxis.IsNearlyZero())
						{
							StableAxis = FVector::UpVector;
						}
						StableAxis.Normalize();
						DeltaQuat = FQuat(StableAxis, PI);
					}
					else
					{
						DeltaQuat = FQuat::FindBetweenNormals(ToTip, ToTarget);
					}

					// not use InBaseAngle
					FRotator JointInnerClamp = InBaseAngle;
					float UpLookRatio = UpRotClamp;


					if (bIsHeadUseSeparateClamp && bIsLastJoint)
					{
						JointInnerClamp = FRotator::ZeroRotator;
						MultiplierValue = 1.0f;
						UpLookRatio = 1.0f;
					}

					if (Override)
					{
						if (Override->bOverrideMultiplier)
						{
							MultiplierValue = Override->Multiplier;
						}

						if (Override->bOverrideUpRotClamp)
						{
							UpLookRatio = Override->UpRotClamp;
						}

						if (Override->bOverrideClamp)
						{
							JointInnerClamp = Override->Clamp;
						}

					}

					// MultiplierとClampを適用したIKが求める回転を計算
					FRotator DeltaRot = DeltaQuat.Rotator();
					DeltaRot.Pitch *= UpLookRatio;
					DeltaRot = ClampDeltaRot(DeltaRot, JointInnerClamp);

					WeightedDelta = FQuat::Slerp(
						FQuat::Identity,
						DeltaRot.Quaternion(),
						FMath::Clamp(MultiplierValue, 0.0f, 1.0f)).GetNormalized();
				}


				CurrentTransforms[LinkIndex].SetRotation((WeightedDelta * CurrentTransforms[LinkIndex].GetRotation()).GetNormalized());
				if (RobotArm.bEnable)
				{
					PropagateToChildren(LinkIndex, PivotPos, WeightedDelta);
				}
				else
				{
					UpdateChildPositionsFrom(LinkIndex);
				}

			}

		}


		if (bIsEnableDipMultiplier && !RobotArm.bEnable)
		{
			const FRotator PelvisRot = CurrentTransforms[0].Rotator();
			const float SideAngle = -PelvisRot.Yaw;
			float VerticalAngle = FMath::Abs(PelvisRot.Roll);
			VerticalAngle = FMath::Clamp(VerticalAngle - VerticalDipTreshold, 0.0f, 1000.0f);

			const float VerticalDirectionVal = (CSEffectorLocation.Z > CurrentTransforms.Last().GetLocation().Z) ? 1.0f : -1.0f;

			const float SideDownVal = FMath::Abs(SideAngle);
			const FVector RightDir = FVector::CrossProduct(LookAtAxis.Axis, FVector::UpVector);

			for (int32 i = 0; i < NumTransforms; ++i)
			{
				if (VerticalDirectionVal > 0.0f)
				{
					CurrentTransforms[i].AddToTranslation(FVector::UpVector * DownwardDipMultiplier * VerticalAngle);
				}
				else
				{
					CurrentTransforms[i].AddToTranslation(FVector::UpVector * InvertedDipMultiplier * VerticalAngle);
				}

				CurrentTransforms[i].AddToTranslation(RightDir * SideMultiplier * SideAngle);
				CurrentTransforms[i].AddToTranslation(FVector::UpVector * SideDownMultiplier * SideDownVal);
			}
		}

		// CCD後、EndSplineBone 自体の yaw を Effector に合わせる。
		// CCDは基本的に tip position を近づける処理なので、
		// end bone の look/forward axis が target の yaw を向く保証をここで明示的に作る。
		if (bAlignTipYawToTarget && !RobotArm.bEnable)
		{
			const int32 EndIndex = NumTransforms - 1;
			FTransform& EndTransform = CurrentTransforms[EndIndex];

			FRotator EndClamp = FRotator::ZeroRotator;
			float EndMultiplier = 1.0f;

			const FName EndBoneName = AimHelper::GetBoneNameFromCompactIndex(BoneContainer, BoneIndices[EndIndex]);

			if (bUsePerJointRotationOverrides)
			{
				if (const FAimJointRotationOverride* Override = PerJointRotationOverrides.Find(EndBoneName))
				{
					if (Override->bOverrideClamp)
					{
						EndClamp = Override->Clamp;
					}

					if (Override->bOverrideMultiplier)
					{
						EndMultiplier = Override->Multiplier;
					}
				}
				else
				{
					UE_LOG(LogQuadrupedIK, Warning, TEXT("PerJointRotationOverrides not found EndBoneName : %s"), *EndBoneName.ToString());
				}
			}

			const FVector EndLocation = EndTransform.GetLocation();
			const FVector ToEffector = (CSEffectorLocation - EndLocation).GetSafeNormal();

			if (!ToEffector.IsNearlyZero())
			{
				FVector CurrentAimAxisCS = LookAtAxis.GetTransformedAxis(EndTransform).GetSafeNormal();
				if (CurrentAimAxisCS.IsNearlyZero())
				{
					CurrentAimAxisCS = EndTransform.GetRotation().RotateVector(FVector::ForwardVector).GetSafeNormal();
				}

				FVector UpAxisCS = FVector::UpVector;
				if (CurrentTransforms.IsValidIndex(0))
				{
					const FVector RootUpCS = CurrentTransforms[0].GetRotation().RotateVector(FVector::UpVector).GetSafeNormal();
					if (!RootUpCS.IsNearlyZero())
					{
						UpAxisCS = RootUpCS;
					}
				}

				const FVector CurrentAimYawCS = FVector::VectorPlaneProject(CurrentAimAxisCS, UpAxisCS).GetSafeNormal();
				const FVector TargetYawCS = FVector::VectorPlaneProject(ToEffector, UpAxisCS).GetSafeNormal();

				if (!CurrentAimYawCS.IsNearlyZero() && !TargetYawCS.IsNearlyZero())
				{
					FQuat YawDeltaQuat = FQuat::Identity;

					const float DotYaw = FVector::DotProduct(CurrentAimYawCS, TargetYawCS);
					if (DotYaw < -0.98f)
					{
						YawDeltaQuat = FQuat(UpAxisCS, PI);
					}
					else
					{
						YawDeltaQuat = FQuat::FindBetweenNormals(CurrentAimYawCS, TargetYawCS);
					}

					FRotator DeltaRot = YawDeltaQuat.Rotator();

					// 末端の override clamp をここで適用
					if (EndClamp.Yaw != 0.0f)
					{
						const float AbsYaw = FMath::Abs(EndClamp.Yaw);
						DeltaRot.Yaw = FMath::ClampAngle(DeltaRot.Yaw, -AbsYaw, AbsYaw);
					}

					if (EndClamp.Pitch != 0.0f)
					{
						const float AbsPitch = FMath::Abs(EndClamp.Pitch);
						DeltaRot.Pitch = FMath::ClampAngle(DeltaRot.Pitch, -AbsPitch, AbsPitch);
					}

					if (EndClamp.Roll != 0.0f)
					{
						const float AbsRoll = FMath::Abs(EndClamp.Roll);
						DeltaRot.Roll = FMath::ClampAngle(DeltaRot.Roll, -AbsRoll, AbsRoll);
					}

					const FQuat WeightedDelta = FQuat::Slerp(
						FQuat::Identity,
						DeltaRot.Quaternion(),
						FMath::Clamp(EndMultiplier, 0.0f, 1.0f)).GetNormalized();

					EndTransform.SetRotation((WeightedDelta * EndTransform.GetRotation()).GetNormalized());
				}
			}
		}


		for (int32 i = 0; i < NumTransforms; ++i)
		{
			OutBoneTransforms[i] = FBoneTransform(BoneIndices[i], CurrentTransforms[i]);
		}
	}



	/// <summary>
	/// Modify HandIK
	/// </summary>
	void Evaluate_TwoBoneIK_Direct_Modified(
		FComponentSpacePoseContext& Output,
		const USkeletalMeshComponent* SkeletalMeshComponent,
		const FBoneReference& HandBone,
		const FBoneReference& ElbowBone,
		const FBoneReference& ShoulderBone,
		const FTransform& ThighTransform,
		const FTransform& Shoulder,
		const FTransform& Knee,
		const FTransform& Hand,
		const FVector& JointLocation,
		const FVector& KneePoleOffset,
		const FTransform& TransformOffset,
		const FTransform& CommonSpineModifiedTransform,
		const FRotator& LimbRotationOffset,
		FCustomBone_ArmsData& HandData,
		const float HandClampValue,
		const FTransform& ExtraHandOffset,
		const FVector& ElbowPoleOffset,
		const bool bIsOverrideHandRotation,
		const FTransform& KneeTransformDefault,
		const FVector& LookAtAxis,
		const FVector& ReferenceConstantForwardAxis,
		float& LastShoulderAngle,
		const bool bIsUseNSEWPoles,
		const bool bIsUseUpArmTwist,
		const FVector UpVectorVal,
		const bool bIsSeparateArmsLogicUsed,
		const bool bIsReachMode,
		FArmSolverWorkArea& OutArmSolverWorkArea)
	{

		const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();
		const FCompactPoseBoneIndex& CachedUpperLimbIndex = OutArmSolverWorkArea.ResultShoulder.BoneIndex;
		const FCompactPoseBoneIndex& CachedLowerLimbIndex = OutArmSolverWorkArea.ResultElbow.BoneIndex;
		const FCompactPoseBoneIndex& CachedFeetPoseIndex = OutArmSolverWorkArea.ResultHand.BoneIndex;

		FCompactPoseBoneIndex CachedClavicleIndex = FCompactPoseBoneIndex(0);

		FTransform ClavicleOffset = FTransform::Identity;
		FTransform ClavicleTransform = FTransform::Identity;

		bool bIsShoulderTwistAvailable = false;

		if (OutArmSolverWorkArea.bHasClavicle && bIsSeparateArmsLogicUsed)
		{
			bIsShoulderTwistAvailable = true;
			CachedClavicleIndex = OutArmSolverWorkArea.ResultClavicle.BoneIndex;

			ClavicleTransform = Output.Pose.GetComponentSpaceTransform(CachedClavicleIndex) * TransformOffset;
			const FTransform OrigClavicleTransform = ClavicleTransform;
			const FVector ClavicleDirection = (ClavicleTransform.GetLocation() - Shoulder.GetLocation()).GetSafeNormal();
			const FVector ClavicleEffectorDirection = (ClavicleTransform.GetLocation() - ThighTransform.GetLocation()).GetSafeNormal();
			FRotator ClavicleRotDifference = FQuat::FindBetweenVectors(ClavicleDirection, ClavicleEffectorDirection).Rotator();

			ClavicleRotDifference.Pitch = FMath::ClampAngle(ClavicleRotDifference.Pitch,
				FMath::Abs(HandData.InnerClavicle_VLimit.Y),
				-FMath::Abs(HandData.InnerClavicle_VLimit.X));

			ClavicleRotDifference.Yaw = FMath::ClampAngle(ClavicleRotDifference.Yaw,
				FMath::Abs(HandData.InnerClavicle_HLimit.X),
				-FMath::Abs(HandData.InnerClavicle_HLimit.Y));

			if (ClavicleRotDifference.Pitch > 0)
			{
				ClavicleRotDifference.Pitch = ClavicleRotDifference.Pitch - FMath::Abs(HandData.InnerClavicle_VLimit.Y);
			}
			else
			{
				ClavicleRotDifference.Pitch = ClavicleRotDifference.Pitch + FMath::Abs(HandData.InnerClavicle_VLimit.X);
			}

			if (ClavicleRotDifference.Yaw > 0)
			{
				ClavicleRotDifference.Yaw = ClavicleRotDifference.Yaw - FMath::Abs(HandData.InnerClavicle_HLimit.X);
			}
			else
			{
				ClavicleRotDifference.Yaw = ClavicleRotDifference.Yaw + FMath::Abs(HandData.InnerClavicle_HLimit.Y);
			}

			ClavicleRotDifference.Roll = 0;
			ClavicleRotDifference.Pitch = FMath::ClampAngle(ClavicleRotDifference.Pitch,
				-FMath::Abs(HandData.OuterClavicle_VLimit.X), FMath::Abs(HandData.OuterClavicle_VLimit.Y));
			ClavicleRotDifference.Yaw = FMath::ClampAngle(ClavicleRotDifference.Yaw,
				-FMath::Abs(HandData.OuterClavicle_HLimit.X), FMath::Abs(HandData.OuterClavicle_HLimit.Y));

			if (ClavicleRotDifference.Pitch != -FMath::Abs(HandData.OuterClavicle_VLimit.X) &&
				ClavicleRotDifference.Pitch != FMath::Abs(HandData.OuterClavicle_VLimit.Y))
			{
				HandData.LastClavicleRotation.Pitch = ClavicleRotDifference.Pitch;
			}

			if (ClavicleRotDifference.Yaw != -FMath::Abs(HandData.OuterClavicle_HLimit.X) &&
				ClavicleRotDifference.Yaw != FMath::Abs(HandData.OuterClavicle_HLimit.Y))
			{
				HandData.LastClavicleRotation.Yaw = ClavicleRotDifference.Yaw;
			}

			HandData.LastClavicleRotation.Roll = ClavicleRotDifference.Roll;
			ClavicleTransform.SetRotation(HandData.LastClavicleRotation.Quaternion() * ClavicleTransform.GetRotation());
			ClavicleOffset = Output.Pose.GetComponentSpaceTransform(CachedClavicleIndex).Inverse() * ClavicleTransform;

			OutArmSolverWorkArea.ResultClavicle.Transform = ClavicleTransform;
			OutArmSolverWorkArea.OrigClavicle.Transform = OrigClavicleTransform;
		}

		// Get Local Space transforms for our bones. We do this first in case they already are local.
		// As right after we get them in component space. (And that does the auto conversion).
		// We might save one transform by doing local first...
		const FTransform& EndBoneLocalTransform = Output.Pose.GetLocalSpaceTransform(CachedFeetPoseIndex);
		const FTransform& LowerLimbLocalTransform = Output.Pose.GetLocalSpaceTransform(CachedLowerLimbIndex);
		const FTransform& UpperLimbLocalTransform = Output.Pose.GetLocalSpaceTransform(CachedUpperLimbIndex);

		// Now get those in component space...
		FTransform LowerLimbCSTransform = Knee;
		FTransform UpperLimbCSTransform = Shoulder;
		FTransform EndBoneCSTransform = Hand;

		FTransform OrigLowerLimbCSTransform = Knee;
		FTransform OrigUpperLimbCSTransform = Shoulder;
		FTransform OrigEndBoneCSTransform = Hand;

		if (OutArmSolverWorkArea.bHasClavicle && bIsSeparateArmsLogicUsed)
		{
			LowerLimbCSTransform = Output.Pose.GetComponentSpaceTransform(CachedLowerLimbIndex) * ClavicleOffset;
			UpperLimbCSTransform = Output.Pose.GetComponentSpaceTransform(CachedUpperLimbIndex) * ClavicleOffset;
			EndBoneCSTransform = Output.Pose.GetComponentSpaceTransform(CachedFeetPoseIndex) * ClavicleOffset;

			OrigLowerLimbCSTransform = Output.Pose.GetComponentSpaceTransform(CachedLowerLimbIndex) * ClavicleOffset;
			OrigUpperLimbCSTransform = Output.Pose.GetComponentSpaceTransform(CachedUpperLimbIndex) * ClavicleOffset;
			OrigEndBoneCSTransform = Output.Pose.GetComponentSpaceTransform(CachedFeetPoseIndex) * ClavicleOffset;
		}

		FTransform OriginalElbowTransform = Output.Pose.GetComponentSpaceTransform(CachedLowerLimbIndex);
		const FTransform EndBoneCSTransform_Const = Output.Pose.GetComponentSpaceTransform(CachedFeetPoseIndex);
		FTransform EndBoneCSTransform_Always = EndBoneCSTransform_Const;
		EndBoneCSTransform.SetRotation(EndBoneCSTransform_Const.GetRotation());

		const FVector& RootPos = UpperLimbCSTransform.GetTranslation();
		const FVector& InitialJointPos = LowerLimbCSTransform.GetTranslation();
		const FVector& InitialEndPos = EndBoneCSTransform.GetTranslation();

		FTransform EffectorTransform = EndBoneCSTransform;

		FTransform PoleOrigTransform = FTransform::Identity;
		PoleOrigTransform.SetLocation(ElbowPoleOffset);

		FTransform PoleRefTransform = FTransform::Identity;
		PoleRefTransform.SetLocation(UpperLimbCSTransform.GetLocation());

		{
			const FQuat PoleRotDiff = FQuat::FindBetweenVectors((ThighTransform.GetLocation() - UpperLimbCSTransform.GetLocation()),
				(EffectorTransform.GetLocation() - UpperLimbCSTransform.GetLocation()));
			PoleRefTransform.SetRotation(PoleRotDiff);
		}

		// Get joint target (used for defining plane that joint should be in).
		FTransform JointTargetTransform = LowerLimbCSTransform;

		FQuat Forward_Rotation_Difference = FQuat::FindBetweenNormals(LookAtAxis, ReferenceConstantForwardAxis);
		FTransform FRP_Knee_Transform = FTransform::Identity;
		FRP_Knee_Transform.SetRotation(Forward_Rotation_Difference);
		FTransform Pole_Transform = FTransform::Identity;
		Pole_Transform.SetLocation(ElbowPoleOffset);

		FTransform Second_Pole_Transform = FTransform::Identity;
		Second_Pole_Transform.SetLocation(KneePoleOffset);

		FVector CS_Forward = (
			(
				(UpperLimbCSTransform.GetLocation() + EndBoneCSTransform.GetLocation() + LowerLimbCSTransform.GetLocation()) / 3) -
			(LowerLimbCSTransform.GetLocation() + Pole_Transform.GetLocation())).GetSafeNormal();

		if (bIsReachMode)
		{
			if (bIsUseNSEWPoles)
			{
				FVector forward_dir = LookAtAxis;
				FVector right_dir = FVector::CrossProduct(LookAtAxis, UpVectorVal);
				FVector Hand_Shoulder_dir = (ThighTransform.GetLocation() - UpperLimbCSTransform.GetLocation()).GetSafeNormal();
				float NS_Alpha = UKismetMathLibrary::DegAcos(FVector::DotProduct(forward_dir, Hand_Shoulder_dir)) / 180;
				float EW_Alpha = UKismetMathLibrary::DegAcos(FVector::DotProduct(right_dir, Hand_Shoulder_dir)) / 180;
				FVector NS_Aggregated_Pole;
				FVector EW_Aggregated_Pole;
				FVector Total_Aggregated_Pole;

				if (HandData.bIsRightHand)
				{
					NS_Aggregated_Pole = FMath::Lerp(HandData.NorthPoleOffset * 10, HandData.SouthPoleOffset * 10, EW_Alpha);
					EW_Aggregated_Pole = FMath::Lerp(HandData.EastPoleOffset * 10, HandData.WestPoleOffset * 10, NS_Alpha);
					Total_Aggregated_Pole = (NS_Aggregated_Pole + EW_Aggregated_Pole) / 2;
				}
				else
				{
					NS_Aggregated_Pole = FMath::Lerp(HandData.SouthPoleOffset * 10, HandData.NorthPoleOffset * 10, EW_Alpha);
					EW_Aggregated_Pole = FMath::Lerp(HandData.WestPoleOffset * 10, HandData.EastPoleOffset * 10, NS_Alpha);
					Total_Aggregated_Pole = (NS_Aggregated_Pole + EW_Aggregated_Pole) / 2;
				}
				JointTargetTransform.SetLocation(Total_Aggregated_Pole);
			}
			else
			{
				JointTargetTransform.SetLocation(OriginalElbowTransform.GetLocation() + Pole_Transform.GetLocation() * 10);
			}
		}
		else
		{
			JointTargetTransform.SetLocation(JointTargetTransform.GetLocation() + CS_Forward * -1000);
		}

		FVector	JointTargetPos = JointTargetTransform.GetLocation();
		// IK solver
		UpperLimbCSTransform.SetLocation(RootPos);
		LowerLimbCSTransform.SetLocation(InitialJointPos);
		EndBoneCSTransform.SetLocation(InitialEndPos);

		// This is our reach goal.
		FVector DesiredPos = ThighTransform.GetLocation();
		FVector Far_Target = CommonSpineModifiedTransform.GetLocation();
		FVector DesiredThighPos = SkeletalMeshComponent->GetComponentToWorld().InverseTransformPosition(ThighTransform.GetLocation());
		float WristOffset = 90;

		if (HandData.bIsRightHand)
		{
			WristOffset = -90;
		}

		AimHelper::Solve_Modified_Direct_TwoBoneIK_4(
			SkeletalMeshComponent->GetComponentTransform(),
			WristOffset,
			UpperLimbCSTransform,
			LowerLimbCSTransform,
			EndBoneCSTransform,
			JointTargetPos, DesiredPos,
			false, 1.0f, 1.0f, bIsUseUpArmTwist);


		FRotator InputArmRot = FRotator::ZeroRotator;
		float RollAlpha = 1.0f;
		float RollAbsolute = 0.0f;
		float RollLimit = 60.0f;

		// Update transform for end bone.
		{
			if (bIsOverrideHandRotation)
			{
				FQuat Default_Rot = (EndBoneLocalTransform * LowerLimbCSTransform).GetRotation();
				FTransform Unmodifed_Hand = (EndBoneLocalTransform * LowerLimbCSTransform);
				EndBoneCSTransform.SetRotation(ThighTransform.GetRotation() * EndBoneCSTransform.GetRotation());

				InputArmRot = GetHandYaw(
					true,
					HandData,
					SkeletalMeshComponent->GetComponentToWorld(),
					Output.Pose.GetComponentSpaceTransform(CachedLowerLimbIndex),
					Output.Pose.GetComponentSpaceTransform(CachedFeetPoseIndex),
					LowerLimbCSTransform,
					EndBoneCSTransform,
					Unmodifed_Hand,
					UpVectorVal);
			}
			else
			{
				if (ExtraHandOffset.Equals(FTransform::Identity))
				{
					EndBoneCSTransform.SetRotation((EndBoneLocalTransform * LowerLimbCSTransform).GetRotation());
				}
				else
				{
					EndBoneCSTransform.SetRotation(ExtraHandOffset.GetRotation() * EndBoneCSTransform.GetRotation());
				}
			}

			if (!HandData.bIsAccurateHandRotation)
			{
				OutArmSolverWorkArea.ResultHand.Transform = EndBoneCSTransform;
				OutArmSolverWorkArea.OrigHand.Transform = OrigEndBoneCSTransform;

			}
			else
			{
				FAxis AxisInstance;
				AxisInstance.bInLocalSpace = !HandData.bIsRelativeAxis;
				AxisInstance.Axis = HandData.LocalDirectionAxis;
				EndBoneCSTransform_Always.SetLocation(EndBoneCSTransform.GetLocation());

				FTransform EndBoneCSTransformAccurate = LookAt_Processor_Helper(
					EndBoneCSTransform_Always,
					EndBoneCSTransform_Always.GetLocation(),
					Far_Target, AxisInstance,
					HandClampValue,
					FRotator::ZeroRotator,
					true,
					1.0f,
					1.0f);

				OutArmSolverWorkArea.ResultHand.Transform = EndBoneCSTransformAccurate;
				OutArmSolverWorkArea.OrigHand.Transform = OrigEndBoneCSTransform;

			}
		}

		FTransform BS_EndBoneCSTransform = EndBoneCSTransform_Const.Inverse() * EndBoneCSTransform;
		FTransform BS_LowerLimbTransform = LowerLimbCSTransform;
		FTransform BS_UpperLimbTransform = UpperLimbCSTransform;
		FVector Arm_Direction = (LowerLimbCSTransform.GetLocation() - EndBoneCSTransform.GetLocation()).GetSafeNormal();
		FTransform Reference_Parent = FTransform::Identity;
		Reference_Parent.SetRotation(Arm_Direction.ToOrientationQuat());

		BS_LowerLimbTransform = SetArmYaw(
			HandData.bIsInvertLowerTwist,
			HandData.bIsRightHand,
			InputArmRot.Roll,
			SkeletalMeshComponent->GetComponentToWorld(),
			Output.Pose.GetComponentSpaceTransform(CachedLowerLimbIndex),
			Output.Pose.GetComponentSpaceTransform(CachedFeetPoseIndex),
			LowerLimbCSTransform,
			EndBoneCSTransform);

		float ForeArmAngle = UKismetMathLibrary::RadiansToDegrees((BS_LowerLimbTransform.GetRotation().Inverse() * LowerLimbCSTransform.GetRotation()).GetAngle());
		ForeArmAngle = FMath::UnwindDegrees(ForeArmAngle);
		RollLimit = 70.f;
		ForeArmAngle = FMath::ClampAngle(ForeArmAngle, -RollLimit, RollLimit);

		if (ForeArmAngle <= RollLimit)
		{
			RollAlpha = 1.0f;
		}
		else
		{
			RollAlpha = 0.0f;
		}

		float StartClamp = 25.0f;
		float EndClamp = -25.0f;

		InputArmRot.Roll = FMath::UnwindDegrees(InputArmRot.Roll);

		FVector2D ForearmLimitAbs = HandData.ForeArmAngleLimit;
		ForearmLimitAbs.X = FMath::Clamp(FMath::Abs(ForearmLimitAbs.X), 0.01f, 179.9f);
		ForearmLimitAbs.Y = FMath::Clamp(FMath::Abs(ForearmLimitAbs.Y), 0.01f, 179.9f);

		if (ForearmLimitAbs.X > 0 && ForearmLimitAbs.Y > 0)
		{
			InputArmRot.Roll = FMath::ClampAngle(InputArmRot.Roll, -ForearmLimitAbs.X, ForearmLimitAbs.Y);
			if (InputArmRot.Roll != -ForearmLimitAbs.X && InputArmRot.Roll != ForearmLimitAbs.Y)
			{
				HandData.LastForarmAngle = InputArmRot.Roll;
			}
		}
		else
		{
			HandData.LastForarmAngle = InputArmRot.Roll;
		}

		BS_LowerLimbTransform = SetArmYaw(
			HandData.bIsInvertLowerTwist,
			HandData.bIsRightHand,
			HandData.LastForarmAngle,
			SkeletalMeshComponent->GetComponentToWorld(),
			Output.Pose.GetComponentSpaceTransform(CachedLowerLimbIndex),
			Output.Pose.GetComponentSpaceTransform(CachedFeetPoseIndex),
			LowerLimbCSTransform,
			EndBoneCSTransform);

		const FVector ShoulderArmDir = (UpperLimbCSTransform.GetLocation() - LowerLimbCSTransform.GetLocation()).GetSafeNormal();
		const FVector ArmHandDir = (LowerLimbCSTransform.GetLocation() - EndBoneCSTransform.GetLocation()).GetSafeNormal();
		float ArmAngle = (1 - 1.5f * (UKismetMathLibrary::DegAcos(FVector::DotProduct(ArmHandDir, ShoulderArmDir)) / 180.0f));

		ArmAngle = FMath::Clamp(ArmAngle, 0.0f, 1.0f);

		FRotator ForArmInputRot = GetHandYaw(
			false,
			HandData,
			SkeletalMeshComponent->GetComponentToWorld(),
			Output.Pose.GetComponentSpaceTransform(CachedUpperLimbIndex),
			Output.Pose.GetComponentSpaceTransform(CachedLowerLimbIndex),
			BS_UpperLimbTransform,
			BS_LowerLimbTransform,
			BS_LowerLimbTransform,
			UpVectorVal);

		FVector2D LocalShoulderInnerRange = HandData.ShoulderInnerRange;
		LocalShoulderInnerRange.X = FMath::Abs(LocalShoulderInnerRange.X);
		LocalShoulderInnerRange.Y = FMath::Abs(LocalShoulderInnerRange.Y);

		if (LocalShoulderInnerRange.X > 0 && LocalShoulderInnerRange.Y > 0)
		{
			ForArmInputRot.Roll = FMath::ClampAngle(ForArmInputRot.Roll, LocalShoulderInnerRange.X, -LocalShoulderInnerRange.Y);

			if (ForArmInputRot.Roll > 0)
			{
				ForArmInputRot.Roll = ForArmInputRot.Roll - LocalShoulderInnerRange.X;
			}
			else
			{
				ForArmInputRot.Roll = ForArmInputRot.Roll + LocalShoulderInnerRange.Y;
			}
		}

		FVector2D ShoulderOuterClamps = HandData.ShoulderOuterRange;
		ShoulderOuterClamps.X = FMath::Clamp(FMath::Abs(ShoulderOuterClamps.X), 0.01f, 179.9f);
		ShoulderOuterClamps.Y = FMath::Clamp(FMath::Abs(ShoulderOuterClamps.Y), 0.01f, 179.9f);
		ForArmInputRot.Roll = FMath::ClampAngle(ForArmInputRot.Roll, -ShoulderOuterClamps.X, ShoulderOuterClamps.Y);
		if (ForArmInputRot.Roll != -ShoulderOuterClamps.X && ForArmInputRot.Roll != ShoulderOuterClamps.Y)
		{
			LastShoulderAngle = ForArmInputRot.Roll;
		}

		{

			BS_UpperLimbTransform = AimHelper::SetArmYaw(
				HandData.bIsInvertUpperTwist,
				HandData.bIsRightHand,
				LastShoulderAngle * ArmAngle,
				SkeletalMeshComponent->GetComponentToWorld(),
				Output.Pose.GetComponentSpaceTransform(CachedUpperLimbIndex),
				Output.Pose.GetComponentSpaceTransform(CachedLowerLimbIndex),
				BS_UpperLimbTransform,
				BS_LowerLimbTransform);

			OutArmSolverWorkArea.ResultShoulder.Transform = BS_UpperLimbTransform;
			OutArmSolverWorkArea.OrigShoulder.Transform = OrigUpperLimbCSTransform;

		}

		OutArmSolverWorkArea.ResultElbow.Transform = BS_LowerLimbTransform;
		OutArmSolverWorkArea.OrigElbow.Transform = OrigLowerLimbCSTransform;
	}


}

FAnimNode_CustomAimSolver::FAnimNode_CustomAimSolver()
{
	LerpedLookatLocation = FVector::ZeroVector;
	FRichCurve* Look_Bending_CurveData = LookBendingCurve.GetRichCurve();
	Look_Bending_CurveData->AddKey(0.f, 0.02f);
	Look_Bending_CurveData->AddKey(0.5f, 0.5f);
	Look_Bending_CurveData->AddKey(1.f, 1.f);

	FRichCurve* Look_Multiplier_CurveData = LookMultiplierCurve.GetRichCurve();
	Look_Multiplier_CurveData->AddKey(0.f, 0.02f);
	Look_Multiplier_CurveData->AddKey(0.5f, 0.5f);
	Look_Multiplier_CurveData->AddKey(1.f, 1.0f);

	FRichCurve* Distance_Alpha_CurveData = DistanceAlphaCurve.GetRichCurve();
	Distance_Alpha_CurveData->AddKey(0.0f, 0.02f);
	Distance_Alpha_CurveData->AddKey(1.0f, 1500.0f);

#if WITH_EDITOR
	if (DebugHandTransforms.IsEmpty())
	{
		ResizeDebugLocations(2);
	}
	DebugLookAtTransform.SetLocation(FVector(200.0f, 0.0f, 100.0f));
#endif

}


void FAnimNode_CustomAimSolver::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	Super::Initialize_AnyThread(Context);

	if (Context.AnimInstanceProxy)
	{
		owning_skel = Context.AnimInstanceProxy->GetSkelMeshComponent();
		IgnoreActors.Add(Context.AnimInstanceProxy->GetSkelMeshComponent()->GetOwner());

	}
}

void FAnimNode_CustomAimSolver::ConditionalDebugDraw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* PreviewSkelMeshComp) const
{

#if WITH_EDITORONLY_DATA

	const bool bIsValid = (PreviewSkelMeshComp && PreviewSkelMeshComp->GetWorld());

	if (!bIsValid)
	{
		return;
	}

	if (bDisplayLineTrace)
	{
		const float CurOwnerScale = ComponentScale;

		const FTransform& ComponentToWorld = PreviewSkelMeshComp->GetComponentTransform();
		FVector CharacterUpWS = ComponentToWorld.GetUnitAxis(EAxis::Z);
		FQuat DebugRotation = FRotationMatrix::MakeFromZ(CharacterUpWS).ToQuat();

		for (int32 Index = 0; Index < TraceStartList.Num(); Index++)
		{
			const float SelectedRadius = (TraceRadiusList[Index] > 0.0f) ? TraceRadiusList[Index] : Trace_Radius;
			const float ScaledTraceRadius = SelectedRadius * CurOwnerScale;

			const FVector& Start = TraceStartList[Index];
			const FVector& End = TraceEndList[Index];
			const FVector& Center = (Start + End) * 0.5f;
			const FVector& TraceVector = Start - End;
			const float HalfHeight = TraceVector.Size() * 0.5f;

			switch (RaycastTraceType)
			{
			case EIKRaycastType::LineTrace:
			{
				DrawDebugLine(PreviewSkelMeshComp->GetWorld(), Start, End, FColor::Red, false, 0.1f);
			}
			break;

			case EIKRaycastType::SphereTrace:
			{
				DrawDebugCapsule(PreviewSkelMeshComp->GetWorld(), Center, HalfHeight, ScaledTraceRadius, DebugRotation, FColor::Red, false, 0.1f);
			}
			break;

			case EIKRaycastType::BoxTrace:
			{
				const FVector& BoxExtent = FVector(ScaledTraceRadius, ScaledTraceRadius, HalfHeight);
				DrawDebugBox(PreviewSkelMeshComp->GetWorld(), Center, BoxExtent, DebugRotation, FColor::Red, false, 0.1f);
			}
			break;
			}
		}
	}


	//if (bDisplayHorizontalAngleDebug)
	//{
	//	const UWorld* PreviewWorld = PreviewSkelMeshComp->GetWorld();
	//	if (PreviewWorld && !PreviewWorld->IsGameWorld())
	//	{
	//		DrawHorizontalAngleDebugArc(PreviewSkelMeshComp, PDI, nullptr);
	//	}
	//}

#endif


}

void FAnimNode_CustomAimSolver::UpdateInternal(const FAnimationUpdateContext& Context)
{
	TraceStartList.Empty();
	TraceEndList.Empty();

	const float MinValue = 0.25f;
	const float MaxValue = 100.0f;
	InterpolationSpeed = FMath::Clamp(InterpolationSpeed, MinValue, MaxValue);

	CachedDeltaSeconds = Context.GetDeltaTime();

	const FTransform& ComponentToWorld = Context.AnimInstanceProxy->GetComponentTransform();
	USkeletalMeshComponent* SK = Context.AnimInstanceProxy->GetSkelMeshComponent();
	AActor* Owner = Context.AnimInstanceProxy->GetSkelMeshComponent()->GetOwner();

	const bool bIsTurnOnSolver = (bIsAdaptiveTerrain) ? bIsEnableSolver && AimHitResult.bBlockingHit : bIsEnableSolver;

	const float AlphaVal = (bIsTurnOnSolver) ? 1.0f : 0.f;

	const float Factor = (1.0f - FMath::Exp(-SmoothFactor * CachedDeltaSeconds));
	HeadActualAlpha = FMath::FInterpTo(HeadActualAlpha, AlphaVal, Factor, InterpolationSpeed);


	if (bIsAllowSmoothingEnableSolver)
	{
		if (bIsPrevEnableSolver && !bIsEnableSolver)
		{
			// on -> off
			bSolverFadeOutOnly = true;
		}
		else if (bIsEnableSolver)
		{
			// off -> on
			bSolverFadeOutOnly = false;
		}

		bIsPrevEnableSolver = bIsEnableSolver;
	}
	else
	{
		bSolverFadeOutOnly = false;
		bIsPrevEnableSolver = bIsEnableSolver;
	}

	ComponentScale = IsValid(Owner) ? Owner->GetActorScale3D().Z : ComponentToWorld.GetScale3D().Z;
	ComponentScale = FMath::Max(ComponentScale, 0.01f);


	FVector ClampOriginWS = ComponentToWorld.TransformPosition(HeadOrigTransform.GetLocation());
	if (bUseRootAsClampOrigin)
	{
		ClampOriginWS = ComponentToWorld.TransformPosition(OriginTransform.GetLocation());
	}

	const FVector ClampForwardWS = ComponentToWorld.TransformVector(ForwardDirectionVector).GetSafeNormal();
	const FVector RawTargetWS = LookAtLocation.GetLocation();

	const float RawHorizontalDeg = AimHelper::GetSignedHorizontalAngleDeg(
		ClampForwardWS,
		ClampOriginWS,
		RawTargetWS);

	FVector ClampedTargetWS = FVector::ZeroVector;

	if (bIsNotLimitMethod)
	{
		ClampedTargetWS = AimHelper::ClampRotateVector_NoLimit(
			RawTargetWS,
			ClampForwardWS,
			ClampOriginWS,
			VerticalRangeAngles.X,
			VerticalRangeAngles.Y,
			HorizontalRangeAngles.X,
			HorizontalRangeAngles.Y,
			bIsUseNaturalMethod);
	}
	else
	{
		ClampedTargetWS = AimHelper::ClampRotateVector(
			RawTargetWS,
			ClampForwardWS,
			ClampOriginWS,
			VerticalRangeAngles.X,
			VerticalRangeAngles.Y,
			HorizontalRangeAngles.X,
			HorizontalRangeAngles.Y,
			bIsUseNaturalMethod);
	}


	//#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
#if false
	if (AimAnimationCommand::CVarDebugAimIK.GetValueOnAnyThread() > 0)
	{
		bDisplayHorizontalAngleDebug = true;
	}
	else
	{
		bDisplayHorizontalAngleDebug = false;
	}

	if (bDisplayHorizontalAngleDebug)
	{
		const FVector RootOriginWS = ComponentToWorld.TransformPosition(OriginTransform.GetLocation());

		DebugClampOriginWS = RootOriginWS;
		DebugClampForwardWS = ClampForwardWS;
		DebugClampRawTargetWS = RawTargetWS;
		DebugClampResultTargetWS = ClampedTargetWS;
		DebugHorizontalRawDeg = RawHorizontalDeg;
		DebugHorizontalClampedDeg = AimHelper::GetSignedHorizontalAngleDeg(
			ClampForwardWS,
			ClampOriginWS,
			ClampedTargetWS);

		DebugHorizontalClampMinDeg = HorizontalRangeAngles.X;
		DebugHorizontalClampMaxDeg = HorizontalRangeAngles.Y;

		if (SK && SK->GetWorld() && SK->GetWorld()->IsGameWorld())
		{
			DrawHorizontalAngleDebugArc(SK, nullptr, Context.AnimInstanceProxy);
		}
	}
#endif


}

bool FAnimNode_CustomAimSolver::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones)
{

	if (!bIsEnableSolver && !bIsAllowSmoothingEnableSolver)
	{
		return false;
	}

	for (int32 i = 0; i < AimingHandLimbs.Num(); ++i)
	{
		const FCustomBone_ArmsData& ArmData = AimingHandLimbs[i];

		// @NOTE
		// dont must need to setting ClavicleBone
		// !ArmData.ClavicleBone.IsValidToEvaluate(RequiredBones) || 
		if (!ArmData.ShoulderBone.IsValidToEvaluate(RequiredBones) ||
			!ArmData.ElbowBone.IsValidToEvaluate(RequiredBones) ||
			!ArmData.HandBone.IsValidToEvaluate(RequiredBones))
		{
			return false;
		}
	}


	return (EndSplineBone.IsValidToEvaluate(RequiredBones) &&
		StartSplineBone.IsValidToEvaluate(RequiredBones) &&
		RequiredBones.IsValid() &&
		(RequiredBones.BoneIsChildOf(EndSplineBone.BoneIndex, StartSplineBone.BoneIndex)));
}

void FAnimNode_CustomAimSolver::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{

	EndSplineBone.Initialize(RequiredBones);
	StartSplineBone.Initialize(RequiredBones);

	TipReferenceBone.Initialize(RequiredBones);

	CombinedIndices.Empty();

	HandBoneArray.Empty();
	ElbowBoneArray.Empty();
	ShoulderBoneArray.Empty();
	ActualShoulderBoneArray.Empty();

	LastShoulderAngles.Empty();
	ElbowBoneTransformArray.Empty();
	HandDefaultTransformArray.Empty();

	HeadTransforms.Empty();

	ArmSolverWorkArea.Empty();

#if WITH_EDITOR
	if (DebugHandTransforms.Num() < AimingHandLimbs.Num())
	{
		ResizeDebugLocations(AimingHandLimbs.Num());
	}
#endif


	bIsArmsEnable = (!AimingHandLimbs.IsEmpty());

	for (int32 i = 0; i < AimingHandLimbs.Num(); ++i)
	{
		FCustomBone_ArmsData& ArmData = AimingHandLimbs[i];
		ArmData.ClavicleBone.Initialize(RequiredBones);
		ArmData.ShoulderBone.Initialize(RequiredBones);
		ArmData.ElbowBone.Initialize(RequiredBones);
		ArmData.HandBone.Initialize(RequiredBones);

		HandBoneArray.Add(ArmData.HandBone);
		ElbowBoneArray.Add(ArmData.ElbowBone);
		ShoulderBoneArray.Add(ArmData.ShoulderBone);
		ActualShoulderBoneArray.Add(ArmData.ShoulderBone);
	}

	// Gather all bone indices between root and tip.
	TArray<FCompactPoseBoneIndex> BoneIndices;

	{

		const FCompactPoseBoneIndex RootIndex = StartSplineBone.GetCompactPoseIndex(RequiredBones);
		FCompactPoseBoneIndex BoneIndex = EndSplineBone.GetCompactPoseIndex(RequiredBones);
		do
		{
			BoneIndices.Insert(BoneIndex, 0);
			BoneIndex = RequiredBones.GetParentBoneIndex(BoneIndex);
		} while (BoneIndex != RootIndex);
		BoneIndices.Insert(BoneIndex, 0);
	}

	const int32 NumHeadBones = BoneIndices.Num();
	HeadTransforms.SetNumUninitialized(NumHeadBones);
	for (int32 i = 0; i < NumHeadBones; ++i)
	{
		HeadTransforms[i] = FBoneTransform(BoneIndices[i], FTransform::Identity);
	}

	const int32 NumArms = AimingHandLimbs.Num();
	ElbowBoneTransformArray.SetNumUninitialized(NumArms);
	HandDefaultTransformArray.SetNumUninitialized(NumArms);
	LastShoulderAngles.SetNumZeroed(NumArms);

	ArmSolverWorkArea.SetNumUninitialized(NumArms);

	for (int32 i = 0; i < NumArms; ++i)
	{
		ArmSolverWorkArea[i].Initialize(RequiredBones, AimingHandLimbs[i]);
	}

	bIsDebugHandsInitialized = true;
}


void FAnimNode_CustomAimSolver::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{
	SCOPE_CYCLE_COUNTER(STAT_CustomAimSolver_EvalSKelControl);

	check(OutBoneTransforms.Num() == 0);

	SolveAimIKSystem(Output.Pose, Output, OutBoneTransforms);

	for (int32 i = 0; i < HeadTransforms.Num(); i++)
	{
		OutBoneTransforms.Add(HeadTransforms[i]);
	}

	for (int32 i = 0; i < ArmSolverWorkArea.Num(); i++)
	{
		FArmSolverWorkArea& WorkData = ArmSolverWorkArea[i];

		if (!WorkData.bIsInitialized)
		{
			WorkData.CurrentClavicle = WorkData.ResultClavicle.Transform;
			WorkData.CurrentShoulder = WorkData.ResultShoulder.Transform;
			WorkData.CurrentElbow = WorkData.ResultElbow.Transform;
			WorkData.CurrentHand = WorkData.ResultHand.Transform;
			WorkData.bIsInitialized = true;
		}

		const float Factor = (1.0f - FMath::Exp(-SmoothFactor * CachedDeltaSeconds));

		if (bIsEnableHandInterpolation)
		{
			WorkData.CurrentHand.SetLocation(FMath::VInterpTo(WorkData.CurrentHand.GetLocation(), WorkData.ResultHand.Transform.GetLocation(), Factor, HandInterpolationSpeed));
			WorkData.CurrentHand.SetRotation(FMath::QInterpTo(WorkData.CurrentHand.GetRotation(), WorkData.ResultHand.Transform.GetRotation(), Factor, HandInterpolationSpeed));

			if (WorkData.bHasClavicle)
			{
				WorkData.CurrentClavicle.SetLocation(FMath::VInterpTo(WorkData.CurrentClavicle.GetLocation(), WorkData.ResultClavicle.Transform.GetLocation(), Factor, HandInterpolationSpeed));
				WorkData.CurrentClavicle.SetRotation(FMath::QInterpTo(WorkData.CurrentClavicle.GetRotation(), WorkData.ResultClavicle.Transform.GetRotation(), Factor, HandInterpolationSpeed));
			}

			WorkData.CurrentShoulder.SetLocation(FMath::VInterpTo(WorkData.CurrentShoulder.GetLocation(), WorkData.ResultShoulder.Transform.GetLocation(), Factor, HandInterpolationSpeed));
			WorkData.CurrentShoulder.SetRotation(FMath::QInterpTo(WorkData.CurrentShoulder.GetRotation(), WorkData.ResultShoulder.Transform.GetRotation(), Factor, HandInterpolationSpeed));

			WorkData.CurrentElbow.SetLocation(FMath::VInterpTo(WorkData.CurrentElbow.GetLocation(), WorkData.ResultElbow.Transform.GetLocation(), Factor, HandInterpolationSpeed));
			WorkData.CurrentElbow.SetRotation(FMath::QInterpTo(WorkData.CurrentElbow.GetRotation(), WorkData.ResultElbow.Transform.GetRotation(), Factor, HandInterpolationSpeed));
		}
		else
		{
			WorkData.CurrentClavicle = WorkData.ResultClavicle.Transform;
			WorkData.CurrentShoulder = WorkData.ResultShoulder.Transform;
			WorkData.CurrentElbow = WorkData.ResultElbow.Transform;
			WorkData.CurrentHand = WorkData.ResultHand.Transform;
		}

		if (WorkData.bHasClavicle)
		{
			OutBoneTransforms.Add(FBoneTransform(WorkData.ResultClavicle.BoneIndex, WorkData.CurrentClavicle));
		}

		OutBoneTransforms.Add(FBoneTransform(WorkData.ResultShoulder.BoneIndex, WorkData.CurrentShoulder));
		OutBoneTransforms.Add(FBoneTransform(WorkData.ResultElbow.BoneIndex, WorkData.CurrentElbow));
		OutBoneTransforms.Add(FBoneTransform(WorkData.ResultHand.BoneIndex, WorkData.CurrentHand));
	}


	float TargetFinalAlpha = HeadActualAlpha * AnimCurveAlpha * DistanceAlpha;

	if (!bIsEnableSolver)
	{
		TargetFinalAlpha = 0.0f;
	}

	SolverBlendAlphaSmoothed = FMath::FInterpTo(SolverBlendAlphaSmoothed, TargetFinalAlpha, CachedDeltaSeconds, FinalAlphaInterpSpeed);


	if (!bIsEnableSolver && bSolverFadeOutOnly && SolverBlendAlphaSmoothed <= SolverDisableCompleteThreshold)
	{
		SolverBlendAlphaSmoothed = 0.0f;
		bSolverFadeOutOnly = false;
	}

	for (int32 i = 0; i < OutBoneTransforms.Num(); i++)
	{
		const FTransform& AnimBoneTransform = Output.Pose.GetComponentSpaceTransform(OutBoneTransforms[i].BoneIndex);
		OutBoneTransforms[i].Transform = UKismetMathLibrary::TLerp(AnimBoneTransform, OutBoneTransforms[i].Transform, SolverBlendAlphaSmoothed);
	}

}

void FAnimNode_CustomAimSolver::SolveAimIKSystem(FCSPose<FCompactPose>& MeshBases, FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{

	if (!bIsEnableSolver && !bIsAllowSmoothingEnableSolver)
	{
		return;
	}

	// Do nothing if the fade-out is complete
	if (!bIsEnableSolver &&
		bIsAllowSmoothingEnableSolver &&
		SolverBlendAlphaSmoothed <= SolverDisableCompleteThreshold)
	{
		bSolverFadeOutOnly = false;
		return;
	}

	// During the fade-out phase while OFF, dont perform a new solve
	if (bSolverFadeOutOnly && !bIsEnableSolver)
	{
		return;
	}

	const FBoneContainer& RequiredBones = Output.Pose.GetPose().GetBoneContainer();
	const FTransform& ComponentToWorld = Output.AnimInstanceProxy->GetComponentTransform();
	const auto SK = Output.AnimInstanceProxy->GetSkelMeshComponent();

	RefConstantForwardTemp = bIsUseReferenceForwardAxis ? ReferenceConstantForwardAxis : ForwardDirectionVector;


	FTransform LookAtLocation_WS = LookAtLocation;
	FVector ArmAveragePosition = FVector::ZeroVector;

	if (bIsAggregateHandBody && bIsUseSeparateTargets)
	{
		for (int32 ArmIdx = 0; ArmIdx < ArmTargetLocationOverrides.ArmTargetLocationOverrides.Num(); ArmIdx++)
		{
			FVector Arm_Position = ArmTargetLocationOverrides.ArmTargetLocationOverrides[ArmIdx].OverrideArmTransform.GetLocation();

#if WITH_EDITOR
			if (!SK->GetWorld()->IsGameWorld() && bIsWorkOutsidePIE)
			{
				if (DebugHandTransforms.IsValidIndex(ArmIdx))
				{
					Arm_Position = DebugHandTransforms[ArmIdx].GetLocation();
				}
			}
#endif

			ArmAveragePosition += Arm_Position;
		}

		if (ArmTargetLocationOverrides.ArmTargetLocationOverrides.Num() > 0)
		{
			ArmAveragePosition = ArmAveragePosition / ArmTargetLocationOverrides.ArmTargetLocationOverrides.Num();
		}
	}


#if WITH_EDITOR
	for (int32 ArmIdx = 0; ArmIdx < ArmTargetLocationOverrides.ArmTargetLocationOverrides.Num(); ArmIdx++)
	{
		if (!SK->GetWorld()->IsGameWorld() && bIsWorkOutsidePIE)
		{
			if (DebugHandTransforms.IsValidIndex(ArmIdx))
			{
				DebugHandTransforms[ArmIdx] = ArmTargetLocationOverrides.ArmTargetLocationOverrides[ArmIdx].OverrideArmTransform;
			}
		}
	}

	if (!SK->GetWorld()->IsGameWorld() && bIsWorkOutsidePIE)
	{
		LookAtLocation_WS = DebugLookAtTransform;
	}
#endif

	FAxis LookatAxis_Temp;
	LookatAxis_Temp.bInLocalSpace = false;
	LookatAxis_Temp.Axis = ForwardDirectionVector.GetSafeNormal();

	FAxis IntermediateAxis = IntermediateLookAtAxis.ToFAxis();

	const bool bIsPIE = (!SK->GetWorld()->IsGameWorld() && bIsWorkOutsidePIE);

	if (!bIsPIE)
	{
		if (bIsAdaptiveTerrain)
		{
			FVector TTS_Ref_Pos = LookAtLocation_WS.GetLocation();
			FVector TTS_Ref_Down = TTS_Ref_Pos;

			ApplyLineTrace(
				FAnimationUpdateContext(Output.AnimInstanceProxy, 0.0f),
				TTS_Ref_Pos + FVector(0, 0, TraceUpHeight) * ComponentScale,
				TTS_Ref_Down + FVector(0, 0, -TraceDownHeight) * ComponentScale,
				AimHitResult,
				FLinearColor::Blue,
				true);

			HitResultHeight = FMath::Abs(TTS_Ref_Pos.Z - ComponentToWorld.GetLocation().Z);

			if (AimHitResult.bBlockingHit)
			{
				LookAtLocation_WS.SetLocation(AimHitResult.ImpactPoint + FVector(0, 0, HitResultHeight));
			}

		}
		else if (bIsAdaptiveTerrainFollowBone)
		{

			const auto HitPos = GetCorrectedEffectorLocationFromTraceByName(
				Output,
				TraceJointName,
				TraceChildBoneName,
				LookAtLocation_WS.GetLocation(),
				TraceUpHeight,
				TraceDownHeight,
				AdditionalOffsetHeight);

			LookAtLocation_WS.SetLocation(HitPos);

		}

	}

	// ccd root
	HeadOrigTransform = MeshBases.GetComponentSpaceTransform(EndSplineBone.CachedCompactPoseIndex);

	// root
	OriginTransform = MeshBases.GetComponentSpaceTransform(StartSplineBone.CachedCompactPoseIndex);

	AnimCurveAlpha = 1.0f;
	if (bUseAnimCurveAlpha)
	{
		const float CurveValue = Output.Curve.Get(IKAlphaCurveName);
		AnimCurveAlpha = FMath::Clamp(CurveValue, 0.0f, 1.0f);
	}

	DistanceAlpha = 1.0f;
	if (bUseDistanceAlpha)
	{
		FVector RelativeDistancePositionWS = ComponentToWorld.TransformPosition(HeadOrigTransform.GetLocation());
		if (bUseRootAsClampOrigin)
		{
			RelativeDistancePositionWS = ComponentToWorld.TransformPosition(OriginTransform.GetLocation());
		}

		const int32 DistanceCheckBoneIdx = RequiredBones.GetPoseBoneIndexForBoneName(DistanceCheckBoneName);
		if (DistanceCheckBoneIdx != INDEX_NONE)
		{
			const FCompactPoseBoneIndex DistanceCheckBoneIndex(DistanceCheckBoneIdx);
			const FTransform DistanceCheckBoneCS = Output.Pose.GetComponentSpaceTransform(DistanceCheckBoneIndex);
			const FVector DistanceCheckBoneWS = ComponentToWorld.TransformPosition(DistanceCheckBoneCS.GetLocation());
			RelativeDistancePositionWS = DistanceCheckBoneWS;
		}

		const float EffectorDist = FVector::Dist(RelativeDistancePositionWS, LookAtLocation_WS.GetLocation());

#if ENABLE_DRAW_DEBUG
		if (bDisplayLineTrace)
		{
			DrawDebugLine(SK->GetWorld(), RelativeDistancePositionWS, LookAtLocation_WS.GetLocation(), FColor::White);
			DrawDebugSphere(SK->GetWorld(), RelativeDistancePositionWS, 50.0f, 10.0, FColor::Blue);
			DrawDebugSphere(SK->GetWorld(), LookAtLocation_WS.GetLocation(), 50.0f, 10.0, FColor::Blue);
		}
#endif

		DistanceAlpha = FMath::Clamp(DistanceAlphaCurve.GetRichCurve()->Eval(EffectorDist), 0.0f, 1.0f);
	}

	if (LerpedLookatLocation.IsNearlyZero())
	{
		LerpedLookatLocation = LookAtLocation_WS.GetLocation();
	}
	LerpedLookatLocation = AnimationLocationLerp(LerpedLookatLocation, LookAtLocation_WS.GetLocation(), CachedDeltaSeconds);


	LookAtLocation_WS.SetLocation(LerpedLookatLocation);

	// 手前側に入りすぎる target を押し戻す
	if (bUseNearMinDistanceClamp)
	{
		FVector OriginWS = ComponentToWorld.TransformPosition(HeadOrigTransform.GetLocation());
		if (bUseRootAsClampOrigin)
		{
			OriginWS = ComponentToWorld.TransformPosition(OriginTransform.GetLocation());
		}
		const FVector ForwardWS = ComponentToWorld.TransformVector(ForwardDirectionVector).GetSafeNormal();

		FVector ToTargetWS = LookAtLocation_WS.GetLocation() - OriginWS;

		const float ForwardDist = FVector::DotProduct(ToTargetWS, ForwardWS);
		const FVector ForwardProjected = ForwardWS * ForwardDist;
		const FVector SideOffset = ToTargetWS - ForwardProjected;

		if (ForwardDist < NearMinDistance)
		{
			const float ClampedForwardDist = NearMinDistance;

			const FVector ForwardOnlyClamped = OriginWS + (ForwardWS * ClampedForwardDist) + SideOffset;

			const float TotalDist = ToTargetWS.Size();
			FVector RadialClamped = LookAtLocation_WS.GetLocation();

			if (TotalDist > KINDA_SMALL_NUMBER && TotalDist < NearMinDistance)
			{
				RadialClamped = OriginWS + ToTargetWS.GetSafeNormal() * NearMinDistance;
			}

			const FVector FinalClampedPos = FMath::Lerp(RadialClamped, ForwardOnlyClamped, FMath::Clamp(NearMinDistanceForwardOnlyAlpha, 0.0f, 1.0f));
			LookAtLocation_WS.SetLocation(FinalClampedPos);
		}
	}

	FVector ClampOriginWS = ComponentToWorld.TransformPosition(HeadOrigTransform.GetLocation());
	if (bUseRootAsClampOrigin)
	{
		ClampOriginWS = ComponentToWorld.TransformPosition(OriginTransform.GetLocation());
	}
	const FVector ClampForwardWS = ComponentToWorld.TransformVector(ForwardDirectionVector).GetSafeNormal();
	const FVector RawTargetWS = LookAtLocation_WS.GetLocation();

	FVector ClampedTargetWS = FVector::ZeroVector;
	if (bIsNotLimitMethod)
	{
		ClampedTargetWS = AimHelper::ClampRotateVector_NoLimit(
			RawTargetWS,
			ClampForwardWS,
			ClampOriginWS,
			VerticalRangeAngles.X,
			VerticalRangeAngles.Y,
			HorizontalRangeAngles.X,
			HorizontalRangeAngles.Y,
			bIsUseNaturalMethod);
	}
	else
	{
		ClampedTargetWS = AimHelper::ClampRotateVector(
			RawTargetWS,
			ClampForwardWS,
			ClampOriginWS,
			VerticalRangeAngles.X,
			VerticalRangeAngles.Y,
			HorizontalRangeAngles.X,
			HorizontalRangeAngles.Y,
			bIsUseNaturalMethod);
	}


	LookAtLocation_WS.SetLocation(ClampedTargetWS);


#if ENABLE_DRAW_DEBUG
	if (bDisplayLineTrace && SK && SK->GetWorld())
	{
		// clamp 前の生ターゲット(黄) と clamp 後(緑)
		DrawDebugSphere(SK->GetWorld(), RawTargetWS, 20.0f, 12, FColor::Yellow, false, -1.0f);
		DrawDebugSphere(SK->GetWorld(), ClampedTargetWS, 20.0f, 12, FColor::Green, false, -1.0f);
		DrawDebugLine(SK->GetWorld(), RawTargetWS, ClampedTargetWS, FColor::Red, false, -1.0f, 0, 2.0f);
	}
#endif

	FTransform LookHeadTransform = LookAtLocation_WS;
	FTransform LookTargetTransform = LookAtLocation_WS;

	if (bIsAggregateHandBody && bIsUseSeparateTargets)
	{
		FVector OriginWS = ComponentToWorld.TransformPosition(HeadOrigTransform.GetLocation());
		if (bUseRootAsClampOrigin)
		{
			OriginWS = ComponentToWorld.TransformPosition(OriginTransform.GetLocation());
		}
		const FVector ClampedTargetDir = (LookAtLocation_WS.GetLocation() - OriginWS).GetSafeNormal();
		const float HeadArmDist = (OriginWS - ArmAveragePosition).Size() * 2;
		LookAtLocation_WS.SetLocation(OriginWS + ClampedTargetDir * HeadArmDist);
		LookTargetTransform.SetLocation((LookAtLocation_WS.GetLocation() + ArmAveragePosition) / 2);
		LookAtLocation_WS.SetLocation((LookAtLocation_WS.GetLocation() + ArmAveragePosition) / 2);
	}

	FAnimationRuntime::ConvertBoneSpaceTransformToCS(ComponentToWorld, MeshBases, LookTargetTransform, EndSplineBone.GetCompactPoseIndex(RequiredBones), EBoneControlSpace::BCS_WorldSpace);
	FAnimationRuntime::ConvertBoneSpaceTransformToCS(ComponentToWorld, MeshBases, LookHeadTransform, EndSplineBone.GetCompactPoseIndex(RequiredBones), EBoneControlSpace::BCS_WorldSpace);

	bIsNsewPoleMethod = false;
	bIsUpArmTwistTechnique = (ArmTwistAxis == EArmTwistIKType::UpAxisTwist) ? true : false;


	switch (AimSolverType)
	{
	case EAimSolverType::HandAim:
	case EAimSolverType::LookAt:
	{
		AimHelper::Evaluate_ConsecutiveBoneRotations_LookAt(
			Output,
			StartSplineBone,
			EndSplineBone,
			InnerBodyClamp,
			LookAtLocation_WS,
			LookatAxis_Temp,
			IntermediateAxis,
			LookBendingCurve,
			LookMultiplierCurve,
			bUseSpecificIntermediateAxis,
			bIsEnableDipMultiplier,
			LookAtRadius,
			VerticalDipTreshold,
			DownwardDipMultiplier,
			InvertedDipMultiplier,
			SideMoveMultiplier,
			SideDownMultiplier,
			FTransform::Identity,
			UpRotClamp,
			bIsUseNaturalMethod,
			bIsHeadUseSeparateClamp,
			LookAtClamp,
			LookHeadTransform,
			bUsePerJointRotationOverrides,
			PerJointRotationOverrides,
			HeadTransforms);
	}
	break;
	case EAimSolverType::CCD:
	{
		AimHelper::Evaluate_ConsecutiveBoneRotations_CCD(
			Output,
			StartSplineBone,
			EndSplineBone,
			BaseAngle,
			LookAtLocation_WS,
			LookatAxis_Temp,
			LookBendingCurve,
			LookMultiplierCurve,
			bIsEnableDipMultiplier,
			VerticalDipTreshold,
			DownwardDipMultiplier,
			InvertedDipMultiplier,
			SideMoveMultiplier,
			SideDownMultiplier,
			false,
			FTransform::Identity,
			UpRotClamp,
			bIsUseNaturalMethod,
			bIsHeadUseSeparateClamp,
			LookHeadTransform,
			Precision,
			MaxIteration,
			bUsePerJointRotationOverrides,
			PerJointRotationOverrides,
			bUseTipReferenceBone,
			TipReferenceBone,
			RobotArmSettings,
			bAlignTipYawToTarget,
			HeadTransforms);
	}
	break;
	}


	// follow TerrainFollowBone LookAt ImpactNormal
	if (bUseEndSurfaceNormal && bIsAdaptiveTerrainFollowBone && EndSurfaceHitResult.bBlockingHit)
	{
		const int32 LastIndex = (HeadTransforms.Num() - 1);
		const float DotUp = FMath::Clamp(FVector::DotProduct(EndSurfaceHitResult.ImpactNormal.GetSafeNormal(), FVector::UpVector), -1.0f, 1.0f);

		const float SlopeAngleDeg = FMath::RadiansToDegrees(FMath::Acos(DotUp));

		if (SlopeAngleDeg >= EndSurfaceMinSlopeAngle)
		{
			FTransform EndTransform = HeadTransforms[LastIndex].Transform;
			EndTransform.SetRotation(
				MakeSurfaceAlignedRotationPreserveForward(
					EndTransform,
					EndSurfaceHitResult.ImpactNormal,
					EndSurfaceLocalForwardAxis,
					EndSurfaceLocalUpAxis,
					EndSurfaceNormalAlignAlpha));

			HeadTransforms[LastIndex].Transform = EndTransform;
		}
	}

	if (bIsOverrideHeadRotation)
	{
		const int32 LastIndex = (HeadTransforms.Num() - 1);
		const FBoneTransform& BoneTransform = LookAt_Processor(
			RequiredBones,
			Output,
			MeshBases,
			LookHeadTransform.GetLocation(),
			LastIndex);
		const FQuat ProcessRot = BoneTransform.Transform.GetRotation();
		HeadTransforms[LastIndex].Transform.SetRotation(BoneTransform.Transform.GetRotation());
	}


	TArray<FCompactPoseBoneIndex> HeadTransformPoses = TArray<FCompactPoseBoneIndex>();

	for (int32 Index = 0; Index < HeadTransforms.Num(); Index++)
	{
		HeadTransformPoses.Add(HeadTransforms[Index].BoneIndex);
	}

	if (!bIsIgnoreSeparateHandSolving)
	{
		if (!bIsReachInstead)
		{
			if (MainArmIndex > -1 && MainArmIndex < HandBoneArray.Num())
			{
				if (ActualShoulderBoneArray[MainArmIndex].IsValidToEvaluate() &&
					ElbowBoneArray[MainArmIndex].IsValidToEvaluate() &&
					HandBoneArray[MainArmIndex].IsValidToEvaluate())
				{
					FCompactPoseBoneIndex ConnectorIndex = FCompactPoseBoneIndex(0);
					FTransform CommonSpineModifiedTransform = FTransform::Identity;

					for (int32 Index = 0; Index < HeadTransforms.Num(); Index++)
					{
						const FCompactPoseBoneIndex& FirstBone = MeshBases.GetPose().GetParentBoneIndex(ActualShoulderBoneArray[MainArmIndex].CachedCompactPoseIndex);
						const FCompactPoseBoneIndex& SecondBone = MeshBases.GetPose().GetParentBoneIndex(FirstBone);

						if (HeadTransforms[Index].BoneIndex == FirstBone)
						{
							ConnectorIndex = HeadTransforms[Index].BoneIndex;
							CommonSpineModifiedTransform = HeadTransforms[Index].Transform;
						}

						if (HeadTransforms[Index].BoneIndex == SecondBone)
						{
							ConnectorIndex = HeadTransforms[Index].BoneIndex;
							CommonSpineModifiedTransform = HeadTransforms[Index].Transform;
						}
					}

					if (ConnectorIndex.GetInt() > 0)
					{
						const FVector Arm_LookTarget = LookAtLocation_WS.GetLocation();
						FTransform  Common_Spine_Transform = MeshBases.GetComponentSpaceTransform(ConnectorIndex);
						FVector Target_CS_Position = ComponentToWorld.InverseTransformPosition(Arm_LookTarget);

						Target_CS_Position += AimingHandLimbs[MainArmIndex].ArmAimingOffset;
						const FTransform Inv_Common_Spine = Common_Spine_Transform.Inverse() * CommonSpineModifiedTransform;
						FTransform Hand_Transform_Default = MeshBases.GetComponentSpaceTransform(HandBoneArray[MainArmIndex].CachedCompactPoseIndex);

						const FTransform& Shoulder_Transform_Default = MeshBases.GetComponentSpaceTransform(ActualShoulderBoneArray[MainArmIndex].CachedCompactPoseIndex);

						const FTransform& Elbow_Transform_Default = MeshBases.GetComponentSpaceTransform(ElbowBoneArray[MainArmIndex].CachedCompactPoseIndex);

						FTransform Hand_Transform = MeshBases.GetComponentSpaceTransform(
							HandBoneArray[MainArmIndex].CachedCompactPoseIndex) * Inv_Common_Spine;

						const FTransform Shoulder_Transform = MeshBases.GetComponentSpaceTransform(
							ActualShoulderBoneArray[MainArmIndex].CachedCompactPoseIndex) * Inv_Common_Spine;

						FTransform Shoulder_Offseted_Transform = Shoulder_Transform_Default;
						Shoulder_Offseted_Transform.SetLocation(Shoulder_Transform.GetLocation());

						const float Individual_Leg_Clamp = LimbsClamp;

						FTransform Rotated_Shoulder = AimHelper::LookAt_Processor_Helper(
							Shoulder_Offseted_Transform,
							Common_Spine_Transform.GetLocation(),
							Target_CS_Position,
							LookatAxis_Temp,
							Individual_Leg_Clamp,
							FRotator::ZeroRotator,
							bIsUseNaturalMethod,
							1.0f,
							1.0f);


						const FTransform Inv_Shoulder_Value = Shoulder_Transform_Default.Inverse() * Rotated_Shoulder;
						FTransform Shoulder_Transform_Output = Shoulder_Transform_Default * Inv_Shoulder_Value;
						FTransform Elbow_Transform_Output = Elbow_Transform_Default * Inv_Shoulder_Value;
						FTransform Hand_Transform_Output = Hand_Transform_Default * Inv_Shoulder_Value;

						const FVector Arm_Vector = (Hand_Transform_Output.GetLocation() - Shoulder_Transform_Output.GetLocation());
						const float Arm_Length = Arm_Vector.Size();
						float Target_Arm_Length = (CommonSpineModifiedTransform.GetLocation() - Target_CS_Position).Size();

						Target_Arm_Length = FMath::Clamp(Target_Arm_Length, 1, Arm_Length);
						Hand_Transform.SetLocation(Shoulder_Transform_Output.GetLocation() + Arm_Vector.GetSafeNormal() * Target_Arm_Length);

						Common_Spine_Transform.SetLocation(Target_CS_Position);

						if (!bIsIgnoreElbowModification)
						{
							FTransform Main_Relative_Transform = (MainHandDefaultTransform.Inverse() * MainHandNewTransform);
							FTransform Offseted_Hand_Transform = Hand_Transform_Default * Main_Relative_Transform;

							if (MainArmIndex < 0)
							{
								Main_Relative_Transform = FTransform::Identity;
								Offseted_Hand_Transform = Hand_Transform;
							}

							FTransform Hand_Spine_Relation = Hand_Transform_Default * Inv_Common_Spine;

							if (bIsReachInstead)
							{
								Offseted_Hand_Transform.SetLocation(Target_CS_Position);

								Offseted_Hand_Transform.SetLocation(AimHelper::ClampRotateVector(
									ComponentToWorld.TransformPosition(Offseted_Hand_Transform.GetLocation()),
									ComponentToWorld.TransformVector(ForwardDirectionVector),
									ComponentToWorld.TransformPosition(Shoulder_Transform_Output.GetLocation()),
									AimingHandLimbs[MainArmIndex].MaxArm_VAngle.X,
									AimingHandLimbs[MainArmIndex].MaxArm_VAngle.Y,
									AimingHandLimbs[MainArmIndex].MaxArm_HAngle.X,
									AimingHandLimbs[MainArmIndex].MaxArm_HAngle.Y,
									bIsUseNaturalMethod));


								Offseted_Hand_Transform.SetLocation(
									ComponentToWorld.InverseTransformPosition(Offseted_Hand_Transform.GetLocation()));

								Shoulder_Transform_Output = Shoulder_Transform_Default * Inv_Common_Spine;
								Elbow_Transform_Output = Elbow_Transform_Default * Inv_Common_Spine;
								Hand_Transform_Output = Hand_Spine_Relation;

								FVector Point_Thigh_Dir = (Offseted_Hand_Transform.GetLocation() - Shoulder_Transform_Output.GetLocation());
								const float Point_Thigh_Size = Point_Thigh_Dir.Size();
								const float Effector_Thigh_Size = (Hand_Transform_Output.GetLocation() - Shoulder_Transform_Output.GetLocation()).Size();
								Point_Thigh_Dir.Normalize();

								Offseted_Hand_Transform.SetLocation(
									Shoulder_Transform_Output.GetLocation() + Point_Thigh_Dir *
									FMath::Clamp(
										Point_Thigh_Size,
										Effector_Thigh_Size * FMath::Abs(AimingHandLimbs[MainArmIndex].MinimumExtension),
										Effector_Thigh_Size * FMath::Abs(AimingHandLimbs[MainArmIndex].MaximumExtension)));
							}
							else
							{
								Offseted_Hand_Transform.SetLocation(Shoulder_Transform_Output.GetLocation() + (Arm_Vector.GetSafeNormal() *
									FMath::Clamp(
										Arm_Length,
										Arm_Length * FMath::Abs(AimingHandLimbs[MainArmIndex].MinimumExtension),
										Arm_Length * FMath::Abs(AimingHandLimbs[MainArmIndex].MaximumExtension))));
							}

							if (ArmTargetLocationOverrides.ArmTargetLocationOverrides.Num() > MainArmIndex)
							{
								Offseted_Hand_Transform = UKismetMathLibrary::TLerp(Hand_Spine_Relation, Offseted_Hand_Transform,
									ArmTargetLocationOverrides.ArmTargetLocationOverrides[MainArmIndex].ArmAlpha);

								if (bIsOverrideHandRotation)
								{
									const FQuat Offseted_Rotation_Value = ComponentToWorld.GetRotation().Inverse() *
										DebugHandTransforms[MainArmIndex].GetRotation();
									Offseted_Hand_Transform.SetRotation(Offseted_Rotation_Value);
								}
							}

							float MainArmLSA = 0;
							AimHelper::Evaluate_TwoBoneIK_Direct_Modified(
								Output,
								SK,
								HandBoneArray[MainArmIndex],
								ElbowBoneArray[MainArmIndex],
								ActualShoulderBoneArray[MainArmIndex],
								Hand_Transform,
								Shoulder_Transform_Output,
								Elbow_Transform_Output,
								Hand_Transform_Output,
								FVector::ZeroVector,
								FVector::ZeroVector,
								Inv_Common_Spine,
								Common_Spine_Transform,
								LimbRotationOffset,
								AimingHandLimbs[MainArmIndex],
								Individual_Leg_Clamp,
								FTransform::Identity,
								AimingHandLimbs[MainArmIndex].ElbowPoleOffset,
								bIsOverrideHandRotation,
								Elbow_Transform_Default,
								ForwardDirectionVector,
								RefConstantForwardTemp,
								MainArmLSA,
								bIsNsewPoleMethod,
								bIsUpArmTwistTechnique,
								CharacterDirectionVectorCS,
								bIsUseSeparateTargets,
								bIsReachInstead,
								ArmSolverWorkArea[MainArmIndex]);

							MainHandNewTransform = ArmSolverWorkArea[MainArmIndex].ResultElbow.Transform;
							MainHandDefaultTransform = Elbow_Transform_Default;
						}
						else
						{

							ArmSolverWorkArea[MainArmIndex].ResultShoulder.Transform = Rotated_Shoulder;
						}
					}

				}
			}
		}


		constexpr int32 AimingHandLimb_MAX = 5;
		for (int32 LIndex = 0; LIndex < AimingHandLimbs.Num(); LIndex++)
		{
			ActualShoulderBoneArray[LIndex] = ShoulderBoneArray[LIndex];
			const FBoneReference Original_BoneRef = ActualShoulderBoneArray[LIndex];
			FCompactPoseBoneIndex CurParent = ActualShoulderBoneArray[LIndex].CachedCompactPoseIndex;
			FCompactPoseBoneIndex LastSavedBone = CurParent;

			bool Found_Limbs = false;

			for (int32 Index = 0; Index < AimingHandLimb_MAX; Index++)
			{
				if (ActualShoulderBoneArray[LIndex].BoneIndex > 0)
				{
					LastSavedBone = ActualShoulderBoneArray[LIndex].GetCompactPoseIndex(RequiredBones);

					if (LastSavedBone.GetInt() > INDEX_NONE)
					{
						FCompactPoseBoneIndex Ref_Parent = MeshBases.GetPose().GetParentBoneIndex(LastSavedBone);
						if (HeadTransformPoses.Contains(Ref_Parent))
						{
							break;
						}

						Found_Limbs = true;

						if (!bIsReachInstead && !bIsUseSeparateTargets)
						{
							if (!ActualShoulderBoneArray[LIndex].IsValidToEvaluate())
							{
								ActualShoulderBoneArray[LIndex] = FBoneReference(SK->GetBoneName(CurParent.GetInt()));
								ActualShoulderBoneArray[LIndex].Initialize(RequiredBones);

								UE_LOG(LogTemp, Error, TEXT("[%s] : ActualShoulderBoneArray[LIndex] not initialized"), *FString(__FUNCTION__));
							}
						}

						if (LIndex < ActualShoulderBoneArray.Num() && LIndex > -1)
						{
							if (ActualShoulderBoneArray[LIndex].GetCompactPoseIndex(RequiredBones).GetInt() > -1)
							{
								CurParent = MeshBases.GetPose().GetParentBoneIndex(
									ActualShoulderBoneArray[LIndex].GetCompactPoseIndex(RequiredBones));
							}
						}
					}

				}
				else
				{
					break;
				}
			}


		}


		for (int32 LIndex = 0; LIndex < AimingHandLimbs.Num(); LIndex++)
		{
			if ((ActualShoulderBoneArray[LIndex].IsValidToEvaluate() &&
				ElbowBoneArray[LIndex].IsValidToEvaluate() &&
				HandBoneArray[LIndex].IsValidToEvaluate()) &&
				(LIndex != MainArmIndex || MainArmIndex < 0 || bIsReachInstead))
			{

				if (ElbowBoneTransformArray.Num() > LIndex)
				{
					ElbowBoneTransformArray[LIndex] = MeshBases.GetComponentSpaceTransform(ElbowBoneArray[LIndex].CachedCompactPoseIndex);
				}

				if (HandDefaultTransformArray.Num() > LIndex)
				{
					HandDefaultTransformArray[LIndex] = MeshBases.GetComponentSpaceTransform(HandBoneArray[LIndex].CachedCompactPoseIndex);
				}

				FCompactPoseBoneIndex Connector_Index = FCompactPoseBoneIndex(0);
				FTransform Common_Spine_Modified_Transform = FTransform::Identity;
				FVector Arm_LookTarget = LookAtLocation_WS.GetLocation();

				if (bIsUseSeparateTargets)
				{
					if (ArmTargetLocationOverrides.ArmTargetLocationOverrides.Num() > LIndex)
					{
						if (DebugHandTransforms.IsValidIndex(LIndex))
						{
							Arm_LookTarget = DebugHandTransforms[LIndex].GetLocation();
						}
					}
				}

				for (int32 BodyIndex = 0; BodyIndex < HeadTransforms.Num(); BodyIndex++)
				{
					const FCompactPoseBoneIndex& FirstBoneIdx = MeshBases.GetPose().GetParentBoneIndex(ActualShoulderBoneArray[LIndex].CachedCompactPoseIndex);
					const FCompactPoseBoneIndex& SecondBoneIdx = MeshBases.GetPose().GetParentBoneIndex(FirstBoneIdx);

					if (HeadTransforms[BodyIndex].BoneIndex == FirstBoneIdx)
					{
						Connector_Index = HeadTransforms[BodyIndex].BoneIndex;
						Common_Spine_Modified_Transform = HeadTransforms[BodyIndex].Transform;
					}


					if (HeadTransforms[BodyIndex].BoneIndex == SecondBoneIdx)
					{
						Connector_Index = HeadTransforms[BodyIndex].BoneIndex;
						Common_Spine_Modified_Transform = HeadTransforms[BodyIndex].Transform;
					}
				}

				if (!(Connector_Index.GetInt() > 0))
				{
					continue;
				}

				FTransform Common_Spine_Transform = MeshBases.GetComponentSpaceTransform(Connector_Index);
				FVector Target_CS_Position = ComponentToWorld.InverseTransformPosition(Arm_LookTarget);


				FVector OriginWS = ComponentToWorld.TransformPosition(HeadOrigTransform.GetLocation());
				if (bUseRootAsClampOrigin)
				{
					OriginWS = ComponentToWorld.TransformPosition(OriginTransform.GetLocation());
				}

				if (AimingHandLimbs[LIndex].bIsOverrideLimits)
				{
					Target_CS_Position = (AimHelper::ClampRotateVector(
						Arm_LookTarget,
						ComponentToWorld.TransformVector(ForwardDirectionVector),
						OriginWS,
						AimingHandLimbs[LIndex].MaxArm_VAngle.X,
						AimingHandLimbs[LIndex].MaxArm_VAngle.Y,
						AimingHandLimbs[LIndex].MaxArm_HAngle.X,
						AimingHandLimbs[LIndex].MaxArm_HAngle.Y,
						bIsUseNaturalMethod));
				}
				else
				{
					Target_CS_Position = (AimHelper::ClampRotateVector(
						Arm_LookTarget,
						ComponentToWorld.TransformVector(ForwardDirectionVector),
						OriginWS,
						VerticalRangeAngles.X,
						VerticalRangeAngles.Y,
						HorizontalRangeAngles.X,
						HorizontalRangeAngles.Y,
						bIsUseNaturalMethod));
				}

				Target_CS_Position = ComponentToWorld.InverseTransformPosition(Target_CS_Position);
				Target_CS_Position += AimingHandLimbs[LIndex].ArmAimingOffset;

				const FTransform Inv_Common_Spine = Common_Spine_Transform.Inverse() * Common_Spine_Modified_Transform;
				const FTransform Hand_Transform_Default = MeshBases.GetComponentSpaceTransform(HandBoneArray[LIndex].CachedCompactPoseIndex);

				const FTransform Shoulder_Transform_Default = MeshBases.GetComponentSpaceTransform(ActualShoulderBoneArray[LIndex].CachedCompactPoseIndex);

				const FTransform Knee_Transform_Default = MeshBases.GetComponentSpaceTransform(ElbowBoneArray[LIndex].CachedCompactPoseIndex);

				FTransform Hand_Transform = MeshBases.GetComponentSpaceTransform(HandBoneArray[LIndex].CachedCompactPoseIndex) * Inv_Common_Spine;

				FTransform Shoulder_Transform = MeshBases.GetComponentSpaceTransform(ActualShoulderBoneArray[LIndex].CachedCompactPoseIndex) * Inv_Common_Spine;

				FTransform Shoulder_Offseted_Transform = Shoulder_Transform_Default;
				Shoulder_Offseted_Transform.SetLocation(Shoulder_Transform.GetLocation());
				const float Individual_Leg_Clamp = LimbsClamp;
				const FVector X_Diff = Shoulder_Transform.GetLocation() - Common_Spine_Transform.GetLocation();

				FTransform Rotated_Shoulder = AimHelper::LookAt_Processor_Helper(
					Shoulder_Offseted_Transform,
					Common_Spine_Transform.GetLocation(),
					Target_CS_Position,
					LookatAxis_Temp,
					Individual_Leg_Clamp,
					FRotator::ZeroRotator,
					bIsUseNaturalMethod,
					1.0f,
					1.0f);

				const FTransform Inv_Shoulder_Value = Shoulder_Transform_Default.Inverse() * Rotated_Shoulder;
				FTransform Shoulder_Transform_Output = Shoulder_Transform_Default * Inv_Shoulder_Value;
				FTransform Knee_Transform_Output = Knee_Transform_Default * Inv_Shoulder_Value;
				FTransform Hand_Transform_Output = Hand_Transform_Default * Inv_Shoulder_Value;

				const FVector Arm_Vector = (Hand_Transform_Output.GetLocation() - Shoulder_Transform_Output.GetLocation());
				const float Arm_Length = Arm_Vector.Size();
				float Target_Arm_Length = (Common_Spine_Modified_Transform.GetLocation() - Target_CS_Position).Size();

				Target_Arm_Length = FMath::Clamp(Target_Arm_Length, 1, Arm_Length);
				Hand_Transform.SetLocation(Shoulder_Transform_Output.GetLocation() + Arm_Vector.GetSafeNormal() * Target_Arm_Length);

				Common_Spine_Transform.SetLocation(Target_CS_Position);

				if (!bIsIgnoreElbowModification)
				{
					FTransform Main_Relative_Transform = (MainHandDefaultTransform.Inverse() * MainHandNewTransform);
					FTransform Offseted_Hand_Transform = Hand_Transform_Default * Main_Relative_Transform;

					if (MainArmIndex < 0 || bIsReachInstead)
					{
						Main_Relative_Transform = FTransform::Identity;
						Offseted_Hand_Transform = Hand_Transform;
					}

					FTransform Hand_Spine_Relation = Hand_Transform_Default * Inv_Common_Spine;
					FTransform Elbow_Pole_Transform = FTransform::Identity;

					if (bIsReachInstead)
					{
						Offseted_Hand_Transform.SetLocation(Target_CS_Position);
						Shoulder_Transform_Output = Shoulder_Transform_Default * Inv_Common_Spine;
						Knee_Transform_Output = Knee_Transform_Default * Inv_Common_Spine;
						Hand_Transform_Output = Hand_Spine_Relation;

						FVector Point_Thigh_Dir = (Offseted_Hand_Transform.GetLocation() - Shoulder_Transform_Output.GetLocation());
						const float Point_Thigh_Size = Point_Thigh_Dir.Size();
						const float Effector_Thigh_Size = (Hand_Transform_Output.GetLocation() - Shoulder_Transform_Output.GetLocation()).Size();
						Point_Thigh_Dir.Normalize();

						const float ClampVal = FMath::Clamp(
							Point_Thigh_Size,
							Effector_Thigh_Size * FMath::Abs(AimingHandLimbs[LIndex].MinimumExtension),
							Effector_Thigh_Size * FMath::Abs(AimingHandLimbs[LIndex].MaximumExtension));

						Offseted_Hand_Transform.SetLocation(Shoulder_Transform_Output.GetLocation() + Point_Thigh_Dir * ClampVal);

						Elbow_Pole_Transform.SetLocation(AimingHandLimbs[LIndex].ElbowPoleOffset);
						Elbow_Pole_Transform = Elbow_Pole_Transform * Inv_Shoulder_Value;
					}
					else
					{
						const float ClampVal = FMath::Clamp(
							Arm_Length,
							Arm_Length * FMath::Abs(AimingHandLimbs[LIndex].MinimumExtension),
							Arm_Length * FMath::Abs(AimingHandLimbs[LIndex].MaximumExtension));

						const FVector Arm_Hand_Vector = (Offseted_Hand_Transform.GetLocation() - Shoulder_Transform_Output.GetLocation()).GetSafeNormal();
						Offseted_Hand_Transform.SetLocation(Shoulder_Transform_Output.GetLocation() + (Arm_Hand_Vector * ClampVal));
					}


					if (ArmTargetLocationOverrides.ArmTargetLocationOverrides.IsValidIndex(LIndex))
					{
						const FBone_SingleArmElement& OverrideData = ArmTargetLocationOverrides.ArmTargetLocationOverrides[LIndex];
						Offseted_Hand_Transform = UKismetMathLibrary::TLerp(Hand_Spine_Relation, Offseted_Hand_Transform, OverrideData.ArmAlpha);

						if (bIsOverrideHandRotation)
						{
							FQuat Offseted_Rotation_Value = ComponentToWorld.GetRotation().Inverse() * LookAtLocation.GetRotation();
							if (!SK->GetWorld()->IsGameWorld() && bIsWorkOutsidePIE)
							{
								if (DebugHandTransforms.IsValidIndex(LIndex))
								{
									Offseted_Rotation_Value = ComponentToWorld.GetRotation().Inverse() * DebugHandTransforms[LIndex].GetRotation();
								}
							}
							Offseted_Hand_Transform.SetRotation(Offseted_Rotation_Value);
						}

					}
					else
					{
						if (bIsOverrideHandRotation)
						{
							const FQuat Offseted_Rotation_Value = ComponentToWorld.GetRotation().Inverse() * LookAtLocation.GetRotation();
							Offseted_Hand_Transform.SetRotation(Offseted_Rotation_Value);
						}
					}

					AimHelper::Evaluate_TwoBoneIK_Direct_Modified(
						Output,
						SK,
						HandBoneArray[LIndex],
						ElbowBoneArray[LIndex],
						ActualShoulderBoneArray[LIndex],
						Offseted_Hand_Transform,
						Shoulder_Transform_Output,
						Knee_Transform_Output,
						Hand_Transform_Output,
						FVector::ZeroVector,
						FVector::ZeroVector,
						Inv_Common_Spine,
						Common_Spine_Transform,
						LimbRotationOffset,
						AimingHandLimbs[LIndex],
						Individual_Leg_Clamp,
						Main_Relative_Transform,
						Elbow_Pole_Transform.GetLocation(),
						bIsOverrideHandRotation,
						Knee_Transform_Default,
						ForwardDirectionVector,
						RefConstantForwardTemp,
						LastShoulderAngles[LIndex],
						bIsNsewPoleMethod,
						bIsUpArmTwistTechnique,
						CharacterDirectionVectorCS,
						bIsUseSeparateTargets,
						bIsReachInstead,
						ArmSolverWorkArea[LIndex]);
				}
				else
				{
					ArmSolverWorkArea[LIndex].ResultShoulder.Transform = Rotated_Shoulder;
				}


			}
		}


	}
}


FVector FAnimNode_CustomAimSolver::AnimationLocationLerp(const FVector& StartPosition, const FVector& EndPosition, const float DeltaSeconds) const
{
	if (!bIsEnableInterpolation)
	{
		return EndPosition;
	}

	FVector Local_StartPosition = StartPosition;
	FVector Local_EndPosition = EndPosition;


	if (bIsAdaptiveTerrain && AimHitResult.bBlockingHit)
	{
		Local_StartPosition.X = Local_EndPosition.X;
		Local_StartPosition.Y = Local_EndPosition.Y;
	}

	constexpr float Max = 6.0f;
	const FVector Diff = (Local_StartPosition - Local_EndPosition) / FMath::Clamp(Max - InterpolationSpeed, 1.0f, Max);

	if (InterpLocationType == EIKInterpLocationType::DivisiveLocation)
	{
		return (Local_StartPosition - Diff);
	}

	const float InterpSpeed = FMath::Abs(InterpolationSpeed);
	return FMath::VInterpTo(Local_StartPosition, Local_EndPosition, DeltaSeconds, InterpSpeed);
}


const FBoneTransform FAnimNode_CustomAimSolver::LookAt_Processor(
	const FBoneContainer& RequiredBones,
	FComponentSpacePoseContext& Output,
	FCSPose<FCompactPose>& MeshBases,
	const FVector& OffsetVector,
	const int32 InIndex)
{
	const FCompactPoseBoneIndex ModifyBoneIndex = EndSplineBone.GetCompactPoseIndex(RequiredBones);
	FTransform ComponentBoneTransform = MeshBases.GetComponentSpaceTransform(ModifyBoneIndex);

	const FTransform& ComponentToWorld = Output.AnimInstanceProxy->GetComponentTransform();

	if (!HeadTransforms.IsEmpty())
	{
		ComponentBoneTransform.SetLocation(HeadTransforms[InIndex].Transform.GetLocation());
	}

	const FVector TargetLocationInComponentSpace = OffsetVector;
	FAxis LookatAxisTemp;
	LookatAxisTemp.bInLocalSpace = false;
	LookatAxisTemp.Axis = CCDTipBoneDirectionVector.GetSafeNormal();

	const FVector LookAtVector = LookatAxisTemp.GetTransformedAxis(ComponentBoneTransform).GetSafeNormal();
	FVector TargetDir = (TargetLocationInComponentSpace - ComponentBoneTransform.GetLocation()).GetSafeNormal();
	const float AimClampInRadians = FMath::DegreesToRadians(FMath::Min(HeadEndLookAtClamp, 180.f));
	const float DiffAngle = FMath::Acos(FVector::DotProduct(LookAtVector, TargetDir));

	if (DiffAngle > AimClampInRadians)
	{
		check(DiffAngle > 0.f);
		FVector DeltaTarget = TargetDir - LookAtVector;
		DeltaTarget *= (AimClampInRadians / DiffAngle);
		TargetDir = LookAtVector + DeltaTarget;
		TargetDir.Normalize();
	}

	FQuat NormalizedDelta = FQuat::FindBetweenNormals(LookAtVector, TargetDir);
	if (!bIsUseNaturalMethod)
	{
		const FRotator Rot_Ref_01 = UKismetMathLibrary::FindLookAtRotation(FVector::ZeroVector, TargetDir);
		const FRotator Rot_Ref_02 = UKismetMathLibrary::FindLookAtRotation(FVector::ZeroVector, LookAtVector);
		NormalizedDelta = Rot_Ref_01.Quaternion() * Rot_Ref_02.Quaternion().Inverse();
	}

	FTransform RotationTransform_WS = ComponentBoneTransform;
	RotationTransform_WS.SetRotation(NormalizedDelta);

	FAnimationRuntime::ConvertCSTransformToBoneSpace(
		ComponentToWorld, MeshBases,
		RotationTransform_WS, ModifyBoneIndex, EBoneControlSpace::BCS_WorldSpace);

	FAnimationRuntime::ConvertBoneSpaceTransformToCS(
		ComponentToWorld, MeshBases,
		RotationTransform_WS, ModifyBoneIndex, EBoneControlSpace::BCS_WorldSpace);

	ComponentBoneTransform.SetRotation(RotationTransform_WS.GetRotation() * ComponentBoneTransform.GetRotation());

	return (FBoneTransform(ModifyBoneIndex, ComponentBoneTransform));
}


#if WITH_EDITOR
void FAnimNode_CustomAimSolver::ResizeDebugLocations(const int32 NewSize)
{
	if (NewSize == 0)
	{
		DebugHandTransforms.Reset();
	}
	else if (DebugHandTransforms.Num() != NewSize)
	{
		const int32 StartIndex = DebugHandTransforms.Num();
		DebugHandTransforms.SetNum(NewSize);

		int32 PairFinishCount = 1;

		for (int32 Index = StartIndex; Index < DebugHandTransforms.Num(); ++Index)
		{
			DebugHandTransforms[Index] = FTransform::Identity;

			const bool bIsEven = (Index % 2 == 0);

			if (bIsEven)
			{
				++PairFinishCount;
			}

			if (bIsEven)
			{
				DebugHandTransforms[Index].SetLocation(FVector(50.0f * PairFinishCount, 75.0f, 75.0f));
			}
			else
			{
				DebugHandTransforms[Index].SetLocation(FVector(-50.0f * PairFinishCount, 75.0f, 75.0f));
			}
		}
	}
}
#endif


FQuat FAnimNode_CustomAimSolver::MakeSurfaceAlignedRotationPreserveForward(
	const FTransform& CurrentTransform,
	const FVector& SurfaceNormalWS,
	const FVector& LocalForwardAxis,
	const FVector& LocalUpAxis,
	const float InNormalAlpha) const
{
	const FQuat CurrentRot = CurrentTransform.GetRotation();

	FVector ForwardWS = CurrentRot.RotateVector(LocalForwardAxis).GetSafeNormal();
	FVector UpWS = SurfaceNormalWS.GetSafeNormal();

	if (ForwardWS.IsNearlyZero() || UpWS.IsNearlyZero())
	{
		return CurrentRot;
	}

	ForwardWS = FVector::VectorPlaneProject(ForwardWS, UpWS).GetSafeNormal();
	if (ForwardWS.IsNearlyZero())
	{
		return CurrentRot;
	}

	const FVector RightWS = FVector::CrossProduct(UpWS, ForwardWS).GetSafeNormal();
	ForwardWS = FVector::CrossProduct(RightWS, UpWS).GetSafeNormal();

	const FMatrix TargetMatrix(
		FPlane(ForwardWS, 0.0f),
		FPlane(RightWS, 0.0f),
		FPlane(UpWS, 0.0f),
		FPlane(0, 0, 0, 1));

	const FQuat TargetRot = FQuat(TargetMatrix).GetNormalized();
	return FQuat::Slerp(CurrentRot, TargetRot, FMath::Clamp(InNormalAlpha, 0.0f, 1.0f)).GetNormalized();
}

const FVector FAnimNode_CustomAimSolver::GetCorrectedEffectorLocationFromTraceByName(
	FComponentSpacePoseContext& Output,
	const FName& TraceBoneName,
	const FName& ChildBoneName,
	const FVector& InEffectorWorldLocation,
	const float TraceUpOffset,
	const float TraceDownOffset,
	const float AdditionalOffsetZ)
{


	const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();
	const FTransform& ComponentToWorld = Output.AnimInstanceProxy->GetComponentTransform();
	const USkeletalMeshComponent* SK = Output.AnimInstanceProxy->GetSkelMeshComponent();

	const int32 TraceBonePoseIndexInt = BoneContainer.GetPoseBoneIndexForBoneName(TraceBoneName);
	const int32 ChildBonePoseIndexInt = BoneContainer.GetPoseBoneIndexForBoneName(ChildBoneName);

	if (TraceBonePoseIndexInt == INDEX_NONE || ChildBonePoseIndexInt == INDEX_NONE)
	{
		return InEffectorWorldLocation;
	}

	const FCompactPoseBoneIndex TraceBoneIndex(TraceBonePoseIndexInt);
	const FTransform TraceBoneCS = Output.Pose.GetComponentSpaceTransform(TraceBoneIndex);
	const FVector TraceBoneWS = ComponentToWorld.TransformPosition(TraceBoneCS.GetLocation());

	const FCompactPoseBoneIndex ChildBoneIndex(ChildBonePoseIndexInt);
	const FTransform ChildBoneCS = Output.Pose.GetComponentSpaceTransform(ChildBoneIndex);
	const FVector ChildBoneWS = ComponentToWorld.TransformPosition(ChildBoneCS.GetLocation());


	const auto EffectPos = InEffectorWorldLocation;

	// calc effectorlocation based trace
	const FVector TraceStart = EffectPos + FVector(0.0f, 0.0f, TraceUpOffset * ComponentScale);
	const FVector TraceEnd = EffectPos - FVector(0.0f, 0.0f, TraceDownOffset * ComponentScale);

	EndSurfaceHitResult.Reset();

	ApplyLineTrace(
		FAnimationUpdateContext(Output.AnimInstanceProxy, 0.0f),
		TraceStart,
		TraceEnd,
		EndSurfaceHitResult,
		FLinearColor::Green,
		true);

	if (!EndSurfaceHitResult.bBlockingHit)
	{
		return InEffectorWorldLocation;
	}


	FVector CorrectedEffectorWS = InEffectorWorldLocation;
	const float EndHeightOffset = FMath::Abs(ChildBoneWS.Z - TraceBoneWS.Z);
	CorrectedEffectorWS.Z = EndSurfaceHitResult.ImpactPoint.Z + EndHeightOffset + AdditionalOffsetZ;

	return CorrectedEffectorWS;
}

void FAnimNode_CustomAimSolver::ApplyLineTrace(
	const FAnimationUpdateContext& Context,
	const FVector& StartLocation,
	const FVector& EndLocation,
	FHitResult& OutHitResult,
	const FLinearColor& DebugColor,
	const bool bRenderTrace)
{

	const auto SK = Context.AnimInstanceProxy->GetSkelMeshComponent();

	const float OwnerScale = ComponentScale;
	const float SelectedRadius = Trace_Radius;
	bool bIgnoreOverlaps = false;
	TArray<FHitResult> HitResults;

	ECollisionChannel CollisionChannel = UEngineTypes::ConvertToCollisionChannel(TraceChannel);

	switch (RaycastTraceType)
	{
	case EIKRaycastType::LineTrace:
	{
		Wv::Anim::LineTraceMulti(SK, StartLocation, EndLocation, CollisionChannel,
			true, bIgnoreOverlaps, IgnoreActors, HitResults);
	}
	break;
	case EIKRaycastType::SphereTrace:
	{
		Wv::Anim::SphereTraceMulti(SK, StartLocation, EndLocation, SelectedRadius, CollisionChannel,
			true, bIgnoreOverlaps, IgnoreActors, HitResults);
	}
	break;
	case EIKRaycastType::BoxTrace:
	{
		const FVector& Extent = FVector(1, 1, 0) * SelectedRadius;
		Wv::Anim::BoxTraceMulti(SK, StartLocation, EndLocation, Extent, FQuat::Identity, CollisionChannel,
			true, bIgnoreOverlaps, IgnoreActors, HitResults);
	}
	break;
	}

	if (bRenderTrace)
	{
		TraceStartList.Add(StartLocation);
		TraceEndList.Add(EndLocation);
		TraceRadiusList.Add(SelectedRadius);
	}

	bool bHit = false;
	UQuadrupedIKLibrary::HandlePickHitResult(HitResults, OutHitResult, bHit, NormalDotThreshold);


#if ENABLE_DRAW_DEBUG
	if (bDisplayLineTrace)
	{
		TWeakObjectPtr<UWorld> Editor_World = GEngine->GetWorldFromContextObject(SK, EGetWorldErrorMode::LogAndReturnNull);

		switch (RaycastTraceType)
		{
		case EIKRaycastType::LineTrace:
		{
			FFunctionGraphTask::CreateAndDispatchWhenReady(
				[=]() {
					if (Editor_World.IsValid())
					{
						Wv::Anim::DrawDebugLineTraceSingle(Editor_World.Get(), StartLocation, EndLocation, OutHitResult.bBlockingHit,
							OutHitResult, FLinearColor::Red, DebugColor);
					}
				},
				TStatId(), nullptr, ENamedThreads::GameThread
			);
		}
		break;
		case EIKRaycastType::SphereTrace:
		{
			FFunctionGraphTask::CreateAndDispatchWhenReady(
				[=]() {
					if (Editor_World.IsValid())
					{
						Wv::Anim::DrawDebugSphereTraceSingle(Editor_World.Get(), StartLocation, EndLocation, SelectedRadius,
							OutHitResult.bBlockingHit, OutHitResult, FLinearColor::Red, DebugColor);
					}
				},
				TStatId(), nullptr, ENamedThreads::GameThread
			);
		}
		break;
		case EIKRaycastType::BoxTrace:
		{
			FFunctionGraphTask::CreateAndDispatchWhenReady(
				[=]() {
					if (Editor_World.IsValid())
					{
						const FVector& Extent = FVector(1, 1, 0) * SelectedRadius;
						Wv::Anim::DrawDebugBoxTraceSingle(Editor_World.Get(), StartLocation, EndLocation, Extent,
							FQuat::Identity, OutHitResult.bBlockingHit, OutHitResult, FLinearColor::Red, DebugColor);
					}
				},
				TStatId(), nullptr, ENamedThreads::GameThread
			);
		}
		break;
		}
	}
#endif

}



void FAnimNode_CustomAimSolver::DrawHorizontalAngleDebugArc(
	USkeletalMeshComponent* MeshComp,
	FPrimitiveDrawInterface* PDI,
	FAnimInstanceProxy* Proxy) const
{
	if (!MeshComp)
	{
		return;
	}

	UWorld* World = MeshComp->GetWorld();

	// PDI も Proxy も無い場合は World 経由でしか描けない
	if (!World && !PDI && !Proxy)
	{
		return;
	}

	auto DrawLine = [&](const FVector& A, const FVector& B, const FColor& InColor, float Thickness)
		{
			if (PDI)
			{
				PDI->DrawLine(A, B, FLinearColor(InColor), SDPG_Foreground, Thickness);
			}
			else if (Proxy)
			{
				Proxy->AnimDrawDebugLine(A, B, InColor, false, -1.0f, Thickness);
			}
			else if (World)
			{
				DrawDebugLine(World, A, B, InColor, false, 0.0f, 0, Thickness);
			}
		};

	FVector Origin = DebugClampOriginWS;
	FVector RawTarget = DebugClampRawTargetWS;
	FVector ClampedTarget = DebugClampResultTargetWS;

	// 円弧を足元の高さに揃えて見やすくする
	{
		const float Offset = 10.0f;
		const float ComponentZ = MeshComp->GetComponentLocation().Z + Offset;

		Origin.Z = ComponentZ;
		RawTarget.Z = ComponentZ;
		ClampedTarget.Z = ComponentZ;
	}

	FVector Forward = DebugClampForwardWS.GetSafeNormal();
	Forward.Z = 0.0f;

	if (!Forward.Normalize())
	{
		return;
	}

	const float Radius = HorizontalDebugArcRadius * ComponentScale;
	const int32 Segments = FMath::Max(8, HorizontalDebugArcSegments);

	FVector Right = FVector::CrossProduct(Forward, FVector::UpVector).GetSafeNormal();
	if (Right.IsNearlyZero())
	{
		Right = FVector::RightVector;
	}

	auto MakeDirFromHorizontalDeg = [&](float InDeg) -> FVector
		{
			const float Rad = FMath::DegreesToRadians(InDeg);
			const float CosA = FMath::Cos(Rad);
			const float SinA = FMath::Sin(Rad);
			return (Forward * CosA + Right * SinA).GetSafeNormal();
		};

	// 正面方向
	DrawLine(Origin, Origin + Forward * Radius, HorizontalDebugForwardColor, 2.0f);

	// clamp 前のターゲット方向
	const FVector RawDir = (RawTarget - Origin).GetSafeNormal();
	if (!RawDir.IsNearlyZero())
	{
		DrawLine(Origin, Origin + RawDir * Radius, HorizontalDebugRawColor, 2.0f);
	}

	// clamp 後のターゲット方向
	const FVector ClampedDir = (ClampedTarget - Origin).GetSafeNormal();
	if (!ClampedDir.IsNearlyZero())
	{
		DrawLine(Origin, Origin + ClampedDir * Radius, HorizontalDebugClampedColor, 3.0f);
	}

	const float CircleThickness = 12.0f;

	// 可動域の両端
	const FVector MinDir = MakeDirFromHorizontalDeg(DebugHorizontalClampMinDeg);
	const FVector MaxDir = MakeDirFromHorizontalDeg(DebugHorizontalClampMaxDeg);

	DrawLine(Origin, Origin + MinDir * Radius, HorizontalDebugArcColor, CircleThickness);
	DrawLine(Origin, Origin + MaxDir * Radius, HorizontalDebugArcColor, CircleThickness);

	// 可動域の円弧
	float StartDeg = DebugHorizontalClampMinDeg;
	float EndDeg = DebugHorizontalClampMaxDeg;

	if (EndDeg < StartDeg)
	{
		EndDeg += 360.0f;
	}

	FVector PrevPoint = Origin + MakeDirFromHorizontalDeg(StartDeg) * Radius;

	for (int32 SegIdx = 1; SegIdx <= Segments; ++SegIdx)
	{
		const float T = static_cast<float>(SegIdx) / static_cast<float>(Segments);
		const float Deg = FMath::Lerp(StartDeg, EndDeg, T);
		const FVector CurrPoint = Origin + MakeDirFromHorizontalDeg(Deg) * Radius;

		DrawLine(PrevPoint, CurrPoint, HorizontalDebugArcColor, CircleThickness);

		PrevPoint = CurrPoint;
	}
}


EAimSolverType FAnimNode_CustomAimSolver::GetAimSolverType() const
{
	return AimSolverType;
}

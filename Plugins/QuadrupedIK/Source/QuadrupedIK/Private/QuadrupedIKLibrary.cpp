// Copyright 2022 wevet works All Rights Reserved.


#include "QuadrupedIKLibrary.h"
#include "Animation/AnimInstanceProxy.h"
//#include "DrawDebugHelpers.h"
//#include "AnimationRuntime.h"
#include "AnimationCoreLibrary.h"
#include "Algo/Reverse.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "CommonAnimTypes.h"

#include "GameFramework/Character.h"



FRotator UQuadrupedIKLibrary::CustomLookRotation(const FVector& LookAt, const FVector& UpDirection)
{
	FVector Forward = LookAt;
	FVector Up = UpDirection;
	Forward = Forward.GetSafeNormal();
	Up = Up - (Forward * FVector::DotProduct(Up, Forward));
	Up = Up.GetSafeNormal();

	FVector Position = Forward.GetSafeNormal();
	FVector Position2 = FVector::CrossProduct(Up, Position);
	FVector Position3 = FVector::CrossProduct(Position, Position2);
	const float M00 = Position2.X;
	const float M01 = Position2.Y;
	const float M02 = Position2.Z;
	const float M10 = Position3.X;
	const float M11 = Position3.Y;
	const float M12 = Position3.Z;
	const float M20 = Position.X;
	const float M21 = Position.Y;
	const float M22 = Position.Z;
	const float Num8 = (M00 + M11) + M22;
	FQuat Quaternion = FQuat();
	if (Num8 > 0.0f)
	{
		float Num = (float)FMath::Sqrt(Num8 + 1.0f);
		Quaternion.W = Num * 0.5f;
		Num = 0.5f / Num;
		Quaternion.X = (M12 - M21) * Num;
		Quaternion.Y = (M20 - M02) * Num;
		Quaternion.Z = (M01 - M10) * Num;
		return FRotator(Quaternion);
	}
	if ((M00 >= M11) && (M00 >= M22))
	{
		const float Num7 = (float)FMath::Sqrt(((1.0f + M00) - M11) - M22);
		const float Num4 = 0.5f / Num7;
		Quaternion.X = 0.5f * Num7;
		Quaternion.Y = (M01 + M10) * Num4;
		Quaternion.Z = (M02 + M20) * Num4;
		Quaternion.W = (M12 - M21) * Num4;
		return FRotator(Quaternion);
	}
	if (M11 > M22)
	{
		const float Num6 = (float)FMath::Sqrt(((1.0f + M11) - M00) - M22);
		const float Num3 = 0.5f / Num6;
		Quaternion.X = (M10 + M01) * Num3;
		Quaternion.Y = 0.5f * Num6;
		Quaternion.Z = (M21 + M12) * Num3;
		Quaternion.W = (M20 - M02) * Num3;
		return FRotator(Quaternion);
	}
	const float Num5 = (float)FMath::Sqrt(((1.0f + M22) - M00) - M11);
	const float Num2 = 0.5f / Num5;
	Quaternion.X = (M20 + M02) * Num2;
	Quaternion.Y = (M21 + M12) * Num2;
	Quaternion.Z = 0.5f * Num5;
	Quaternion.W = (M01 - M10) * Num2;
	return FRotator(Quaternion);
}

const FName UQuadrupedIKLibrary::GetChildBone(const FName& BoneName, const USkeletalMeshComponent* SkeletalMeshComponent)
{
	return SkeletalMeshComponent->GetBoneName(SkeletalMeshComponent->GetBoneIndex(BoneName) + 1);
}

const FVector UQuadrupedIKLibrary::SmoothApproach(
	const float InDeltaTimeSeconds,
	const FVector& PastPosition,
	const FVector& PastTargetPosition,
	const FVector& TargetPosition,
	const float Speed) 
{
	const float T = InDeltaTimeSeconds * Speed;
	const FVector V = (TargetPosition - PastTargetPosition) / T;
	const FVector F = PastPosition - PastTargetPosition + V;
	return TargetPosition - V + F * FMath::Exp(-T);
}



const FVector UQuadrupedIKLibrary::RotateAroundPoint(const FVector& InputPoint, const FVector& ForwardVector, const FVector& Origin, const float Angle)
{
	const FVector Direction = InputPoint - Origin;
	const FVector Axis = UKismetMathLibrary::RotateAngleAxis(Direction, Angle, ForwardVector);
	const FVector Result = InputPoint + (Axis - Direction);
	return Result;
}


const FVector UQuadrupedIKLibrary::ClampRotateVector(
	const FVector& InputPosition,
	const FVector& ForwardDirection,
	const FVector& OriginLocation,
	const float MinClampDegrees,
	const float MaxClampDegrees,
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
	float Max_Vertical_Angle = MaxClampDegrees;
	float Min_Vertical_Angle = MinClampDegrees;
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

	const float Selected_Clamp_Value = FMath::Lerp(Vertical_Degree_Priority, Horizontal_Degree_Priority,
		FMath::Clamp(FMath::Abs(Dir_Horizontal), 0.0f, 1.0f));

	float Alpha = (Selected_Clamp_Value / (FMath::Max(Selected_Clamp_Value, Degrees)));
	Alpha = FMath::Clamp(Alpha, 0.0f, 1.0f);

	const FVector OutputRot = UKismetMathLibrary::VLerp(Rot1_V, Rot2_V, Alpha);
	return (OriginLocation + (OutputRot.GetSafeNormal() * Magnitude));
}

const bool UQuadrupedIKLibrary::DoesContainsNaN(const TArray<FBoneTransform>& BoneTransforms)
{
	for (int32 Index = 0; Index < BoneTransforms.Num(); ++Index)
	{
		if (BoneTransforms[Index].Transform.ContainsNaN())
		{
			return true;
		}
	}
	return false;
}

void UQuadrupedIKLibrary::HandlePickHitResult(const TArray<FHitResult>& Hits, FHitResult& OutHitResult, bool& bIsOutHit, const float NormalDotThreshold)
{
	for (const FHitResult& Hit : Hits)
	{
		const UPrimitiveComponent* HitComponent = Hit.GetComponent();
		if (!IsValid(HitComponent))
		{
			continue;
		}

		if (!HitComponent->IsPhysicsCollisionEnabled())
		{
			continue;
		}

		if (Hit.GetActor())
		{
			if (Hit.GetActor()->IsA(ACharacter::StaticClass()))
			{
				continue;
			}
		}

		const float ImpactNormalCos = FVector::DotProduct(FVector::ZAxisVector, Hit.ImpactNormal.GetSafeNormal());
		if (ImpactNormalCos > NormalDotThreshold)
		{
			OutHitResult = Hit;
			bIsOutHit = true;
			break;
		}
	}
}


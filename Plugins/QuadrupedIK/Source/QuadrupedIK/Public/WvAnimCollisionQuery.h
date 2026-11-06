// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CustomIKData.h"
#include "DrawDebugHelpers.h"

namespace Wv::Anim
{
#if ENABLE_DRAW_DEBUG
	void DrawDebugLineTraceSingle(const UWorld* World, const FVector& Start, const FVector& End, bool bHit, const FHitResult& OutHit, FLinearColor TraceColor, FLinearColor TraceHitColor);
	void DrawDebugSweptSphere(const UWorld* InWorld, FVector const& Start, FVector const& End, float Radius, FColor const& Color, bool bPersistentLines, float LifeTime, uint8 DepthPriority);
	void DrawDebugSphereTraceSingle(const UWorld* World, const FVector& Start, const FVector& End, float Radius, bool bHit, const FHitResult& OutHit, FLinearColor TraceColor, FLinearColor TraceHitColor);
	void DrawDebugSweptBox(const UWorld* InWorld, FVector const& Start, FVector const& End, FRotator const & Orientation, FVector const & HalfSize, FColor const& Color, bool bPersistentLines, float LifeTime, uint8 DepthPriority);
	void DrawDebugBoxTraceSingle(const UWorld* World, const FVector& Start, const FVector& End, const FVector HalfSize, const FQuat Orientation, bool bHit, const FHitResult& OutHit, FLinearColor TraceColor, FLinearColor TraceHitColor);
#endif // ENABLE_DRAW_DEBUG

	bool LineTraceMulti(const class USkeletalMeshComponent* Mesh, const FVector Start, const FVector End, ECollisionChannel CollisionChannel, bool bTraceComplex, bool bIgnoreOverlaps, const TArray<class AActor*>& ActorsToIgnore, TArray<FHitResult>& OutHits);
	bool SphereTraceMulti(const class USkeletalMeshComponent* Mesh, const FVector Start, const FVector End, float Radius, ECollisionChannel CollisionChannel, bool bTraceComplex, bool bIgnoreOverlaps, const TArray<AActor*>& ActorsToIgnore, TArray<FHitResult>& OutHits);
	bool BoxTraceMulti(const class USkeletalMeshComponent* Mesh, const FVector Start, const FVector End, const FVector HalfSize, const FQuat Orientation, ECollisionChannel CollisionChannel, bool bTraceComplex, bool bIgnoreOverlaps, const TArray<class AActor*>& ActorsToIgnore, TArray<FHitResult>& OutHits);

	bool LineTraceSingle(const class USkeletalMeshComponent* Mesh, const FVector Start, const FVector End, const TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypes, bool bTraceComplex, bool bIgnoreOverlaps, const TArray<class AActor*>& ActorsToIgnore, bool bEnableDebugDraw, FHitResult& OutHit);
	bool SphereTraceSingle(const class USkeletalMeshComponent* Mesh, const FVector Start, const FVector End, float Radius, const TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypes, bool bTraceComplex, bool bIgnoreOverlaps, const TArray<class AActor*>& ActorsToIgnore, bool bEnableDebugDraw, FHitResult& OutHit);
	bool BoxTraceSingle(const class USkeletalMeshComponent* Mesh, const FVector Start, const FVector End, const FVector HalfSize, const FQuat Orientation, const TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypes, bool bTraceComplex, bool bIgnoreOverlaps, const TArray<class AActor*>& ActorsToIgnore, bool bEnableDebugDraw, FHitResult& OutHit);

	bool LineTraceMulti(const class USkeletalMeshComponent* Mesh, const FVector Start, const FVector End, const TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypes, bool bTraceComplex, bool bIgnoreOverlaps, const TArray<class AActor*>& ActorsToIgnore, TArray<FHitResult>& OutHits);
	bool SphereTraceMulti(const class USkeletalMeshComponent* Mesh, const FVector Start, const FVector End, float Radius, const TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypes, bool bTraceComplex, bool bIgnoreOverlaps, const TArray<class AActor*>& ActorsToIgnore, TArray<FHitResult>& OutHits);
	bool BoxTraceMulti(const class USkeletalMeshComponent* Mesh, const FVector Start, const FVector End, const FVector HalfSize, const FQuat Orientation, const TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypes, bool bTraceComplex, bool bIgnoreOverlaps, const TArray<class AActor*>& ActorsToIgnore, TArray<FHitResult>& OutHits);
}


namespace Wv::IK::Memory
{
	template<typename T>
	FORCEINLINE int64 GetArrayMemoryBytes_Shallow(const TArray<T>& Array)
	{
		return sizeof(Array) + Array.GetAllocatedSize();
	}

	FORCEINLINE int64 GetFingerHitArrayMemoryBytes(const TArray<TArray<FHitResult>>& Arrays)
	{
		int64 Bytes = sizeof(Arrays) + Arrays.GetAllocatedSize();
		for (const TArray<FHitResult>& Inner : Arrays)
		{
			Bytes += sizeof(Inner) + Inner.GetAllocatedSize();
		}
		return Bytes;
	}

	FORCEINLINE int64 GetFingerTransformArrayMemoryBytes(const TArray<TArray<FTransform>>& Arrays)
	{
		int64 Bytes = sizeof(Arrays) + Arrays.GetAllocatedSize();
		for (const TArray<FTransform>& Inner : Arrays)
		{
			Bytes += sizeof(Inner) + Inner.GetAllocatedSize();
		}
		return Bytes;
	}

	FORCEINLINE int64 GetFingerBoneRefArrayMemoryBytes(const TArray<TArray<FBoneReference>>& Arrays)
	{
		int64 Bytes = sizeof(Arrays) + Arrays.GetAllocatedSize();
		for (const TArray<FBoneReference>& Inner : Arrays)
		{
			Bytes += sizeof(Inner) + Inner.GetAllocatedSize();
		}
		return Bytes;
	}

	FORCEINLINE int64 GetFingerIntArrayMemoryBytes(const TArray<TArray<int32>>& Arrays)
	{
		int64 Bytes = sizeof(Arrays) + Arrays.GetAllocatedSize();
		for (const TArray<int32>& Inner : Arrays)
		{
			Bytes += sizeof(Inner) + Inner.GetAllocatedSize();
		}
		return Bytes;
	}

	FORCEINLINE int64 GetTripleTransformArrayMemoryBytes(const TArray<TArray<TArray<FTransform>>>& Arrays)
	{
		int64 Bytes = sizeof(Arrays) + Arrays.GetAllocatedSize();
		for (const TArray<TArray<FTransform>>& Mid : Arrays)
		{
			Bytes += sizeof(Mid) + Mid.GetAllocatedSize();
			for (const TArray<FTransform>& Inner : Mid)
			{
				Bytes += sizeof(Inner) + Inner.GetAllocatedSize();
			}
		}
		return Bytes;
	}

	FORCEINLINE int64 GetDoubleFloatArrayMemoryBytes(const TArray<TArray<float>>& Arrays)
	{
		int64 Bytes = sizeof(Arrays) + Arrays.GetAllocatedSize();
		for (const TArray<float>& Inner : Arrays)
		{
			Bytes += sizeof(Inner) + Inner.GetAllocatedSize();
		}
		return Bytes;
	}

	FORCEINLINE int64 GetDoubleVectorArrayMemoryBytes(const TArray<TArray<FVector>>& Arrays)
	{
		int64 Bytes = sizeof(Arrays) + Arrays.GetAllocatedSize();
		for (const TArray<FVector>& Inner : Arrays)
		{
			Bytes += sizeof(Inner) + Inner.GetAllocatedSize();
		}
		return Bytes;
	}

	FORCEINLINE int64 GetDoubleTransformArrayMemoryBytes(const TArray<TArray<FTransform>>& Arrays)
	{
		int64 Bytes = sizeof(Arrays) + Arrays.GetAllocatedSize();
		for (const TArray<FTransform>& Inner : Arrays)
		{
			Bytes += sizeof(Inner) + Inner.GetAllocatedSize();
		}
		return Bytes;
	}

	FORCEINLINE int64 GetHitPairsMemoryBytes(const TArray<FQuadlupedBoneHitPairs>& HitPairs)
	{
		int64 Bytes = sizeof(HitPairs) + HitPairs.GetAllocatedSize();

		for (const FQuadlupedBoneHitPairs& Pair : HitPairs)
		{
			Bytes += sizeof(Pair);

			Bytes += sizeof(Pair.FeetHitArray) + Pair.FeetHitArray.GetAllocatedSize();
			Bytes += sizeof(Pair.FeetFrontHitArray) + Pair.FeetFrontHitArray.GetAllocatedSize();
			Bytes += sizeof(Pair.FeetLeftHitArray) + Pair.FeetLeftHitArray.GetAllocatedSize();
			Bytes += sizeof(Pair.FeetRightHitArray) + Pair.FeetRightHitArray.GetAllocatedSize();
			Bytes += sizeof(Pair.ToeHitArray) + Pair.ToeHitArray.GetAllocatedSize();
			Bytes += sizeof(Pair.FeetHitPointArray) + Pair.FeetHitPointArray.GetAllocatedSize();

			Bytes += GetFingerHitArrayMemoryBytes(Pair.FingerHitArray);
			Bytes += GetFingerHitArrayMemoryBytes(Pair.OriginalFingerHitArray);
		}

		return Bytes;
	}

	FORCEINLINE int64 GetSpineFeetPairMemoryBytes(const TArray<FQuadrupedBone_SpineFeetPair>& Pairs)
	{
		int64 Bytes = sizeof(Pairs) + Pairs.GetAllocatedSize();

		for (const FQuadrupedBone_SpineFeetPair& Pair : Pairs)
		{
			Bytes += sizeof(Pair);

			Bytes += sizeof(Pair.FeetArray) + Pair.FeetArray.GetAllocatedSize();
			Bytes += sizeof(Pair.KneeArray) + Pair.KneeArray.GetAllocatedSize();
			Bytes += sizeof(Pair.ThighArray) + Pair.ThighArray.GetAllocatedSize();
			Bytes += sizeof(Pair.ToeArray) + Pair.ToeArray.GetAllocatedSize();

			Bytes += sizeof(Pair.FeetRotationOffsetArray) + Pair.FeetRotationOffsetArray.GetAllocatedSize();
			Bytes += sizeof(Pair.FeetHeightArray) + Pair.FeetHeightArray.GetAllocatedSize();
			Bytes += sizeof(Pair.FeetRotationLimitArray) + Pair.FeetRotationLimitArray.GetAllocatedSize();
			Bytes += sizeof(Pair.OrderIndexArray) + Pair.OrderIndexArray.GetAllocatedSize();
			Bytes += sizeof(Pair.FeetTraceOffsetArray) + Pair.FeetTraceOffsetArray.GetAllocatedSize();

			Bytes += GetFingerBoneRefArrayMemoryBytes(Pair.FingerArray);
			Bytes += GetFingerIntArrayMemoryBytes(Pair.FingerChainNumArray);
		}

		return Bytes;
	}

	FORCEINLINE int64 GetSpineFeetPairWSMemoryBytes(const TArray<FQuadlupedBoneSpineFeetPair_WS>& Pairs)
	{
		int64 Bytes = sizeof(Pairs) + Pairs.GetAllocatedSize();

		for (const FQuadlupedBoneSpineFeetPair_WS& Pair : Pairs)
		{
			Bytes += sizeof(Pair);

			Bytes += sizeof(Pair.AssociatedFootArray) + Pair.AssociatedFootArray.GetAllocatedSize();
			Bytes += sizeof(Pair.AssociatedToeArray) + Pair.AssociatedToeArray.GetAllocatedSize();
			Bytes += sizeof(Pair.AssociatedKneeArray) + Pair.AssociatedKneeArray.GetAllocatedSize();
			Bytes += GetFingerTransformArrayMemoryBytes(Pair.AssociatedFingerArray);
		}

		return Bytes;
	}
}
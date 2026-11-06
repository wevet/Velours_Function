// Copyright 2022 wevet works All Rights Reserved.

#include "WvAnimCollisionQuery.h"

namespace Wv::Anim
{
#if ENABLE_DRAW_DEBUG
	static const float Wv_TRACE_DEBUG_IMPACTPOINT_SIZE = 8.0f;

	void DrawDebugLineTraceSingle(const UWorld* World, const FVector& Start, const FVector& End,
		bool bHit, const FHitResult& OutHit, FLinearColor TraceColor, FLinearColor TraceHitColor)
	{
		bool bPersistent = false;
		float LifeTime = 0.0f;

		if (bHit && OutHit.bBlockingHit)
		{
			::DrawDebugLine(World, Start, OutHit.ImpactPoint, TraceColor.ToFColor(true), bPersistent, LifeTime, ESceneDepthPriorityGroup::SDPG_Foreground);
			::DrawDebugLine(World, OutHit.ImpactPoint, End, TraceHitColor.ToFColor(true), bPersistent, LifeTime, ESceneDepthPriorityGroup::SDPG_Foreground);
			::DrawDebugPoint(World, OutHit.ImpactPoint, Wv_TRACE_DEBUG_IMPACTPOINT_SIZE, TraceColor.ToFColor(true), bPersistent, LifeTime, ESceneDepthPriorityGroup::SDPG_Foreground);
		}
		else
		{
			::DrawDebugLine(World, Start, End, TraceColor.ToFColor(true), bPersistent, LifeTime, ESceneDepthPriorityGroup::SDPG_Foreground);
		}
	}

	void DrawDebugSweptSphere(const UWorld* InWorld, FVector const& Start, FVector const& End, 
		float Radius, FColor const& Color, bool bPersistentLines, float LifeTime, uint8 DepthPriority)
	{
		FVector const TraceVec = End - Start;
		float const Dist = TraceVec.Size();

		FVector const Center = Start + TraceVec * 0.5f;
		float const HalfHeight = (Dist * 0.5f) + Radius;

		FQuat const CapsuleRot = FRotationMatrix::MakeFromZ(TraceVec).ToQuat();
		::DrawDebugCapsule(InWorld, Center, HalfHeight, Radius, CapsuleRot, Color, bPersistentLines, LifeTime, DepthPriority);
	}

	void DrawDebugSphereTraceSingle(const UWorld* World, const FVector& Start, const FVector& End,
		float Radius, bool bHit, const FHitResult& OutHit, FLinearColor TraceColor, FLinearColor TraceHitColor)
	{
		bool bPersistent = false;
		float LifeTime = 0.0f;

		if (bHit && OutHit.bBlockingHit)
		{
			// Red up to the blocking hit, green thereafter
			Wv::Anim::DrawDebugSweptSphere(World, Start, OutHit.Location, Radius, TraceColor.ToFColor(true), bPersistent, LifeTime, ESceneDepthPriorityGroup::SDPG_Foreground);
			Wv::Anim::DrawDebugSweptSphere(World, OutHit.Location, End, Radius, TraceHitColor.ToFColor(true), bPersistent, LifeTime, ESceneDepthPriorityGroup::SDPG_Foreground);
			::DrawDebugPoint(World, OutHit.ImpactPoint, Wv_TRACE_DEBUG_IMPACTPOINT_SIZE, TraceColor.ToFColor(true), bPersistent, LifeTime, ESceneDepthPriorityGroup::SDPG_Foreground);
		}
		else
		{
			// no hit means all red
			Wv::Anim::DrawDebugSweptSphere(World, Start, End, Radius, TraceColor.ToFColor(true), bPersistent, LifeTime, ESceneDepthPriorityGroup::SDPG_Foreground);
		}
	}

	void DrawDebugSweptBox(
		const UWorld* InWorld, 
		FVector const& Start,
		FVector const& End, 
		FQuat const & Orientation,
		FVector const & HalfSize, 
		FColor const& Color, 
		bool bPersistentLines, 
		float LifeTime, 
		uint8 DepthPriority)
	{
		FVector const TraceVec = End - Start;
		float const Dist = TraceVec.Size();

		FVector const Center = Start + TraceVec * 0.5f;

		FQuat const CapsuleRot = Orientation;
		::DrawDebugBox(InWorld, Start, HalfSize, CapsuleRot, Color, bPersistentLines, LifeTime, DepthPriority);

		//now draw lines from vertices
		FVector Vertices[8];
		Vertices[0] = Start + CapsuleRot.RotateVector(FVector(-HalfSize.X, -HalfSize.Y, -HalfSize.Z));	//flt
		Vertices[1] = Start + CapsuleRot.RotateVector(FVector(-HalfSize.X, HalfSize.Y, -HalfSize.Z));	//frt
		Vertices[2] = Start + CapsuleRot.RotateVector(FVector(-HalfSize.X, -HalfSize.Y, HalfSize.Z));	//flb
		Vertices[3] = Start + CapsuleRot.RotateVector(FVector(-HalfSize.X, HalfSize.Y, HalfSize.Z));	//frb
		Vertices[4] = Start + CapsuleRot.RotateVector(FVector(HalfSize.X, -HalfSize.Y, -HalfSize.Z));	//blt
		Vertices[5] = Start + CapsuleRot.RotateVector(FVector(HalfSize.X, HalfSize.Y, -HalfSize.Z));	//brt
		Vertices[6] = Start + CapsuleRot.RotateVector(FVector(HalfSize.X, -HalfSize.Y, HalfSize.Z));	//blb
		Vertices[7] = Start + CapsuleRot.RotateVector(FVector(HalfSize.X, HalfSize.Y, HalfSize.Z));		//brb
		for (int32 VertexIdx = 0; VertexIdx < 8; ++VertexIdx)
		{
			::DrawDebugLine(InWorld, Vertices[VertexIdx], Vertices[VertexIdx] + TraceVec, Color, bPersistentLines, LifeTime, DepthPriority);
		}

		::DrawDebugBox(InWorld, End, HalfSize, CapsuleRot, Color, bPersistentLines, LifeTime, DepthPriority);
	}

	void DrawDebugBoxTraceSingle(
		const UWorld* World, 
		const FVector& Start, 
		const FVector& End, 
		const FVector HalfSize, 
		const FQuat Orientation, 
		const bool bHit,
		const FHitResult& OutHit, 
		FLinearColor TraceColor, 
		FLinearColor TraceHitColor)
	{
		bool bPersistent = false;
		float LifeTime = 0.0f;

		if (bHit && OutHit.bBlockingHit)
		{
			// Red up to the blocking hit, green thereafter
			Wv::Anim::DrawDebugSweptBox(World, Start, OutHit.Location, Orientation, HalfSize, TraceColor.ToFColor(true), bPersistent, LifeTime, ESceneDepthPriorityGroup::SDPG_Foreground);
			Wv::Anim::DrawDebugSweptBox(World, OutHit.Location, End, Orientation, HalfSize, TraceHitColor.ToFColor(true), bPersistent, LifeTime, ESceneDepthPriorityGroup::SDPG_Foreground);
			::DrawDebugPoint(World, OutHit.ImpactPoint, Wv_TRACE_DEBUG_IMPACTPOINT_SIZE, TraceColor.ToFColor(true), bPersistent, LifeTime, ESceneDepthPriorityGroup::SDPG_Foreground);
		}
		else
		{
			// no hit means all red
			Wv::Anim::DrawDebugSweptBox(World, Start, End, Orientation, HalfSize, TraceColor.ToFColor(true), bPersistent, LifeTime, ESceneDepthPriorityGroup::SDPG_Foreground);
		}
	}
#endif // ENABLE_DRAW_DEBUG


	FCollisionObjectQueryParams ConfigureCollisionObjectParams(const TArray<TEnumAsByte<EObjectTypeQuery> > & ObjectTypes)
	{
		TArray<TEnumAsByte<ECollisionChannel>> CollisionObjectTraces;
		CollisionObjectTraces.AddUninitialized(ObjectTypes.Num());

		for (auto Iter = ObjectTypes.CreateConstIterator(); Iter; ++Iter)
		{
			CollisionObjectTraces[Iter.GetIndex()] = UEngineTypes::ConvertToCollisionChannel(*Iter);
		}

		FCollisionObjectQueryParams ObjectParams;
		for (auto Iter = CollisionObjectTraces.CreateConstIterator(); Iter; ++Iter)
		{
			const ECollisionChannel & Channel = (*Iter);
			if (FCollisionObjectQueryParams::IsValidObjectQuery(Channel))
			{
				ObjectParams.AddObjectTypesToQuery(Channel);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("%d isn't valid object type"), (int32)Channel);
			}
		}

		return ObjectParams;
	}

	/// <summary>
	/// Valid GT Check Function
	/// </summary>
	bool IsValidWorldChecked(const class USkeletalMeshComponent* Mesh, const FVector Start, const FVector End)
	{
		if (Start.ContainsNaN() || End.ContainsNaN())
		{
			return false;
		}

		if (!IsValid(Mesh) || !Mesh->IsRegistered())
		{
			return false;
		}

		UWorld* World = Mesh->GetWorld();
		if (!World)
		{
			return false;
		}

		if (!World->IsGameWorld())
		{
			return false;
		}

		if (!World->GetPhysicsScene())
		{
			return false;
		}

		const AActor* Owner = Mesh->GetOwner();
		if (!IsValid(Owner))
		{
			return false;
		}

		// 「World に spawn 済みだが開始前」で落ちる
		// BeginPlay 前の Anim 初期化中には Chaos scene query を打たない。
		if (!World->HasBegunPlay() || !Owner->HasActorBegunPlay())
		{
			return false;
		}

		if (!Mesh->HasBegunPlay())
		{
			return false;
		}

		return true;
	}


	bool LineTraceMulti(
		const class USkeletalMeshComponent* Mesh, 
		const FVector Start, 
		const FVector End, 
		const ECollisionChannel CollisionChannel,
		const bool bTraceComplex,
		const bool bIgnoreOverlaps,
		const TArray<class AActor*>& ActorsToIgnore, 
		TArray<FHitResult>& OutHits)
	{

		if (!IsValidWorldChecked(Mesh, Start, End))
		{
			return false;
		}

		FCollisionQueryParams Params(NAME_None, bTraceComplex, Mesh->GetOwner());
		Params.bReturnPhysicalMaterial = false;
		Params.bIgnoreTouches = bIgnoreOverlaps;
		Params.AddIgnoredComponent(Mesh);
		Params.AddIgnoredActors(ActorsToIgnore);

		UWorld* World = GEngine->GetWorldFromContextObject(Mesh, EGetWorldErrorMode::LogAndReturnNull);
		bool const bHit = World ? World->LineTraceMultiByChannel(OutHits, Start, End, CollisionChannel, Params) : false;

		// デバッグ描画は呼び出し側で行う。
		// 複数のヒット結果に対してどれを採用するか、呼び出し側で決定することがあるので
		return bHit;
	}

	bool SphereTraceMulti(
		const class USkeletalMeshComponent* Mesh, 
		const FVector Start, 
		const FVector End,
		const float Radius,
		const ECollisionChannel CollisionChannel,
		const bool bTraceComplex,
		const bool bIgnoreOverlaps,
		const TArray<AActor*>& ActorsToIgnore, 
		TArray<FHitResult>& OutHits)
	{
		if (!IsValidWorldChecked(Mesh, Start, End))
		{
			return false;
		}

		const float SafeRadius = Radius;
		if (!FMath::IsFinite(SafeRadius) || SafeRadius <= KINDA_SMALL_NUMBER)
		{
			return false;
		}

		FCollisionQueryParams Params(NAME_None, bTraceComplex, Mesh->GetOwner());
		Params.bReturnPhysicalMaterial = false;
		Params.bIgnoreTouches = bIgnoreOverlaps;
		Params.AddIgnoredComponent(Mesh);
		Params.AddIgnoredActors(ActorsToIgnore);

		UWorld* World = GEngine->GetWorldFromContextObject(Mesh, EGetWorldErrorMode::LogAndReturnNull);
		bool const bHit = World ? World->SweepMultiByChannel(OutHits, Start, End, FQuat::Identity, CollisionChannel, FCollisionShape::MakeSphere(SafeRadius), Params) : false;

		// デバッグ描画は呼び出し側で行う。
		// 複数のヒット結果に対してどれを採用するか、呼び出し側で決定することがあるので
		return bHit;
	}

	bool BoxTraceMulti(
		const class USkeletalMeshComponent* Mesh,
		const FVector Start, 
		const FVector End, 
		const FVector HalfSize, 
		const FQuat Orientation, 
		const ECollisionChannel CollisionChannel,
		const bool bTraceComplex,
		const bool bIgnoreOverlaps,
		const TArray<class AActor*>& ActorsToIgnore, 
		TArray<FHitResult>& OutHits)
	{

		if (!IsValidWorldChecked(Mesh, Start, End))
		{
			return false;
		}

		FCollisionQueryParams Params(NAME_None, bTraceComplex, Mesh->GetOwner());
		Params.bReturnPhysicalMaterial = false;
		Params.bIgnoreTouches = bIgnoreOverlaps;
		Params.AddIgnoredComponent(Mesh);
		Params.AddIgnoredActors(ActorsToIgnore);

		UWorld* World = GEngine->GetWorldFromContextObject(Mesh, EGetWorldErrorMode::LogAndReturnNull);

		bool const bHit = World ? World->SweepMultiByChannel(OutHits, Start, End, Orientation, CollisionChannel, FCollisionShape::MakeBox(HalfSize), Params) : false;

		// デバッグ描画は呼び出し側で行う。
		// 複数のヒット結果に対してどれを採用するか、呼び出し側で決定することがあるので
		return bHit;
	}


	bool LineTraceSingle(
		const USkeletalMeshComponent* Mesh, 
		const FVector Start, 
		const FVector End, 
		const TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypes,
		const bool bTraceComplex,
		const bool bIgnoreOverlaps,
		const TArray<AActor*>& ActorsToIgnore,
		const bool bEnableDebugDraw,
		FHitResult& OutHit)
	{

		if (!IsValidWorldChecked(Mesh, Start, End))
		{
			return false;
		}


		FCollisionQueryParams Params(NAME_None, bTraceComplex, Mesh->GetOwner());
		Params.bReturnPhysicalMaterial = false;
		Params.bIgnoreTouches = bIgnoreOverlaps;
		Params.AddIgnoredComponent(Mesh);
		Params.AddIgnoredActors(ActorsToIgnore);

		FCollisionObjectQueryParams ObjectParams = ConfigureCollisionObjectParams(ObjectTypes);
		if (!ObjectParams.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("Invalid object types"));
			return false;
		}

		UWorld* World = GEngine->GetWorldFromContextObject(Mesh, EGetWorldErrorMode::LogAndReturnNull);
		bool const bHit = World ? World->LineTraceSingleByObjectType(OutHit, Start, End, ObjectParams, Params) : false;

#if ENABLE_DRAW_DEBUG
		if (bEnableDebugDraw)
		{
			FFunctionGraphTask::CreateAndDispatchWhenReady(
				[=]() {
					DrawDebugLineTraceSingle(World, Start, End, bHit, OutHit, FLinearColor::Red, FLinearColor::Green);
				},
				TStatId(), nullptr, ENamedThreads::GameThread
			);
		}
#endif

		return bHit;
	}

	bool SphereTraceSingle(
		const USkeletalMeshComponent* Mesh, 
		const FVector Start, 
		const FVector End, 
		const float Radius,
		const TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypes,
		const bool bTraceComplex,
		const bool bIgnoreOverlaps,
		const TArray<AActor*>& ActorsToIgnore, 
		const bool bEnableDebugDraw,
		FHitResult& OutHit)
	{

		if (!IsValidWorldChecked(Mesh, Start, End))
		{
			return false;
		}

		const float SafeRadius = Radius;
		if (!FMath::IsFinite(SafeRadius) || SafeRadius <= KINDA_SMALL_NUMBER)
		{
			return false;
		}


		FCollisionQueryParams Params(NAME_None, bTraceComplex, Mesh->GetOwner());
		Params.bReturnPhysicalMaterial = false;
		Params.bIgnoreTouches = bIgnoreOverlaps;
		Params.AddIgnoredComponent(Mesh);
		Params.AddIgnoredActors(ActorsToIgnore);

		FCollisionObjectQueryParams ObjectParams = ConfigureCollisionObjectParams(ObjectTypes);
		if (!ObjectParams.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("Invalid object types"));
			return false;
		}

		UWorld* World = GEngine->GetWorldFromContextObject(Mesh, EGetWorldErrorMode::LogAndReturnNull);
		bool const bHit = World ? World->SweepSingleByObjectType(OutHit, Start, End, FQuat::Identity, ObjectParams, FCollisionShape::MakeSphere(SafeRadius), Params) : false;

#if ENABLE_DRAW_DEBUG
		if (bEnableDebugDraw)
		{
			FFunctionGraphTask::CreateAndDispatchWhenReady(
				[=]() {
					DrawDebugSphereTraceSingle(World, Start, End, Radius, bHit, OutHit, FLinearColor::Red, FLinearColor::Green);
				},
				TStatId(), nullptr, ENamedThreads::GameThread
			);
		}
#endif

		return bHit;
	}

	bool BoxTraceSingle(
		const USkeletalMeshComponent* Mesh,
		const FVector Start,
		const FVector End, 
		const FVector HalfSize,
		const FQuat Orientation, 
		const TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypes,
		const bool bTraceComplex,
		const bool bIgnoreOverlaps,
		const TArray<AActor*>& ActorsToIgnore, 
		const bool bEnableDebugDraw,
		FHitResult& OutHit)
	{

		if (!IsValidWorldChecked(Mesh, Start, End))
		{
			return false;
		}

		FCollisionQueryParams Params(NAME_None, bTraceComplex, Mesh->GetOwner());
		Params.bReturnPhysicalMaterial = false;
		Params.bIgnoreTouches = bIgnoreOverlaps;
		Params.AddIgnoredComponent(Mesh);
		Params.AddIgnoredActors(ActorsToIgnore);

		FCollisionObjectQueryParams ObjectParams = ConfigureCollisionObjectParams(ObjectTypes);
		if (!ObjectParams.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("Invalid object types"));
			return false;
		}

		UWorld* World = GEngine->GetWorldFromContextObject(Mesh, EGetWorldErrorMode::LogAndReturnNull);
		bool const bHit = World ? World->SweepSingleByObjectType(OutHit, Start, End, Orientation, ObjectParams, FCollisionShape::MakeBox(HalfSize), Params) : false;

#if ENABLE_DRAW_DEBUG
		if (bEnableDebugDraw)
		{
			FFunctionGraphTask::CreateAndDispatchWhenReady(
				[=]() {
					DrawDebugBoxTraceSingle(World, Start, End, HalfSize, Orientation, bHit, OutHit, FLinearColor::Red, FLinearColor::Green);
				},
				TStatId(), nullptr, ENamedThreads::GameThread
			);
		}
#endif

		return bHit;
	}

	bool LineTraceMulti(
		const USkeletalMeshComponent* Mesh, 
		const FVector Start, 
		const FVector End, 
		const TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypes,
		const bool bTraceComplex,
		const bool bIgnoreOverlaps,
		const TArray<AActor*>& ActorsToIgnore,
		TArray<FHitResult>& OutHits)
	{

		if (!IsValidWorldChecked(Mesh, Start, End))
		{
			return false;
		}


		FCollisionQueryParams Params(NAME_None, bTraceComplex, Mesh->GetOwner());
		Params.bReturnPhysicalMaterial = false;
		Params.bIgnoreTouches = bIgnoreOverlaps;
		Params.AddIgnoredComponent(Mesh);
		Params.AddIgnoredActors(ActorsToIgnore);

		FCollisionObjectQueryParams ObjectParams = ConfigureCollisionObjectParams(ObjectTypes);
		if (!ObjectParams.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("Invalid object types"));
			return false;
		}

		UWorld* World = GEngine->GetWorldFromContextObject(Mesh, EGetWorldErrorMode::LogAndReturnNull);
		bool const bHit = World ? World->LineTraceMultiByObjectType(OutHits, Start, End, ObjectParams, Params) : false;
		return bHit;
	}

	bool SphereTraceMulti(
		const USkeletalMeshComponent* Mesh, 
		const FVector Start, 
		const FVector End, 
		const float Radius,
		const TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypes,
		const bool bTraceComplex,
		const bool bIgnoreOverlaps,
		const TArray<AActor*>& ActorsToIgnore,
		TArray<FHitResult>& OutHits)
	{

		if (!IsValidWorldChecked(Mesh, Start, End))
		{
			return false;
		}

		const float SafeRadius = Radius;
		if (!FMath::IsFinite(SafeRadius) || SafeRadius <= KINDA_SMALL_NUMBER)
		{
			return false;
		}

		FCollisionQueryParams Params(NAME_None, bTraceComplex, Mesh->GetOwner());
		Params.bReturnPhysicalMaterial = false;
		Params.bIgnoreTouches = bIgnoreOverlaps;
		Params.AddIgnoredComponent(Mesh);
		Params.AddIgnoredActors(ActorsToIgnore);

		FCollisionObjectQueryParams ObjectParams = ConfigureCollisionObjectParams(ObjectTypes);
		if (!ObjectParams.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("Invalid object types"));
			return false;
		}

		UWorld* World = GEngine->GetWorldFromContextObject(Mesh, EGetWorldErrorMode::LogAndReturnNull);
		bool const bHit = World ? World->SweepMultiByObjectType(OutHits, Start, End, FQuat::Identity, ObjectParams, FCollisionShape::MakeSphere(SafeRadius), Params) : false;

		// デバッグ描画は呼び出し側で行う。
		// 複数のヒット結果に対してどれを採用するか、呼び出し側で決定することがあるので
		return bHit;
	}

	bool BoxTraceMulti(
		const USkeletalMeshComponent* Mesh, 
		const FVector Start,
		const FVector End, 
		const FVector HalfSize, 
		const FQuat Orientation, 
		const TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypes, 
		const bool bTraceComplex,
		const bool bIgnoreOverlaps,
		const TArray<AActor*>& ActorsToIgnore,
		TArray<FHitResult>& OutHits)
	{

		if (!IsValidWorldChecked(Mesh, Start, End))
		{
			return false;
		}

		FCollisionQueryParams Params(NAME_None, bTraceComplex, Mesh->GetOwner());
		Params.bReturnPhysicalMaterial = false;
		Params.bIgnoreTouches = bIgnoreOverlaps;
		Params.AddIgnoredComponent(Mesh);
		Params.AddIgnoredActors(ActorsToIgnore);

		FCollisionObjectQueryParams ObjectParams = ConfigureCollisionObjectParams(ObjectTypes);
		if (!ObjectParams.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("Invalid object types"));
			return false;
		}

		UWorld* World = GEngine->GetWorldFromContextObject(Mesh, EGetWorldErrorMode::LogAndReturnNull);
		bool const bHit = World ? World->SweepMultiByObjectType(OutHits, Start, End, Orientation, ObjectParams, FCollisionShape::MakeBox(HalfSize), Params) : false;

		// デバッグ描画は呼び出し側で行う。
		// 複数のヒット結果に対してどれを採用するか、呼び出し側で決定することがあるので
		return bHit;
	}
}


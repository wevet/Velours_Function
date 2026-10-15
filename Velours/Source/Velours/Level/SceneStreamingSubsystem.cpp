
// Copyright 2022 wevet works All Rights Reserved.

#include "Level/SceneStreamingSubsystem.h"
#include "WorldPartition/DataLayer/DataLayerManager.h"
#include "WorldPartition/DataLayer/DataLayerInstance.h"
#include "WorldPartition/WorldPartitionSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SceneStreamingSubsystem)

DEFINE_LOG_CATEGORY_STATIC(LogSceneStreaming, Log, All)

namespace SceneStreamingHelper
{
	constexpr float PollInterval = 0.05f;
}


UDataLayerManager* USceneStreamingSubsystem::GetDataLayerManager() const
{
	UWorld* World = GetWorld();
	return World ? UDataLayerManager::GetDataLayerManager(World) : nullptr;
}

const UDataLayerInstance* USceneStreamingSubsystem::FindInstance(const TSoftObjectPtr<UDataLayerAsset>& Layer) const
{
	UDataLayerManager* Manager = GetDataLayerManager();
	if (!Manager || Layer.IsNull())
	{
		return nullptr;
	}

	const UDataLayerAsset* Asset = Layer.LoadSynchronous();
	return Asset ? Manager->GetDataLayerInstanceFromAsset(Asset) : nullptr;
}

void USceneStreamingSubsystem::SetLayerState(const TArray<TSoftObjectPtr<UDataLayerAsset>>& Layers, EDataLayerRuntimeState State)
{
	UDataLayerManager* Manager = GetDataLayerManager();
	if (!Manager)
	{
		UE_LOG(LogSceneStreaming, Warning, TEXT("no DataLayerManager => %s"), *FString(__FUNCTION__));
		return;
	}

	for (int32 Index = 0; Index < Layers.Num(); ++Index)
	{
		if (Layers[Index].IsNull())
		{
			continue;
		}

		const UDataLayerAsset* Asset = Layers[Index].LoadSynchronous();
		if (!Asset)
		{
			continue;
		}

		Manager->SetDataLayerRuntimeState(Asset, State);
	}
}


void USceneStreamingSubsystem::PreloadLayers(const TArray<TSoftObjectPtr<UDataLayerAsset>>& Layers)
{
	// 既に Activated なものを Loaded に落とさないよう、状態を見てから設定する
	UDataLayerManager* Manager = GetDataLayerManager();
	if (!Manager)
	{
		return;
	}

	for (int32 Index = 0; Index < Layers.Num(); ++Index)
	{
		if (Layers[Index].IsNull())
		{
			continue;
		}

		const UDataLayerAsset* Asset = Layers[Index].LoadSynchronous();
		if (!Asset)
		{
			continue;
		}

		const UDataLayerInstance* Instance = FindInstance(Layers[Index]);
		if (!Instance)
		{
			continue;
		}

		const EDataLayerRuntimeState Current = Manager->GetDataLayerInstanceRuntimeState(Instance);
		if (Current == EDataLayerRuntimeState::Unloaded)
		{
			Manager->SetDataLayerRuntimeState(Asset, EDataLayerRuntimeState::Loaded);
		}
	}
}


void USceneStreamingSubsystem::ActivateLayers(const TArray<TSoftObjectPtr<UDataLayerAsset>>& Layers)
{
	SetLayerState(Layers, EDataLayerRuntimeState::Activated);
}


void USceneStreamingSubsystem::DeactivateLayers(const TArray<TSoftObjectPtr<UDataLayerAsset>>& Layers)
{
	SetLayerState(Layers, EDataLayerRuntimeState::Loaded);
}


void USceneStreamingSubsystem::UnloadLayers(const TArray<TSoftObjectPtr<UDataLayerAsset>>& Layers)
{
	SetLayerState(Layers, EDataLayerRuntimeState::Unloaded);
}


bool USceneStreamingSubsystem::AreLayersReady(const TArray<TSoftObjectPtr<UDataLayerAsset>>& Layers) const
{
	UDataLayerManager* Manager = GetDataLayerManager();
	if (!Manager)
	{
		return false;
	}

	for (int32 Index = 0; Index < Layers.Num(); ++Index)
	{
		if (Layers[Index].IsNull())
		{
			continue;
		}

		const UDataLayerAsset* Asset = Layers[Index].Get();
		if (!Asset)
		{
			return false;
		}

		const UDataLayerInstance* Instance = FindInstance(Layers[Index]);
		if (!Instance)
		{
			continue;
		}

		const EDataLayerRuntimeState State = Manager->GetDataLayerInstanceRuntimeState(Instance);
		if (State == EDataLayerRuntimeState::Unloaded)
		{
			return false;
		}
	}

	// セル単位のストリーミング完了も見る
	UWorld* World = GetWorld();
	if (UWorldPartitionSubsystem* WP = World ? World->GetSubsystem<UWorldPartitionSubsystem>() : nullptr)
	{
		return WP->IsStreamingCompleted();
	}

	return true;
}


void USceneStreamingSubsystem::ActivateLayersWhenReady(
	const TArray<TSoftObjectPtr<UDataLayerAsset>>& Layers,
	float TimeoutSeconds,
	const FLayerReadyDelegate& OnReady)
{
	// 既に準備できていれば即座に
	if (AreLayersReady(Layers))
	{
		ActivateLayers(Layers);
		OnReady.ExecuteIfBound(true);
		return;
	}

	// まだなら Loaded を要求して待つ
	PreloadLayers(Layers);

	PendingLayers = Layers;
	PendingCallback = OnReady;
	PendingElapsed = 0.f;
	PendingTimeout = TimeoutSeconds;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			PollTimerHandle, this, &ThisClass::PollLayerReady,
			SceneStreamingHelper::PollInterval, true);
	}
}


void USceneStreamingSubsystem::PollLayerReady()
{
	PendingElapsed += SceneStreamingHelper::PollInterval;

	const bool bReady = AreLayersReady(PendingLayers);
	const bool bTimedOut = (PendingTimeout > 0.f && PendingElapsed >= PendingTimeout);

	if (!bReady && !bTimedOut)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PollTimerHandle);
	}

	// タイムアウトでも表示はする。呼び出し側が bSucceeded=false を見て
	// フェードを挟むかどうかを決める
	ActivateLayers(PendingLayers);

	if (bTimedOut)
	{
		UE_LOG(LogSceneStreaming, Warning, TEXT("layer activation timed out => %s"), *FString(__FUNCTION__));
	}

	PendingCallback.ExecuteIfBound(bReady);
	PendingCallback.Clear();
	PendingLayers.Reset();
}


void USceneStreamingSubsystem::SwapLayers(
	const TArray<TSoftObjectPtr<UDataLayerAsset>>& LayersToActivate,
	const TArray<TSoftObjectPtr<UDataLayerAsset>>& LayersToDeactivate)
{
	// 新しい背景を先に出してから古いほうを隠す。
	// 逆順だと一瞬どちらも無い状態が見える
	ActivateLayers(LayersToActivate);
	DeactivateLayers(LayersToDeactivate);
}


// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldPartition/DataLayer/DataLayerAsset.h"
#include "WorldPartition/DataLayer/DataLayerInstance.h"
#include "SceneStreamingSubsystem.generated.h"

class UDataLayerManager;

DECLARE_DYNAMIC_DELEGATE_OneParam(FLayerReadyDelegate, bool, bSucceeded);


/// <summary>
/// Data Layer の Loaded / Activated を分けて扱い、暗転なしの背景切り替えを可能にする
/// </summary>
UCLASS()
class VELOURS_API USceneStreamingSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// メモリに載せるだけ。非表示のまま。事前に呼んでおく
	UFUNCTION(BlueprintCallable, Category = "Streaming")
	void PreloadLayers(const TArray<TSoftObjectPtr<UDataLayerAsset>>& Layers);

	// 表示に切り替える。Preload 済みなら即座に完了する
	UFUNCTION(BlueprintCallable, Category = "Streaming")
	void ActivateLayers(const TArray<TSoftObjectPtr<UDataLayerAsset>>& Layers);

	// 非表示に戻す（Loaded のまま保持）
	UFUNCTION(BlueprintCallable, Category = "Streaming")
	void DeactivateLayers(const TArray<TSoftObjectPtr<UDataLayerAsset>>& Layers);

	// メモリから降ろす
	UFUNCTION(BlueprintCallable, Category = "Streaming")
	void UnloadLayers(const TArray<TSoftObjectPtr<UDataLayerAsset>>& Layers);

	// 指定 Layer が Loaded 以上になっているか
	UFUNCTION(BlueprintPure, Category = "Streaming")
	bool AreLayersReady(const TArray<TSoftObjectPtr<UDataLayerAsset>>& Layers) const;

	// 準備完了を待って Activate。TimeoutSeconds を過ぎたら
	// bSucceeded=false で通知（呼び出し側がフェードに逃げられるように）
	void ActivateLayersWhenReady(
		const TArray<TSoftObjectPtr<UDataLayerAsset>>& Layers,
		float TimeoutSeconds,
		const FLayerReadyDelegate& OnReady);

	// 現在の背景を隠して別の背景へ、をひとまとめに
	UFUNCTION(BlueprintCallable, Category = "Streaming")
	void SwapLayers(
		const TArray<TSoftObjectPtr<UDataLayerAsset>>& LayersToActivate,
		const TArray<TSoftObjectPtr<UDataLayerAsset>>& LayersToDeactivate);

protected:
	const UDataLayerInstance* FindInstance(const TSoftObjectPtr<UDataLayerAsset>& Layer) const;

	UDataLayerManager* GetDataLayerManager() const;

	void SetLayerState(const TArray<TSoftObjectPtr<UDataLayerAsset>>& Layers, EDataLayerRuntimeState State);

	void PollLayerReady();

	// ActivateLayersWhenReady の待機状態
	UPROPERTY(Transient)
	TArray<TSoftObjectPtr<UDataLayerAsset>> PendingLayers;

	FLayerReadyDelegate PendingCallback;
	float PendingElapsed = 0.f;
	float PendingTimeout = 0.f;
	FTimerHandle PollTimerHandle;
};


// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "BendsProgressSubsystem.generated.h"

/**
 * マップをまたいで残る進行状況（フラグ、チェックポイント）。
 * セーブの実装時は、この中身をSaveGameに書き出す。
 */
UCLASS()
class BENDSCORE_API UBendsProgressSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static UBendsProgressSubsystem* Get(const UObject* WorldContext);

	// フラグ
	void AddFlag(const FGameplayTag& Flag, int32 Delta = 1);
	void SetFlag(const FGameplayTag& Flag, int32 Value);
	int32 GetFlag(const FGameplayTag& Flag) const;
	bool HasFlag(const FGameplayTag& Flag) const { return GetFlag(Flag) != 0; }

	// チェックポイント
	void WriteCheckpoint(const FGameplayTag& Episode, const FGameplayTag& Phase);
	bool RequestResume(const FGameplayTag& Episode);
	bool ConsumeResume(const FGameplayTag& Episode, FGameplayTag& OutPhase);
	bool HasCheckpoint(const FGameplayTag& Episode) const;

	// 新しく始めるとき
	void ResetProgress();

	// セーブ用
	const TMap<FGameplayTag, int32>& GetAllFlags() const { return Flags; }

private:
	TMap<FGameplayTag, int32> Flags;
	TMap<FGameplayTag, int32> CheckpointFlags;
	FGameplayTag CheckpointEpisode;
	FGameplayTag CheckpointPhase;
	bool bResumePending = false;

};

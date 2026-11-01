// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Tickable.h"
#include "GameplayTagContainer.h"
#include "BendsEpisodeRule.generated.h"

class ABendsGameModeBase;
class UBendsProgressSubsystem;
class UBendsPhaseDriver;
class UBendsOperationClock;
class AController;
struct FBendsClockEvent;
struct FBendsClockSpeed;

DECLARE_MULTICAST_DELEGATE_TwoParams(FBendsPhaseChangedEvent, const FGameplayTag& /*OldPhase*/, const FGameplayTag& /*NewPhase*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FBendsEpisodeEndedEvent, bool /*bCompleted*/);

/** フェーズの登録内容。UPROPERTYはGCにドライバーを守らせるため */
USTRUCT()
struct FBendsPhaseEntry
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag Phase;

	UPROPERTY()
	TObjectPtr<UBendsPhaseDriver> Driver = nullptr;

	// このフェーズに入ったときにチェックポイントを書く
	UPROPERTY()
	bool bCheckpoint = false;
};

/**
 * 1つのEPの進行を管理するルールの基底。
 * フェーズの並び、遷移、チェックポイント、フラグの窓口を持つ。
 * 各フェーズの終わり方はドライバー（UBendsPhaseDriver）に任せ、
 * 派生クラスはBuildPhasesでフェーズとドライバーを登録する。
 */
UCLASS(Abstract)
class BENDSCORE_API UBendsEpisodeRule : public UObject, public FTickableGameObject
{
	GENERATED_BODY()

public:
	// GameModeから呼ぶ。InitializeはアクターのBeginPlayより前、BeginEpisodeは後
	virtual void Initialize(ABendsGameModeBase* InGameMode);
	virtual void BeginEpisode();
	virtual void Shutdown();

	// フェーズ
	void SetPhase(const FGameplayTag& NewPhase);
	bool AdvancePhase();
	void CompleteEpisode();

	const FGameplayTag& GetCurrentPhase() const { return CurrentPhase; }
	const FGameplayTag& GetEpisodeId() const { return EpisodeId; }
	float GetTimeInPhase() const { return TimeInPhase; }
	bool IsRunning() const { return bRunning; }

	UBendsPhaseDriver* GetCurrentDriver() const { return CurrentDriver; }

	template <typename T>
	T* GetCurrentDriver() const { return Cast<T>(CurrentDriver); }

	// アクターからの窓口。今のドライバーが対応していれば渡す
	bool NotifyObjective(const FGameplayTag& Objective);
	void ResolveHold(const FGameplayTag& EventTag);

	// 作戦の時計（CreateClockを呼んだルールだけが持つ）
	UBendsOperationClock* GetClock() const { return Clock; }

	// チェックポイント
	void SaveCheckpoint();
	void RestartFromCheckpoint();
	virtual void OnPlayerDied(AController* Controller);

	// フラグ
	void AddFlag(const FGameplayTag& Flag, int32 Delta = 1);
	int32 GetFlag(const FGameplayTag& Flag) const;

	// イベント
	FBendsPhaseChangedEvent& OnPhaseChanged() { return PhaseChangedEvent; }
	FBendsEpisodeEndedEvent& OnEpisodeEnded() { return EpisodeEndedEvent; }

	// ドライバーから呼ぶ
	void HandleDriverFinished(UBendsPhaseDriver* Driver, const FGameplayTag& NextPhase);
	void HandleDriverFailed(UBendsPhaseDriver* Driver);

	// UObject
	virtual UWorld* GetWorld() const override;

	// FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override { return bRunning; }
	virtual ETickableTickType GetTickableTickType() const override;
	virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }

protected:
	// 派生クラスでフェーズを登録する。Initializeの中で1回だけ呼ばれる
	virtual void BuildPhases() {}

	// フェーズを登録する。登録した順が基本の並び順になる
	template <typename T>
	T* AddPhase(const FGameplayTag& Phase, bool bCheckpoint = false)
	{
		T* Driver = NewObject<T>(this);
		AddPhaseDriver(Phase, Driver, bCheckpoint);
		return Driver;
	}

	void AddPhaseDriver(const FGameplayTag& Phase, UBendsPhaseDriver* Driver, bool bCheckpoint);

	UBendsOperationClock* CreateClock(const TArray<FBendsClockEvent>& Events, const TArray<FBendsClockSpeed>& Speeds);

	// フック
	virtual void OnEpisodeBegin() {}
	virtual void OnPhaseEntered(const FGameplayTag& Phase) {}
	virtual void OnPhaseExited(const FGameplayTag& Phase) {}
	virtual void OnPhaseTick(const FGameplayTag& Phase, float DeltaTime) {}
	virtual void OnPhaseFailed(const FGameplayTag& Phase);
	virtual void OnEpisodeEnd(bool bCompleted) {}

	ABendsGameModeBase* GetBendsGameMode() const { return GameMode.Get(); }
	UBendsProgressSubsystem* GetProgress() const;

	// 派生クラスのコンストラクタで設定する
	FGameplayTag EpisodeId;

	// OnPhaseTickを呼ぶか（経過時間とドライバーのTickは常に動く）
	bool bPhaseTickEnabled = false;

private:
	int32 FindPhaseIndex(const FGameplayTag& Phase) const;
	void EndCurrentDriver();

	UPROPERTY()
	TArray<FBendsPhaseEntry> Phases;

	UPROPERTY(Transient)
	TObjectPtr<UBendsPhaseDriver> CurrentDriver = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UBendsOperationClock> Clock = nullptr;

	FBendsPhaseChangedEvent PhaseChangedEvent;
	FBendsEpisodeEndedEvent EpisodeEndedEvent;

	TWeakObjectPtr<ABendsGameModeBase> GameMode;
	FGameplayTag CurrentPhase;
	FGameplayTag PendingPhase;
	float TimeInPhase = 0.f;
	bool bRunning = false;
	bool bInTransition = false;
};


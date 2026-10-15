// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "EncounterCoordinatorBase.generated.h"

class AWvAIController;

/**
 * シーン単位で1つ生成される、敵AI間の共有戦況コンテキスト。
 * StateTreeからはExternal Dataとしてこれを参照する。
 * BPで組む場合はUBlueprintableに変更可。
 */
UCLASS(Abstract)
class VELOURS_API UEncounterCoordinatorBase : public UObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(UWorld* InWorld);

	void RegisterEnemy(AWvAIController* Enemy);
	void UnregisterEnemy(AWvAIController* Enemy);

	void SetEstimatedPlayerLocation(const FVector& NewLocation) { EstimatedPlayerLocation = NewLocation; }
	FVector GetEstimatedPlayerLocation() const { return EstimatedPlayerLocation; }

	int32 GetActiveEnemyCount() const { return ActiveEnemies.Num(); }
	const TArray<TWeakObjectPtr<AWvAIController>>& GetActiveEnemies() const { return ActiveEnemies; }

protected:
	UPROPERTY()
	TArray<TWeakObjectPtr<AWvAIController>> ActiveEnemies;

	FVector EstimatedPlayerLocation = FVector::ZeroVector;

	TWeakObjectPtr<UWorld> World;
};


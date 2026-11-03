// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "Interaction/BendsInteractionTypes.h"
#include "BendsInteractionEffect.generated.h"

class UBendsEpisodeRule;

/**
 * インタラクトの結果の部品の基底。
 * インタラクト対象の詳細パネルで、複数を組み合わせて設定する。
 * 実行されるのは、手が触れた瞬間（Contactのイベント）か、アニメーションがない場合は即座。
 * 新しい結果が必要になったら、このクラスを継承してExecuteを書く。
 */
UCLASS(Abstract, EditInlineNew, DefaultToInstanced, CollapseCategories)
class BENDSINTERACTION_API UBendsInteractionEffect : public UObject
{
	GENERATED_BODY()

public:
	virtual void Execute(const FBendsInteractionContext& Context) {}

protected:
	static UBendsEpisodeRule* GetRule(const FBendsInteractionContext& Context);
};


/** ルールに目標を通知する（UBendsPhaseDriver_Objectivesで受ける） */
UCLASS(meta = (DisplayName = "Objective"))
class BENDSINTERACTION_API UBendsInteractionEffect_Objective : public UBendsInteractionEffect
{
	GENERATED_BODY()

public:
	virtual void Execute(const FBendsInteractionContext& Context) override;

	UPROPERTY(EditAnywhere, Category = "Effect", meta = (Categories = "Bends.Objective"))
	FGameplayTag Objective;
};

/** フラグを足す */
UCLASS(meta = (DisplayName = "Flag"))
class BENDSINTERACTION_API UBendsInteractionEffect_Flag : public UBendsInteractionEffect
{
	GENERATED_BODY()

public:
	virtual void Execute(const FBendsInteractionContext& Context) override;

	UPROPERTY(EditAnywhere, Category = "Effect", meta = (Categories = "Bends.Flag"))
	FGameplayTag Flag;

	UPROPERTY(EditAnywhere, Category = "Effect")
	int32 Delta = 1;
};

/** ルールに汎用のトリガーを送る（EPのルールのOnTriggerで受ける） */
UCLASS(meta = (DisplayName = "Trigger"))
class BENDSINTERACTION_API UBendsInteractionEffect_Trigger : public UBendsInteractionEffect
{
	GENERATED_BODY()

public:
	virtual void Execute(const FBendsInteractionContext& Context) override;

	UPROPERTY(EditAnywhere, Category = "Effect", meta = (Categories = "Bends.Trigger"))
	FGameplayTag Trigger;
};

/** 作戦の時計のホールドを解く */
UCLASS(meta = (DisplayName = "Resolve Hold"))
class BENDSINTERACTION_API UBendsInteractionEffect_ResolveHold : public UBendsInteractionEffect
{
	GENERATED_BODY()

public:
	virtual void Execute(const FBendsInteractionContext& Context) override;

	UPROPERTY(EditAnywhere, Category = "Effect", meta = (Categories = "Bends.Clock"))
	FGameplayTag HoldTag;
};




// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BendsGameModeBase.generated.h"

class UBendsEpisodeRule;

/**
 * 
 */
UCLASS()
class BENDSCORE_API ABendsGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	virtual void StartPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UBendsEpisodeRule* GetRule() const { return Rule; }

	template <typename T>
	T* GetRule() const { return Cast<T>(Rule); }

	// Ç«Ç±Ç©ÇÁÇ≈Ç‡ÉãÅ[ÉãÇéÊÇÈ
	template <typename T = UBendsEpisodeRule>
	static T* GetRule(const UObject* WorldContext)
	{
		const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
		const ABendsGameModeBase* GameMode = World ? World->GetAuthGameMode<ABendsGameModeBase>() : nullptr;
		return GameMode ? GameMode->GetRule<T>() : nullptr;
	}

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Bends")
	TSubclassOf<UBendsEpisodeRule> RuleClass;

private:
	UPROPERTY(Transient)
	TObjectPtr<UBendsEpisodeRule> Rule;
};

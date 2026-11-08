// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Logging/LogMacros.h"
#include "Character/VVisualOverrideProvider.h"
#include "RedemptionGameMode.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogWvGameMode, All, All)

UCLASS(minimalapi)
class ARedemptionGameMode : public AGameModeBase, public IVVisualOverrideProvider
{
	GENERATED_BODY()

public:
	ARedemptionGameMode();

	virtual void StartPlay() override;

	void EnableCustomLensFlare();

	void DisableCustomLensFlare();

	virtual const TArray<TSoftClassPtr<AActor>>& GetVisualOverrides() const override { return VisualOverrides; }

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "VisualOverride")
	TArray<TSoftClassPtr<AActor>> VisualOverrides;
};




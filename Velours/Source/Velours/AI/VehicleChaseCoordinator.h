// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AI/EncounterCoordinatorBase.h"
#include "VehicleChaseCoordinator.generated.h"

UENUM(BlueprintType)
enum class EChaseSequencePhase : uint8
{
	Driving,
	Stopping,
	WindowCracking,
	WindowBroken,
	Combat
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnChasePhaseChanged, EChaseSequencePhase /*NewPhase*/);


/**
 * 
 */
UCLASS()
class VELOURS_API UVehicleChaseCoordinator : public UEncounterCoordinatorBase
{
	GENERATED_BODY()
	
public:
	EChaseSequencePhase GetCurrentPhase() const { return CurrentPhase; }
	void AdvancePhase(EChaseSequencePhase NewPhase);

	void RegisterWindowHit();
	int32 GetWindowHitCount() const { return WindowHitCount; }

	FOnChasePhaseChanged OnPhaseChanged;

	UPROPERTY(EditAnywhere, Category = "VehicleChase")
	int32 WindowBreakThreshold = 3;

private:
	EChaseSequencePhase CurrentPhase = EChaseSequencePhase::Driving;
	int32 WindowHitCount = 0;
};


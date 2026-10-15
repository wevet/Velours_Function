// Copyright 2022 wevet works All Rights Reserved.


#include "AI/VehicleChaseCoordinator.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(VehicleChaseCoordinator)

void UVehicleChaseCoordinator::AdvancePhase(EChaseSequencePhase NewPhase)
{
	if (CurrentPhase == NewPhase)
	{
		return;
	}
	CurrentPhase = NewPhase;
	OnPhaseChanged.Broadcast(NewPhase);
}

void UVehicleChaseCoordinator::RegisterWindowHit()
{
	if (CurrentPhase != EChaseSequencePhase::WindowCracking)
	{
		return;
	}

	++WindowHitCount;
	if (WindowHitCount >= WindowBreakThreshold)
	{
		AdvancePhase(EChaseSequencePhase::WindowBroken);
	}
}


// Copyright 2022 wevet works All Rights Reserved.


#include "Component/WvCharacterMoverComponent.h"
#include "DefaultMovementSet/Modes/WalkingMode.h"
#include "DefaultMovementSet/Modes/FallingMode.h"
#include "DefaultMovementSet/Modes/FlyingMode.h"
#include "MoveLibrary/MovementMixer.h"
#include "MoverSimulationTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvCharacterMoverComponent)

const FName UWvCharacterMoverComponent::ModeName_Walking = TEXT("Walking");
const FName UWvCharacterMoverComponent::ModeName_Falling = TEXT("Falling");
const FName UWvCharacterMoverComponent::ModeName_Flying = TEXT("Flying");
const FName UWvCharacterMoverComponent::ModeName_Sliding = TEXT("Sliding");
const FName UWvCharacterMoverComponent::ModeName_Traversing = TEXT("Traversing");

UWvCharacterMoverComponent::UWvCharacterMoverComponent()
{
	MovementModes.Add(ModeName_Walking, CreateDefaultSubobject<UWalkingMode>(TEXT("WalkingMode")));
	MovementModes.Add(ModeName_Falling, CreateDefaultSubobject<UFallingMode>(TEXT("FallingMode")));
	MovementModes.Add(ModeName_Flying, CreateDefaultSubobject<UFlyingMode>(TEXT("FlyingMode")));
	MovementModes.Add(ModeName_Traversing, CreateDefaultSubobject<UFlyingMode>(TEXT("TraversingMode")));

	StartingMovementMode = ModeName_Falling;

	bool bHasDefaultSyncState = false;
	for (const FMoverDataPersistence& Persistence : PersistentSyncStateDataTypes)
	{
		if (Persistence.RequiredType == FMoverDefaultSyncState::StaticStruct())
		{
			bHasDefaultSyncState = true;
			break;
		}
	}
	if (!bHasDefaultSyncState)
	{
		PersistentSyncStateDataTypes.Add(FMoverDataPersistence(FMoverDefaultSyncState::StaticStruct(), true));
	}

	if (!MovementMixer)
	{
		MovementMixer = CreateDefaultSubobject<UMovementMixer>(TEXT("MovementMixer"));
	}
}


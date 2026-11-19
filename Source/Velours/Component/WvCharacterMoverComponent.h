// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DefaultMovementSet/CharacterMoverComponent.h"
#include "WvCharacterMoverComponent.generated.h"

/**
 * 
 */
UCLASS()
class VELOURS_API UWvCharacterMoverComponent : public UCharacterMoverComponent
{
	GENERATED_BODY()
	
public:
	UWvCharacterMoverComponent();

	static const FName ModeName_Walking;
	static const FName ModeName_Falling;
	static const FName ModeName_Flying;
	static const FName ModeName_Sliding;
	static const FName ModeName_Traversing;
};

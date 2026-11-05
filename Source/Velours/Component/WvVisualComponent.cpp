// Copyright 2022 wevet works All Rights Reserved.


#include "Component/WvVisualComponent.h"
#include "Character/BasePawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvVisualComponent)

UWvVisualComponent::UWvVisualComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWvVisualComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UWvVisualComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}


void UWvVisualComponent::RefreshVisual()
{
	if (ABasePawn* Pawn = Cast<ABasePawn>(GetOwner()))
	{
		Pawn->RefreshVisual();
	}
}

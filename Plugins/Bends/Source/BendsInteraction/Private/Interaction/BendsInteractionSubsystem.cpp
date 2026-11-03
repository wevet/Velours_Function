// Copyright 2022 wevet works All Rights Reserved.

#include "Interaction/BendsInteractionSubsystem.h"
#include "Interaction/BendsInteractableComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BendsInteractionSubsystem)

void UBendsInteractionSubsystem::Register(UBendsInteractableComponent* Interactable)
{
	if (Interactable)
	{
		Interactables.AddUnique(Interactable);
	}
}

void UBendsInteractionSubsystem::Unregister(UBendsInteractableComponent* Interactable)
{
	Interactables.RemoveAllSwap([Interactable](const TWeakObjectPtr<UBendsInteractableComponent>& Entry)
	{
		return !Entry.IsValid() || Entry.Get() == Interactable;
	});
}


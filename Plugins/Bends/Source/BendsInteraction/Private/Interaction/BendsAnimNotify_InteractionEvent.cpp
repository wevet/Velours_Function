// Copyright 2022 wevet works All Rights Reserved.

#include "Interaction/BendsAnimNotify_InteractionEvent.h"
#include "Interaction/BendsInteractorComponent.h"
#include "BendsInteractionTags.h"
#include "Components/SkeletalMeshComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BendsAnimNotify_InteractionEvent)

UBendsAnimNotify_InteractionEvent::UBendsAnimNotify_InteractionEvent()
{
	EventTag = BendsInteractionTags::Event_Contact;
}


void UBendsAnimNotify_InteractionEvent::Notify(
	USkeletalMeshComponent* MeshComp, 
	UAnimSequenceBase* Animation, 
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	// エディタのプレビューではASCがないので、何もしない
	if (MeshComp)
	{
		UBendsInteractorComponent::SendInteractionEvent(MeshComp->GetOwner(), EventTag);
	}
}

FString UBendsAnimNotify_InteractionEvent::GetNotifyName_Implementation() const
{
	return EventTag.IsValid() ? FString::Printf(TEXT("Interaction: %s"), *EventTag.GetTagName().ToString()) : TEXT("Interaction Event");
}

// Copyright 2022 wevet works All Rights Reserved.

#include "Dialogue/DialogueSpeakerRoster.h"
#include "Dialogue/DialogueSystemTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DialogueSpeakerRoster)

const UDialogueSpeakerAsset* UDialogueSpeakerRoster::FindSpeaker(const FGameplayTag& SpeakerId) const
{
	if (!SpeakerId.IsValid())
	{
		return nullptr;
	}
	const TObjectPtr<UDialogueSpeakerAsset>* Found = Speakers.Find(SpeakerId);
	return Found ? Found->Get() : nullptr;
}


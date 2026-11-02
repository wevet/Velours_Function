// Copyright 2022 wevet works All Rights Reserved.

#include "Dialogue/BendsDialogueSettings.h"
#include "Dialogue/DialogueSpeakerRoster.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BendsDialogueSettings)

UBendsDialogueSettings::UBendsDialogueSettings()
{
	CategoryName = TEXT("Plugins");
	SectionName = TEXT("Bends Dialogue");
}

const UDialogueSpeakerRoster* UBendsDialogueSettings::GetSpeakerRoster()
{
	UBendsDialogueSettings* Settings = GetMutableDefault<UBendsDialogueSettings>();
	if (!Settings->LoadedRoster && !Settings->SpeakerRoster.IsNull())
	{
		Settings->LoadedRoster = Settings->SpeakerRoster.LoadSynchronous();
	}
	return Settings->LoadedRoster;
}

#if WITH_EDITOR
void UBendsDialogueSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// 設定で対応表を差し替えたら、次の呼び出しで読み直す
	if (PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(UBendsDialogueSettings, SpeakerRoster))
	{
		LoadedRoster = nullptr;
	}
}
#endif


// Copyright 2022 wevet works All Rights Reserved.


#include "Episode/BendsPhaseDriver_Dialogue.h"
#include "Dialogue/DialogueSubsystem.h"
#include "Dialogue/DialogueSystemTypes.h"
#include "Dialogue/DialogueParticipantComponent.h"
#include "BendsDialogue.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BendsPhaseDriver_Dialogue)

void UBendsPhaseDriver_Dialogue::OnBegin()
{
	Super::OnBegin();
	bDialogueDone = false;

	UWorld* World = GetWorld();
	UDialogueSubsystem* Dialogue = World ? World->GetSubsystem<UDialogueSubsystem>() : nullptr;
	UDialogueScene* LoadedScene = Scene.LoadSynchronous();

	if (!Dialogue || !LoadedScene)
	{
		UE_LOG(LogBendsDialogue, Warning, TEXT("Dialogue driver: scene not found (%s). Skipping."), *Scene.ToString());
		Finish();
		return;
	}

	if (Dialogue->IsInDialogue())
	{
		UE_LOG(LogBendsDialogue, Warning, TEXT("Dialogue driver: another dialogue is playing. Skipping %s."), *GetNameSafe(LoadedScene));
		Finish();
		return;
	}

	TArray<AActor*> ParticipantActors;
	CollectParticipants(World, ParticipantActors);

	Subsystem = Dialogue;
	Dialogue->OnDialogueFinished.AddUniqueDynamic(this, &UBendsPhaseDriver_Dialogue::HandleDialogueFinished);
	Dialogue->StartDialogue(LoadedScene, ParticipantActors);

	// 開始を拒否された場合（シーンの不備など）は進める
	if (!bDialogueDone && !Dialogue->IsInDialogue())
	{
		UE_LOG(LogBendsDialogue, Warning, TEXT("Dialogue driver: failed to start %s. Skipping."), *GetNameSafe(LoadedScene));
		Unbind();
		Finish();
	}
}

void UBendsPhaseDriver_Dialogue::OnEnd()
{
	if (UDialogueSubsystem* Dialogue = Subsystem.Get())
	{
		// 会話の途中でフェーズが変わった場合だけ止める
		if (!bDialogueDone && Dialogue->IsInDialogue())
		{
			Unbind(); // 止めたときの終了通知で、もう一度Finishが走らないように先に外す
			Dialogue->StopDialogue();
		}
	}
	Unbind();

	Super::OnEnd();
}

void UBendsPhaseDriver_Dialogue::HandleDialogueFinished()
{
	bDialogueDone = true;
	Finish();
}


void UBendsPhaseDriver_Dialogue::CollectParticipants(const UWorld* World, TArray<AActor*>& OutActors) const
{
	for (const FGameplayTag& SpeakerId : Participants)
	{
		UDialogueParticipantComponent* Component = UDialogueParticipantComponent::FindBySpeakerId(World, SpeakerId);
		AActor* Owner = Component ? Component->GetOwner() : nullptr;
		if (!Owner)
		{
			UE_LOG(LogBendsDialogue, Warning, TEXT("Dialogue driver: participant %s not found."), *SpeakerId.ToString());
			continue;
		}
		OutActors.AddUnique(Owner);
	}
}

void UBendsPhaseDriver_Dialogue::Unbind()
{
	if (UDialogueSubsystem* Dialogue = Subsystem.Get())
	{
		Dialogue->OnDialogueFinished.RemoveDynamic(this, &UBendsPhaseDriver_Dialogue::HandleDialogueFinished);
	}
	Subsystem.Reset();
}


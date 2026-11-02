// Copyright 2022 wevet works All Rights Reserved.

#include "Dialogue/DialogueParticipantComponent.h"
#include "BendsDialogue.h"

#include "Dialogue/BendsDialogueSpeaker.h"
#include "Dialogue/BendsDialogueSettings.h"
#include "Dialogue/DialogueSpeakerRoster.h"

#include "UObject/UObjectIterator.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DialogueParticipantComponent)

UDialogueParticipantComponent::UDialogueParticipantComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDialogueParticipantComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UDialogueParticipantComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}


void UDialogueParticipantComponent::OnDialogueBegin_Implementation(AActor* OtherParticipant)
{
	if (bInDialogue)
	{
		UE_LOG(LogBendsDialogue, Warning, TEXT("already in dialogue => %s"), *GetNameSafe(GetOwner()));
		return;
	}

	bInDialogue = true;
	PartnerActor = OtherParticipant;

	// 会話モードへの遷移。相手を向く、武器を下ろす等は BP 側で。
	UE_LOG(LogBendsDialogue, Log, TEXT("[%s] begin with %s"), *GetNameSafe(GetOwner()), *GetNameSafe(OtherParticipant));
}

void UDialogueParticipantComponent::OnDialogueEnd_Implementation()
{
	if (!bInDialogue)
	{
		return;
	}

	// 中断経路でも必ずここを通るので、状態は漏れなく戻す
	bInDialogue = false;
	CurrentNode = nullptr;
	PartnerActor = nullptr;
	CurrentExpression = FGameplayTag::EmptyTag;
}

void UDialogueParticipantComponent::OnDialogueNodeEnter_Implementation(const UDialogueNode* Node)
{
	if (!IsValid(Node))
	{
		return;
	}

	CurrentNode = Node;

	// ノードに表情指定があれば適用。
	// シーケンス側からの細かい変化は別経路で上書きされる。
	if (Node->Expression.IsValid())
	{
		OnDialogueExpressionChanged(Node->Expression);
	}

	if (IsSpeakerFor(Node))
	{
		// 話者：口パク開始、ジェスチャー
	}
	else
	{
		// 聞き手：話者へ視線を向ける
	}
}

void UDialogueParticipantComponent::OnDialogueNodeExit_Implementation(const UDialogueNode* Node)
{
	// 話し終わり／聞き終わりの後始末。待機へ戻す。
	CurrentNode = nullptr;
}

void UDialogueParticipantComponent::OnDialogueExpressionChanged_Implementation(FGameplayTag Expression)
{
	if (CurrentExpression == Expression)
	{
		return;
	}

	CurrentExpression = Expression;

	// ここで直接メッシュを触らず、通知だけに留める。
	// 実際の適用は BP か、Chooser 経由のアニメ側で。
	ExpressionChangeDelegate.Broadcast(Expression);
}

void UDialogueParticipantComponent::OnDialogueChoiceMade_Implementation(const FDialogueChoice& Choice, bool bWasTimeout)
{
	// 次のノードに入る前の即応リアクション。
	// 沈黙は選択とは別の反応にする。
	if (bWasTimeout)
	{
		// 痺れを切らす、目を逸らす等
	}
	else
	{
		// 選択内容への反応
	}
}

bool UDialogueParticipantComponent::IsSpeakerFor(const UDialogueNode* Node) const
{
	const FGameplayTag Id = GetSpeakerId();
	if (!IsValid(Node) || !Id.IsValid())
	{
		return false;
	}
	return Node->SpeakerId.MatchesTagExact(Id);
}

USceneComponent* UDialogueParticipantComponent::GetVoiceAttachComponent() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}
	if (const IBendsDialogueSpeaker* Speaker = Cast<IBendsDialogueSpeaker>(Owner))
	{
		if (USceneComponent* Component = Speaker->GetDialogueVoiceComponent())
		{
			return Component;
		}
	}
	return Owner->GetRootComponent();
}

FGameplayTag UDialogueParticipantComponent::GetSpeakerId() const
{
	if (const IBendsDialogueSpeaker* Speaker = Cast<IBendsDialogueSpeaker>(GetOwner()))
	{
		return Speaker->GetDialogueSpeakerTag();
	}
	return FGameplayTag::EmptyTag;
}

const UDialogueSpeakerAsset* UDialogueParticipantComponent::GetSpeakerAsset() const
{
	if (SpeakerAsset)
	{
		return SpeakerAsset;
	}
	const UDialogueSpeakerRoster* Roster = UBendsDialogueSettings::GetSpeakerRoster();
	return Roster ? Roster->FindSpeaker(GetSpeakerId()) : nullptr;
}

FText UDialogueParticipantComponent::GetDisplayName() const
{
	const UDialogueSpeakerAsset* Asset = GetSpeakerAsset();
	return Asset ? Asset->DisplayName : FText::GetEmpty();
}

UDialogueParticipantComponent* UDialogueParticipantComponent::FindBySpeakerId(const UWorld* World, const FGameplayTag& SpeakerId)
{
	if (!World || !SpeakerId.IsValid())
	{
		return nullptr;
	}

	for (TObjectIterator<UDialogueParticipantComponent> It; It; ++It)
	{
		UDialogueParticipantComponent* Component = *It;
		if (!IsValid(Component) || Component->GetWorld() != World || !Component->IsRegistered())
		{
			continue;
		}
		if (Component->GetSpeakerId().MatchesTagExact(SpeakerId))
		{
			return Component;
		}
	}
	return nullptr;
}



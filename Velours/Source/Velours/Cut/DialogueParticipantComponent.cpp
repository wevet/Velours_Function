// Copyright 2022 wevet works All Rights Reserved.


#include "Cut/DialogueParticipantComponent.h"
#include "Velours.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DialogueParticipantComponent)

DEFINE_LOG_CATEGORY_STATIC(LogDialogue, Log, All)

// Sets default values for this component's properties
UDialogueParticipantComponent::UDialogueParticipantComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

}


// Called when the game starts
void UDialogueParticipantComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...

}


// Called every frame
void UDialogueParticipantComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}


void UDialogueParticipantComponent::OnDialogueBegin_Implementation(AActor* OtherParticipant)
{
	if (bInDialogue)
	{
		UE_LOG(LogDialogue, Warning, TEXT("already in dialogue => %s"), *GetNameSafe(GetOwner()));
		return;
	}

	bInDialogue = true;
	PartnerActor = OtherParticipant;

	// 会話モードへの遷移。相手を向く、武器を下ろす等は BP 側で。
	UE_LOG(LogDialogue, Log, TEXT("[%s] begin with %s"),
		*GetNameSafe(GetOwner()), *GetNameSafe(OtherParticipant));
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

FGameplayTag UDialogueParticipantComponent::GetSpeakerId() const
{
	return SpeakerAsset ? SpeakerAsset->SpeakerId : FGameplayTag::EmptyTag;
}

FText UDialogueParticipantComponent::GetDisplayName() const
{
	return SpeakerAsset ? SpeakerAsset->DisplayName : FText::GetEmpty();
}


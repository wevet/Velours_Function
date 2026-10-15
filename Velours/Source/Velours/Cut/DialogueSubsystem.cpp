// Copyright 2022 wevet works All Rights Reserved.

#include "Cut/DialogueSubsystem.h"
#include "Cut/DialogueParticipantComponent.h"
#include "Cut/DialogueChoiceWidget.h"
#include "Level/SceneStreamingSubsystem.h"
#include "Character/WvPlayerController.h"
#include "Character/BasePawn.h"
#include "Component/WvSkeletalMeshComponent.h"

#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "Kismet/GameplayStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DialogueSubsystem)

DEFINE_LOG_CATEGORY_STATIC(LogDialogue, Log, All)


#pragma region Entry
void UDialogueSubsystem::StartDialogue(UDialogueScene* Scene, const TArray<AActor*>& Participants)
{
	if (!IsValid(Scene) || !IsValid(Scene->RootNode))
	{
		UE_LOG(LogDialogue, Warning, TEXT("invalid scene or root node => %s"), *FString(__FUNCTION__));
		return;
	}

	if (IsInDialogue())
	{
		UE_LOG(LogDialogue, Warning, TEXT("already in dialogue => %s"), *FString(__FUNCTION__));
		return;
	}

	ParticipantComponents.Reset();
	History.Reset();
	AccumulatedTags.Reset();

	for (int32 Index = 0; Index < Participants.Num(); ++Index)
	{
		AActor* Actor = Participants[Index];
		if (!IsValid(Actor))
		{
			continue;
		}

		if (auto* Comp = Actor->FindComponentByClass<UDialogueParticipantComponent>())
		{
			ParticipantComponents.Add(Comp);
		}
	}

	// 相手を渡す。二人会話前提の簡易実装
	for (int32 Index = 0; Index < ParticipantComponents.Num(); ++Index)
	{
		AActor* Other = nullptr;
		for (int32 J = 0; J < ParticipantComponents.Num(); ++J)
		{
			if (J != Index && ParticipantComponents[J])
			{
				Other = ParticipantComponents[J]->GetOwner();
				break;
			}
		}
		ParticipantComponents[Index]->OnDialogueBegin(Other);
	}

	if (ChoiceWidget)
	{
		ChoiceWidget->OnChoiceSelected.AddUniqueDynamic(this, &ThisClass::HandleChoiceSelected);
	}

	EnterScene(Scene);
	UE_LOG(LogDialogue, Log, TEXT("EnterScene => %s"), *FString(__FUNCTION__));
}

void UDialogueSubsystem::StopDialogue()
{
	// シーンが残っていれば閉じる
	if (CurrentScene)
	{
		ExitScene();
	}

	StopPlayback();

	if (ChoiceWidget)
	{
		ChoiceWidget->HideChoices();
		ChoiceWidget->OnChoiceSelected.RemoveDynamic(this, &ThisClass::HandleChoiceSelected);
	}

	// 中断経路でも必ず通す
	BroadcastToParticipants([](UDialogueParticipantComponent* Comp)
		{
			Comp->OnDialogueEnd();
		});

	ParticipantComponents.Reset();
	ActiveChoices.Reset();
	History.Reset();
	CurrentNode = nullptr;
	CurrentScene = nullptr;
	bWaitingForChoice = false;

	OnDialogueFinished.Broadcast();
}
#pragma endregion


#pragma region Scene
void UDialogueSubsystem::EnterScene(UDialogueScene* Scene)
{
	if (!IsValid(Scene) || !IsValid(Scene->RootNode))
	{
		UE_LOG(LogDialogue, Warning, TEXT("invalid scene => %s"), *FString(__FUNCTION__));
		StopDialogue();
		return;
	}

	UDialogueScene* OldScene = CurrentScene;

	// 進行中のノードがあれば先に閉じる
	if (CurrentNode)
	{
		ExitNode();
	}

	if (OldScene && OldScene != Scene)
	{
		OnSceneFinished.Broadcast(OldScene);
	}

	CurrentScene = Scene;

	// 背景の切り替え。Preload 済みなら即座に済む
	ApplySceneStreaming(Scene, OldScene);

	OnSceneStarted.Broadcast(Scene);

	EnterNode(Scene->RootNode);
}


void UDialogueSubsystem::ExitScene()
{
	if (!CurrentScene)
	{
		return;
	}

	if (CurrentNode)
	{
		ExitNode();
	}

	UDialogueScene* Scene = CurrentScene;

	if (Scene->bUnloadOnExit)
	{
		if (USceneStreamingSubsystem* Streaming = GetStreamingSubsystem())
		{
			Streaming->UnloadLayers(Scene->ActivateLayers);
		}
	}

	CurrentScene = nullptr;
	OnSceneFinished.Broadcast(Scene);

	UE_LOG(LogDialogue, Log, TEXT("ExitScene => %s"), *FString(__FUNCTION__));
}
#pragma endregion


#pragma region Node
void UDialogueSubsystem::EnterNode(UDialogueNode* Node, bool bRecordHistory /*= true*/)
{
	if (!IsValid(Node))
	{
		// ノードが尽きた = 会話終了
		StopDialogue();
		return;
	}

	// 前のノードが閉じられていなければ閉じる
	if (CurrentNode && CurrentNode != Node)
	{
		ExitNode();
	}

	CurrentNode = Node;
	bWaitingForChoice = false;

	if (bRecordHistory)
	{
		FDialogueHistoryEntry Entry;
		Entry.Scene = CurrentScene;
		Entry.Node = Node;
		History.Add(Entry);
	}

	BroadcastToParticipants([Node](UDialogueParticipantComponent* Comp)
		{
			Comp->OnDialogueNodeEnter(Node);
		});

	// 到達しうる次シーンを先読みしておく
	PreloadReachableScenes(Node);

	// 字幕。話者名は Participant 側から引く
	FText DisplayName = FText::GetEmpty();
	for (int32 Index = 0; Index < ParticipantComponents.Num(); ++Index)
	{
		if (ParticipantComponents[Index]->IsSpeakerFor(Node))
		{
			DisplayName = ParticipantComponents[Index]->GetDisplayName();
			break;
		}
	}
	OnLineStarted.Broadcast(Node, DisplayName);

	// 再生。Shot / ボイス / 固定時間の三択
	if (Node->bUseShot && !Node->Shot.IsNull())
	{
		ULevelSequence* Sequence = Node->Shot.LoadSynchronous();

		ALevelSequenceActor* OutActor = nullptr;
		FMovieSceneSequencePlaybackSettings Settings;
		ShotPlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(GetWorld(), Sequence, Settings, OutActor);
		ShotActor = OutActor;

		if (ShotPlayer)
		{
			ShotPlayer->OnFinished.AddUniqueDynamic(this, &ThisClass::OnShotFinished);
			ShotPlayer->Play();
			return;
		}
	}

	// 話者のコンポーネントを引く
	UDialogueParticipantComponent* Speaker = nullptr;
	for (int32 Index = 0; Index < ParticipantComponents.Num(); ++Index)
	{
		if (ParticipantComponents[Index]->IsSpeakerFor(Node))
		{
			Speaker = ParticipantComponents[Index];
			break;
		}
	}

	float Duration = Node->FallbackDuration;

	if (!Node->Playback.Voice.IsNull())
	{
		USoundBase* Voice = Node->Playback.Voice.LoadSynchronous();
		if (Voice)
		{
			if (Speaker && Speaker->GetOwner())
			{
				// 話者に追従させる。会話中に動く可能性があるので Attached
				USceneComponent* AttachTo = Speaker->GetOwner()->GetRootComponent();
				if (const ABasePawn* Pawn = Cast<ABasePawn>(Speaker->GetOwner()))
				{
					if (USkeletalMeshComponent* Mesh = Pawn->GetWvSkeletalMeshComponent())
					{
						AttachTo = Mesh;
					}
				}

				UGameplayStatics::SpawnSoundAttached(
					Voice, 
					AttachTo,
					Node->Playback.VoiceSocket,
					FVector::ZeroVector, 
					EAttachLocation::SnapToTarget, 
					true);
			}
			else
			{
				// 話者不明（無線・ナレーション）は 2D
				UGameplayStatics::PlaySound2D(GetWorld(), Voice);
			}

			Duration = Voice->GetDuration();
		}
	}

	GetWorld()->GetTimerManager().SetTimer(
		PlaybackTimerHandle, 
		this,
		&ThisClass::OnNodePlaybackFinished, 
		Duration, false);
}


void UDialogueSubsystem::AdvanceToNode(UDialogueNode* Node)
{
	EnterNode(Node, true);
}


void UDialogueSubsystem::ExitNode()
{
	if (!CurrentNode)
	{
		return;
	}

	StopPlayback();

	UDialogueNode* Node = CurrentNode;

	BroadcastToParticipants([Node](UDialogueParticipantComponent* Comp)
		{
			Comp->OnDialogueNodeExit(Node);
		});

	// スキップ経路でも最終表情が確定するよう、ここで再適用
	if (Node->Expression.IsValid())
	{
		BroadcastToParticipants([Node](UDialogueParticipantComponent* Comp)
			{
				Comp->OnDialogueExpressionChanged(Node->Expression);
			});
	}

	CurrentNode = nullptr;
	bWaitingForChoice = false;
}


void UDialogueSubsystem::OnShotFinished()
{
	if (ShotPlayer)
	{
		ShotPlayer->OnFinished.RemoveDynamic(this, &ThisClass::OnShotFinished);
	}
	OnNodePlaybackFinished();
}


void UDialogueSubsystem::OnNodePlaybackFinished()
{
	if (!IsValid(CurrentNode))
	{
		return;
	}

	// ここではノードを閉じない。選択肢を出すまでが 1 ノード。
	// ExitNode は次へ遷移するときに呼ぶ。
	OnNodeFinished.Broadcast(CurrentNode);

	PresentChoices();
}


void UDialogueSubsystem::SkipCurrentNode()
{
	if (!CurrentNode || bWaitingForChoice)
	{
		return;
	}

	// Shot を終端へ飛ばす。途中の Notify は発火しない点に注意
	if (ShotPlayer)
	{
		ShotPlayer->OnFinished.RemoveDynamic(this, &ThisClass::OnShotFinished);
		ShotPlayer->GoToEndAndStop();
	}

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(PlaybackTimerHandle);
	}

	OnNodePlaybackFinished();
}
#pragma endregion


#pragma region Choices
void UDialogueSubsystem::PresentChoices()
{
	ActiveChoices.Reset();

	// RequiredTags で絞る。フィルタは必ずここで行い、Widget には渡さない
	for (int32 Index = 0; Index < CurrentNode->Choices.Num(); ++Index)
	{
		const FDialogueChoice& Choice = CurrentNode->Choices[Index];
		if (Choice.RequiredTags.IsEmpty() || AccumulatedTags.HasAll(Choice.RequiredTags))
		{
			ActiveChoices.Add(Choice);
		}
	}

	if (ActiveChoices.Num() <= 0)
	{
		// ExitNode で CurrentNode が null になるので先に退避する
		UDialogueNode* NextNode = CurrentNode->NextNode;
		UDialogueScene* NextScene = CurrentNode->NextScene;

		ExitNode();

		if (NextScene)
		{
			EnterScene(NextScene);
		}
		else if (NextNode)
		{
			AdvanceToNode(NextNode);
		}
		else
		{
			// 遷移先なし = ここで会話終了
			StopDialogue();
		}
		return;
	}


	bWaitingForChoice = true;

	if (ChoiceWidget)
	{
		ChoiceWidget->ShowChoices(CurrentNode, ActiveChoices);
	}

	if (CurrentNode->ChoiceTimeLimit > 0.f)
	{
		GetWorld()->GetTimerManager().SetTimer(ChoiceTimeoutHandle, this, &ThisClass::HandleTimeout,
			CurrentNode->ChoiceTimeLimit, false);
	}
}


void UDialogueSubsystem::HandleChoiceSelected(int32 ChoiceIndex)
{
	if (!ActiveChoices.IsValidIndex(ChoiceIndex))
	{
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(ChoiceTimeoutHandle);
	CommitChoice(ActiveChoices[ChoiceIndex], false);
}

void UDialogueSubsystem::SetChoiceWidget(UDialogueChoiceWidget* InWidget)
{
	ChoiceWidget = InWidget;
}


void UDialogueSubsystem::HandleTimeout()
{
	if (!IsValid(CurrentNode))
	{
		return;
	}

	if (ChoiceWidget)
	{
		ChoiceWidget->HideChoices();
	}

	UDialogueNode* Next = CurrentNode->TimeoutNode;

	// TimeoutNode 未設定なら先頭にフォールバック。
	// 沈黙は本来独立した反応にすべきなので、ここに来る時点で設定漏れ。
	if (!Next)
	{
		UE_LOG(LogDialogue, Warning, TEXT("TimeoutNode not set on %s"), *GetNameSafe(CurrentNode));
		if (ActiveChoices.Num() > 0)
		{
			CommitChoice(ActiveChoices[0], true);
		}
		return;
	}

	// 沈黙のタグを記録
	AccumulatedTags.AppendTags(CurrentNode->TimeoutGrantedTags);
	if (History.Num() > 0)
	{
		History.Last().TagsGrantedHere.AppendTags(CurrentNode->TimeoutGrantedTags);
	}

	BroadcastToParticipants([](UDialogueParticipantComponent* Comp)
		{
			Comp->OnDialogueChoiceMade(FDialogueChoice(), true);
		});

	ExitNode();
	AdvanceToNode(Next);
}


void UDialogueSubsystem::CommitChoice(const FDialogueChoice& Choice, bool bWasTimeout)
{
	AccumulatedTags.AppendTags(Choice.GrantedTags);

	// 巻き戻し用に、このステップで付けたタグを記録
	if (History.Num() > 0)
	{
		History.Last().TagsGrantedHere.AppendTags(Choice.GrantedTags);
	}

	// 次のノードに入る前の即応リアクション
	BroadcastToParticipants([&Choice, bWasTimeout](UDialogueParticipantComponent* Comp)
		{
			Comp->OnDialogueChoiceMade(Choice, bWasTimeout);
		});

	ExitNode();

	if (Choice.NextScene)
	{
		EnterScene(Choice.NextScene);
	}
	else if (Choice.NextNode)
	{
		AdvanceToNode(Choice.NextNode);
	}
	else
	{
		StopDialogue();
	}
}
#pragma endregion


#pragma region History
bool UDialogueSubsystem::GoBack()
{
	if (!CanGoBack())
	{
		return false;
	}

	// 現在のステップを捨てる
	History.Pop();

	FDialogueHistoryEntry& Prev = History.Last();

	// そのステップで付けたタグを取り消す
	AccumulatedTags.RemoveTags(Prev.TagsGrantedHere);
	Prev.TagsGrantedHere.Reset();

	ExitNode();

	// シーンが違えば背景も戻す
	if (Prev.Scene != CurrentScene)
	{
		UDialogueScene* OldScene = CurrentScene;
		CurrentScene = Prev.Scene;
		ApplySceneStreaming(CurrentScene, OldScene);
		OnSceneStarted.Broadcast(CurrentScene);
	}

	// 履歴には積み直さない
	EnterNode(Prev.Node, false);
	return true;
}
#pragma endregion


#pragma region Internal
void UDialogueSubsystem::StopPlayback()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(PlaybackTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(ChoiceTimeoutHandle);
	}

	if (ShotPlayer)
	{
		ShotPlayer->OnFinished.RemoveDynamic(this, &ThisClass::OnShotFinished);
		ShotPlayer->Stop();
	}

	ShotPlayer = nullptr;
	ShotActor = nullptr;
}


void UDialogueSubsystem::ApplySceneStreaming(UDialogueScene* NewScene, UDialogueScene* OldScene)
{
	USceneStreamingSubsystem* Streaming = GetStreamingSubsystem();
	if (!Streaming || !NewScene)
	{
		return;
	}

	if (NewScene->ActivateLayers.Num() <= 0)
	{
		return;
	}

	// 同座標に重ねる場合の事故を防ぐため Swap で行う
	const TArray<TSoftObjectPtr<UDataLayerAsset>> ToDeactivate =
		OldScene ? OldScene->ActivateLayers : TArray<TSoftObjectPtr<UDataLayerAsset>>();

	Streaming->SwapLayers(NewScene->ActivateLayers, ToDeactivate);
}


void UDialogueSubsystem::PreloadReachableScenes(const UDialogueNode* Node)
{
	USceneStreamingSubsystem* Streaming = GetStreamingSubsystem();
	if (!Streaming || !Node)
	{
		return;
	}

	for (int32 Index = 0; Index < Node->Choices.Num(); ++Index)
	{
		const UDialogueScene* Next = Node->Choices[Index].NextScene;
		if (Next)
		{
			Streaming->PreloadLayers(Next->PreloadLayers);
			Streaming->PreloadLayers(Next->ActivateLayers);
		}
	}
}


USceneStreamingSubsystem* UDialogueSubsystem::GetStreamingSubsystem() const
{
	UWorld* World = GetWorld();
	return World ? World->GetSubsystem<USceneStreamingSubsystem>() : nullptr;
}


void UDialogueSubsystem::BroadcastToParticipants(TFunctionRef<void(UDialogueParticipantComponent*)> Fn)
{
	for (int32 Index = 0; Index < ParticipantComponents.Num(); ++Index)
	{
		if (IsValid(ParticipantComponents[Index]))
		{
			Fn(ParticipantComponents[Index]);
		}
	}
}
#pragma endregion


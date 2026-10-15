// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayTagContainer.h"
#include "DialogueSystemTypes.h"
#include "DialogueSubsystem.generated.h"

class UDialogueParticipantComponent;
class UDialogueChoiceWidget;
class USceneStreamingSubsystem;
class ULevelSequencePlayer;
class ALevelSequenceActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDialogueLineDelegate, const UDialogueNode*, Node, FText, DisplayName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueNodeDelegate, const UDialogueNode*, Node);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueSceneDelegate, const UDialogueScene*, Scene);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDialogueFinishedDelegate);


/// <summary>
/// 巻き戻し用の履歴。1 ステップ = 1 ノード。
/// </summary>
USTRUCT()
struct FDialogueHistoryEntry
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UDialogueScene> Scene = nullptr;

	UPROPERTY()
	TObjectPtr<UDialogueNode> Node = nullptr;

	// このステップで付与したタグ。戻るときに取り消す
	UPROPERTY()
	FGameplayTagContainer TagsGrantedHere;
};


UCLASS()
class VELOURS_API UDialogueSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
#pragma region Entry
	// シーン単位で開始する。内部で RootNode へ入る
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void StartDialogue(UDialogueScene* Scene, const TArray<AActor*>& Participants);

	// 会話全体を終了する（中断含む）
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void StopDialogue();

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool IsInDialogue() const { return CurrentScene != nullptr; }
#pragma endregion


#pragma region Scene
	// 別シーンへ遷移する。背景の Swap を伴う
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Scene")
	void EnterScene(UDialogueScene* Scene);

	// 現在のシーンを終了する。会話自体は継続しうる
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Scene")
	void ExitScene();

	UFUNCTION(BlueprintPure, Category = "Dialogue|Scene")
	UDialogueScene* GetCurrentScene() const { return CurrentScene; }
#pragma endregion


#pragma region Node
	// 次のノードへ進める
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Node")
	void AdvanceToNode(UDialogueNode* Node);

	// 現在のノードを終える。シーン終了とは独立
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Node")
	void ExitNode();

	// 再生をスキップして選択肢へ飛ばす
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Node")
	void SkipCurrentNode();

	UFUNCTION(BlueprintPure, Category = "Dialogue|Node")
	UDialogueNode* GetCurrentNode() const { return CurrentNode; }
#pragma endregion


#pragma region History
	// 一つ前のノードへ戻す。付与済みタグも取り消す。
	// デバッグ・確認用。製品の巻き戻し機能として使うなら注意（下記コメント参照）
	UFUNCTION(BlueprintCallable, Category = "Dialogue|History")
	bool GoBack();

	UFUNCTION(BlueprintPure, Category = "Dialogue|History")
	bool CanGoBack() const { return History.Num() > 1; }
#pragma endregion


	UFUNCTION()
	void HandleChoiceSelected(int32 ChoiceIndex);

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void SetChoiceWidget(UDialogueChoiceWidget* InWidget);


#pragma region Delegates
	UPROPERTY(BlueprintAssignable)
	FDialogueSceneDelegate OnSceneStarted;

	UPROPERTY(BlueprintAssignable)
	FDialogueSceneDelegate OnSceneFinished;

	// 字幕表示用。ノードに入ってセリフが始まった
	UPROPERTY(BlueprintAssignable)
	FDialogueLineDelegate OnLineStarted;

	// ノードの再生が終わった（選択肢が出る直前）
	UPROPERTY(BlueprintAssignable)
	FDialogueNodeDelegate OnNodeFinished;

	UPROPERTY(BlueprintAssignable)
	FDialogueFinishedDelegate OnDialogueFinished;
#pragma endregion


protected:
	void EnterNode(UDialogueNode* Node, bool bRecordHistory = true);
	void OnNodePlaybackFinished();
	void PresentChoices();
	void CommitChoice(const FDialogueChoice& Choice, bool bWasTimeout);
	void HandleTimeout();

	void StopPlayback();
	void ApplySceneStreaming(UDialogueScene* NewScene, UDialogueScene* OldScene);
	void PreloadReachableScenes(const UDialogueNode* Node);

	USceneStreamingSubsystem* GetStreamingSubsystem() const;

	UFUNCTION()
	void OnShotFinished();

	void BroadcastToParticipants(TFunctionRef<void(UDialogueParticipantComponent*)> Fn);

	UPROPERTY(Transient)
	TObjectPtr<UDialogueScene> CurrentScene = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UDialogueNode> CurrentNode = nullptr;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UDialogueParticipantComponent>> ParticipantComponents;

	UPROPERTY(Transient)
	TObjectPtr<UDialogueChoiceWidget> ChoiceWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<ALevelSequenceActor> ShotActor = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<ULevelSequencePlayer> ShotPlayer = nullptr;

	UPROPERTY(Transient)
	FGameplayTagContainer AccumulatedTags;

	UPROPERTY(Transient)
	TArray<FDialogueChoice> ActiveChoices;

	UPROPERTY(Transient)
	TArray<FDialogueHistoryEntry> History;

	// ノードの再生が終わって選択肢待ちか
	UPROPERTY(Transient)
	bool bWaitingForChoice = false;

	FTimerHandle PlaybackTimerHandle;
	FTimerHandle ChoiceTimeoutHandle;
};


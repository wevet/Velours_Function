// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DialogueSystemTypes.h"
#include "Components/ActorComponent.h"
#include "DialogueParticipantComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueExpressionDelegate, FGameplayTag, Expression);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class VELOURS_API UDialogueParticipantComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UDialogueParticipantComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

public:

	UPROPERTY(EditDefaultsOnly, Category = "Dialogue")
	TObjectPtr<UDialogueSpeakerAsset> SpeakerAsset;

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	FGameplayTag GetSpeakerId() const;

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	FText GetDisplayName() const;

	// この会話に参加者として登録されたとき。会話全体で一度。
	// 相手を向く、武器を下ろす、待機モーションを会話用に切り替える、など
	// 「会話モードに入る」処理。
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dialogue")
	void OnDialogueBegin(AActor* OtherParticipant);

	// 会話が終了したとき。OnDialogueBegin で変えたものを戻す。
	// 中断（プレイヤーが離れた等）でも必ず呼ぶこと。
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dialogue")
	void OnDialogueEnd();

	// ノードに入った瞬間。セリフ再生の直前。
	// このノードで喋るのが自分かどうかは Node->SpeakerId と SpeakerId を比較して判断。
	// 喋る側：口パク開始、ジェスチャーモンタージュ再生
	// 聞く側：話者のほうを向く、相槌
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dialogue")
	void OnDialogueNodeEnter(const UDialogueNode* Node);

	// ノードのセリフ・Shot が完了したとき。選択肢 UI が出る直前。
	// 喋り終わった側は待機に戻る。聞く側はプレイヤーの反応を待つ姿勢に入る。
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dialogue")
	void OnDialogueNodeExit(const UDialogueNode* Node);

	// 表情タグが変わったとき。ノード遷移とは独立に、
	// 会話中の任意のタイミングで呼べるようにしておく。
	// （セリフの途中で表情が動くのが「微細な変化」の本体なので）
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dialogue")
	void OnDialogueExpressionChanged(FGameplayTag Expression);

	// プレイヤーが選択肢を選んだ直後、次のノードに入る前。
	// 尋問者が選択内容に反応する（眉を上げる、目を逸らす）ための即応フック。
	// タイムアウトの場合も、それと分かる形でここを通す。
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dialogue")
	void OnDialogueChoiceMade(const FDialogueChoice& Choice, bool bWasTimeout);

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	bool IsSpeakerFor(const UDialogueNode* Node) const;

	UPROPERTY(BlueprintAssignable)
	FDialogueExpressionDelegate ExpressionChangeDelegate;


protected:
	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	TObjectPtr<const UDialogueNode> CurrentNode = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	bool bInDialogue = false;

	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	TObjectPtr<AActor> PartnerActor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	FGameplayTag CurrentExpression;

};

// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Episode/Driver/BendsPhaseDriver.h"
#include "BendsPhaseDriver_Dialogue.generated.h"

class UDialogueScene;
class UDialogueSubsystem;

/**
 * 会話シーンを再生し、会話の終了で完了するドライバー。
 * プロローグの着信、EP1の前日のテキストやブリーフィング、EP2の尋問などに使う。
 *
 * フェーズに入ると、SceneをUDialogueSubsystemで再生する。
 * 参加者は、ParticipantTagsのアクタータグでレベルから探す。
 * 会話が終わる（OnDialogueFinished）と完了する。
 * 会話の途中でフェーズが切り替わった場合は、会話を止める。
 *
 * 次の場合は、警告を出してすぐ完了する（試作中に会話のないフェーズを飛ばせるように）:
 *   ・Sceneが未設定か、読み込めない
 *   ・別の会話が再生中などで、会話を開始できなかった
 *
 * 会話の選択でフラグを立てる処理は会話システム側（タグの付与）が持ち、
 * このドライバーは「終わったら次へ」だけを担当する。
 *
 * 例:
 *   UBendsPhaseDriver_Dialogue* Briefing = AddPhase<UBendsPhaseDriver_Dialogue>(Phase_EP1_Briefing);
 *   Briefing->Scene = TSoftObjectPtr<UDialogueScene>(FSoftObjectPath(TEXT("/Game/Bends/EP1/Dialogue/DS_EP1_Briefing.DS_EP1_Briefing")));
 *   Briefing->ParticipantTags = { TEXT("Ash"), TEXT("Marlow") };
 */
UCLASS()
class BENDSDIALOGUE_API UBendsPhaseDriver_Dialogue : public UBendsPhaseDriver
{
	GENERATED_BODY()

public:
	TSoftObjectPtr<UDialogueScene> Scene;
	TArray<FName> ParticipantTags;

protected:
	virtual void OnBegin() override;
	virtual void OnEnd() override;

private:
	// UDialogueSubsystemのデリゲートがダイナミックなので、受け口だけUFUNCTIONが必要
	UFUNCTION()
	void HandleDialogueFinished();

	void Unbind();

	TWeakObjectPtr<UDialogueSubsystem> Subsystem;
	bool bDialogueDone = false;
};


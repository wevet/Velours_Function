// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "BendsDialogueSpeaker.generated.h"

class USceneComponent;

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UBendsDialogueSpeaker : public UInterface
{
	GENERATED_BODY()
};

/**

 * 会話の話者になるアクターが実装するインターフェース。
 * 会話システムがゲーム側のクラスを知らずに、声の発生位置などを取れるようにする。
 * 実装しないアクターは、UDialogueParticipantComponentの既定（ルートコンポーネント）になる。
 */
class BENDSDIALOGUE_API IBendsDialogueSpeaker
{
	GENERATED_BODY()

public:
	// 話者を表すタグ（キャラクター名）。
	// 会話のノードのSpeakerIdと、ドライバーの参加者の指定に使う。
	// 無効なタグを返すと、SpeakerAssetのSpeakerIdを使う
	virtual FGameplayTag GetDialogueSpeakerTag() const { return FGameplayTag(); }

	virtual USceneComponent* GetDialogueVoiceComponent() const { return nullptr; }
};


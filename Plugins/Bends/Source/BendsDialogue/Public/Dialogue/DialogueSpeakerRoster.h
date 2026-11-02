// Copyright 2022 wevet works All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "DialogueSpeakerRoster.generated.h"

class UDialogueSpeakerAsset;

/**
 * 話者の名前のタグ（Character.Name.*）から、話者の表示用データを引く対応表。
 * 1プロジェクトに1つ作り、プロジェクト設定（Plugins > Bends Dialogue）で指定する。
 *
 * キャラクター名のタグは、AVBaseHuman::HumanNameTagと会話ノードのSpeakerIdの2か所で使い、
 * 表示名や字幕の色はこの対応表だけで管理する。
 * 画面にアクターがいない話者（留守電、無線、電話、動画の声）も、ここに登録すれば字幕に名前が出る。
 */
UCLASS(BlueprintType)
class BENDSDIALOGUE_API UDialogueSpeakerRoster : public UDataAsset
{
	GENERATED_BODY()

public:
	const UDialogueSpeakerAsset* FindSpeaker(const FGameplayTag& SpeakerId) const;

	UPROPERTY(EditAnywhere, Category = "Speaker", meta = (Categories = "Character.Name", ForceInlineRow))
	TMap<FGameplayTag, TObjectPtr<UDialogueSpeakerAsset>> Speakers;
};


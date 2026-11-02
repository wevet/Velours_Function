// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "BendsDialogueSettings.generated.h"

class UDialogueSpeakerRoster;

/**
 * 会話システムのプロジェクト設定（Project Settings > Plugins > Bends Dialogue）。
 * DefaultGame.iniに保存される。
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Bends Dialogue"))
class BENDSDIALOGUE_API UBendsDialogueSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UBendsDialogueSettings();

	// 話者の対応表。初回の呼び出しで読み込み、以降は保持する
	static const UDialogueSpeakerRoster* GetSpeakerRoster();

	UPROPERTY(Config, EditAnywhere, Category = "Speaker")
	TSoftObjectPtr<UDialogueSpeakerRoster> SpeakerRoster;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	// 読み込んだ対応表がGCされないように保持する
	UPROPERTY(Transient)
	TObjectPtr<UDialogueSpeakerRoster> LoadedRoster;
};


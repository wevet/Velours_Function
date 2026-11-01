// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
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
	// 声を追従させるコンポーネント（通常は体のメッシュ）。nullptrなら既定を使う
	virtual USceneComponent* GetDialogueVoiceComponent() const { return nullptr; }
};


// Copyright 2022 wevet works All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

namespace BendsInteractionTags
{
	// インタラクト対象のID。1回きりの対象の使用済みの記録にも使う
	BENDSINTERACTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Id);

	// アビリティ（Montage）からインタラクターへ送り返すイベント
	BENDSINTERACTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Contact);    // 手が触れた瞬間。ここで結果を出す
	BENDSINTERACTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Finished);   // 動作の終わり。操作の固定を解く
	BENDSINTERACTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Cancelled);  // 中断。結果を出さない

	// インタラクトを禁止する理由
	BENDSINTERACTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Block);
	BENDSINTERACTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Block_Dialogue);
	BENDSINTERACTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Block_Cutscene);
	BENDSINTERACTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Block_HandsFull);

	BENDSINTERACTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Anim);
}


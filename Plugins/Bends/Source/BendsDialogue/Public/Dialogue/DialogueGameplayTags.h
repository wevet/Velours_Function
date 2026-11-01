// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

// ---- 話者 ----
// 主要人物
BENDSDIALOGUE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Speaker_Protagonist);   // 主人公・女性警官
BENDSDIALOGUE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Speaker_Partner);       // 相棒・男性警官（真犯人）

// 警察側
BENDSDIALOGUE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Speaker_Police_Chief);      // 上司
BENDSDIALOGUE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Speaker_Police_OfficerA);
BENDSDIALOGUE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Speaker_Police_OfficerB);
BENDSDIALOGUE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Speaker_Police_OfficerC);

// 捜査側
BENDSDIALOGUE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Speaker_Investigator_LeadA);
BENDSDIALOGUE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Speaker_Investigator_LeadB);
BENDSDIALOGUE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Speaker_Investigator_Female);

// 敵対勢力
BENDSDIALOGUE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Speaker_Hostile_Masked);       // 覆面（個体を区別しない）
BENDSDIALOGUE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Speaker_Hostile_LieutenantA);  // 黒幕側・顔あり
BENDSDIALOGUE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Speaker_Hostile_LieutenantB);

// その他
BENDSDIALOGUE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Speaker_Radio);        // 無線越しの声
BENDSDIALOGUE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Speaker_Narration);    // 独白・ナレーション

// dialog ui
BENDSDIALOGUE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_State_Dialogue);
BENDSDIALOGUE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Input_Choice_Dialogue_Up);
BENDSDIALOGUE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Input_Choice_Dialogue_Down);
BENDSDIALOGUE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Input_Choice_Dialogue_Left);
BENDSDIALOGUE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Input_Choice_Dialogue_Right);
BENDSDIALOGUE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Input_Choice_Dialogue_Confirm);
BENDSDIALOGUE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Input_Choice_Dialogue_Skip);

// ---- 表情 ----


// ---- 会話フラグ（GrantedTags / RequiredTags 用）----
// 捜査で得た情報


// 関係性

// 尋問での振る舞い



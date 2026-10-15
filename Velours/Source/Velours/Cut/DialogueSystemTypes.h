// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LevelSequence.h"
#include "GameplayTagContainer.h"
#include "DialogueSystemTypes.generated.h"

class USoundBase;
class UAnimSequence;
class UDialogueNode;
class UDialogueScene;
class UDataLayerAsset;

UENUM(BlueprintType)
enum class EDialogueChoiceSlot : uint8
{
	Up = 0,
	Down = 1,
	Left = 2,
	Right = 3,
	Max,
};

/**
*
* UDialogueScene   ─ 場面。背景、開始ノード
*	└ UDialogueNode ─ セリフ、話者、Shot、選択肢
*		└ FDialogueChoice ─ 選択肢。次ノード or 次シーン
* UDialogueSpeakerAsset ─ 話者マスター
*
*/


/// <summary>
/// Shot を使わないノードの再生データ
/// </summary>
USTRUCT(BlueprintType)
struct VELOURS_API FDialoguePlayback
{
	GENERATED_BODY()

	// 話者位置から 3D 再生するボイス
	UPROPERTY(EditAnywhere, Category = "Playback")
	TSoftObjectPtr<USoundBase> Voice;

	// ボイスに合わせて再生する表情・ジェスチャー。
	// null ならノード遷移まで直前のアニメを保持する
	UPROPERTY(EditAnywhere, Category = "Playback")
	TSoftObjectPtr<UAnimSequence> FacialAnim;

	UPROPERTY(EditAnywhere, Category = "Playback")
	TSoftObjectPtr<UAnimSequence> BodyAnim;

	// ボイスが無い場合の表示時間
	UPROPERTY(EditAnywhere, Category = "Playback", meta = (ClampMin = "0.0"))
	float Duration = 2.f;

	// 話者の口元ソケット。空なら Owner の位置から鳴らす
	UPROPERTY(EditAnywhere, Category = "Playback")
	FName VoiceSocket = TEXT("head");
};


/// <summary>
/// ノードに紐づく効果音・BGM
/// </summary>
USTRUCT(BlueprintType)
struct VELOURS_API FDialogueAudioCue
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Audio")
	TSoftObjectPtr<USoundBase> Sound;

	// true なら話者位置から 3D、false なら 2D
	UPROPERTY(EditAnywhere, Category = "Audio")
	bool bSpatialized = false;

	// 2D でない場合の発音位置を持つアクタータグ。
	// 空なら話者の位置
	UPROPERTY(EditAnywhere, Category = "Audio", meta = (EditCondition = "bSpatialized"))
	FGameplayTag SourceSpeakerId;
};



/// <summary>
/// 選択肢データ
/// </summary>
USTRUCT(BlueprintType)
struct VELOURS_API FDialogueChoice
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FText Text;

	// この選択肢を出す条件（フラグ）
	UPROPERTY(EditAnywhere)
	FGameplayTagContainer RequiredTags;

	// 選択で立てるフラグ。終盤の分岐はこれを見る
	UPROPERTY(EditAnywhere)
	FGameplayTagContainer GrantedTags;

	// 次のノード。null なら会話終了
	UPROPERTY(EditAnywhere)
	TObjectPtr<UDialogueNode> NextNode;

	// NextNode の代わりに別シーンへ移る場合
	UPROPERTY(EditAnywhere)
	TObjectPtr<UDialogueScene> NextScene;
};


/// <summary>
/// 実際の会話データ
/// </summary>
UCLASS(BlueprintType)
class VELOURS_API UDialogueNode : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Dialogue|Text")
	FText SpeakerLine;

	// 話者・表情などの演出フック
	UPROPERTY(EditAnywhere, Category = "Dialogue|Text")
	FGameplayTag SpeakerId;

	UPROPERTY(EditAnywhere, Category = "Dialogue|Playback")
	bool bUseShot = true;

	// このノードで再生する oneshot シーケンス
	UPROPERTY(EditAnywhere, Category = "Dialogue|Playback", meta = (EditCondition = "bUseShot"))
	TSoftObjectPtr<ULevelSequence> Shot;

	// Shot を使わない場合の再生データ
	UPROPERTY(EditAnywhere, Category = "Dialogue|Playback", meta = (EditCondition = "!bUseShot"))
	FDialoguePlayback Playback;

	// このノードで鳴らす SE / BGM。Shot の有無に関わらず有効
	UPROPERTY(EditAnywhere, Category = "Dialogue|Audio")
	TArray<FDialogueAudioCue> AudioCues;

	// BGM の切り替え。null なら変更しない
	UPROPERTY(EditAnywhere, Category = "Dialogue|Audio")
	TSoftObjectPtr<USoundBase> BGM;

	// ボイスも無い場合の表示時間
	UPROPERTY(EditAnywhere, Category = "Dialogue|Playback")
	float FallbackDuration = 2.f;

	UPROPERTY(EditAnywhere, Category = "Dialogue|Playback")
	FGameplayTag Expression;

	UPROPERTY(EditAnywhere, Category = "Dialogue|Choices")
	TArray<FDialogueChoice> Choices;

	// Choices が空のときの遷移先。両方 null なら会話終了
	UPROPERTY(EditAnywhere, Category = "Dialogue|Choices")
	TObjectPtr<UDialogueNode> NextNode;

	// NextNode より優先。別シーンへ移る場合
	UPROPERTY(EditAnywhere, Category = "Dialogue|Choices")
	TObjectPtr<UDialogueScene> NextScene;

	// 確定に必要な長押し時間。0 以下なら Widget 側の既定値を使う
	UPROPERTY(EditAnywhere, Category = "Dialogue|Choices", meta = (ClampMin = "0.0"))
	float ChoiceHoldTime = 0.f;

	// UDialogueNode に追加
	// 0 なら無制限
	UPROPERTY(EditAnywhere, Category = "Dialogue|Choices", meta = (ClampMin = "0.0"))
	float ChoiceTimeLimit = 0.f;

	// タイムアウト時の遷移先。null なら Choices[0] にフォールバック
	UPROPERTY(EditAnywhere, Category = "Dialogue|Choices", meta = (EditCondition = "ChoiceTimeLimit > 0"))
	TObjectPtr<UDialogueNode> TimeoutNode;

	// タイムアウトで立てるフラグ（沈黙したことを後で参照する用）
	UPROPERTY(EditAnywhere, Category = "Dialogue|Choices", meta = (EditCondition = "ChoiceTimeLimit > 0"))
	FGameplayTagContainer TimeoutGrantedTags;
};


/// <summary>
/// 会話システムのWorldPartition
/// </summary>
UCLASS(BlueprintType)
class VELOURS_API UDialogueScene : public UDataAsset
{
	GENERATED_BODY()
public:
	// このシーンの開始ノード
	UPROPERTY(EditAnywhere, Category = "Scene")
	TObjectPtr<UDialogueNode> RootNode;

	// このシーンで表示する背景
	UPROPERTY(EditAnywhere, Category = "Scene|Streaming")
	TArray<TSoftObjectPtr<UDataLayerAsset>> ActivateLayers;

	// 入る前に読み込んでおきたいもの（次シーンの先読み用）
	UPROPERTY(EditAnywhere, Category = "Scene|Streaming")
	TArray<TSoftObjectPtr<UDataLayerAsset>> PreloadLayers;

	// シーンを抜けたら Unload するか、Loaded で保持するか
	UPROPERTY(EditAnywhere, Category = "Scene|Streaming")
	bool bUnloadOnExit = true;
};


/// <summary>
/// 話者のDataAsset
/// </summary>
UCLASS(BlueprintType)
class VELOURS_API UDialogueSpeakerAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	FGameplayTag SpeakerId;

	// 字幕に出す表示名
	UPROPERTY(EditAnywhere)
	FText DisplayName;

	// 字幕の色分けに使う
	UPROPERTY(EditAnywhere)
	FLinearColor SubtitleColor = FLinearColor::White;

	// この話者が使う表情セット（Chooser でもよい）
	UPROPERTY(EditAnywhere)
	TMap<FGameplayTag, TSoftObjectPtr<UAnimSequence>> Expressions;
};


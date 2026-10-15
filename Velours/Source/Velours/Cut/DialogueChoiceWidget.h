// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "DialogueChoiceWidget.generated.h"

class UDialogueNode;
class APawn;
struct FDialogueChoice;
class UDialogueChoiceItemWidget;
class UBorder;
class UWidgetAnimation;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueChoiceSelectedDelegate, int32, ChoiceIndex);


/**
 *
 */
UCLASS()
class VELOURS_API UDialogueChoiceWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	void Initializer(APawn* NewPlayerCharacter);

protected:
	virtual void NativeConstruct() override;

public:
	// 表示開始。フィルタ済みの選択肢を受け取る
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void ShowChoices(const UDialogueNode* Node, const TArray<FDialogueChoice>& InChoices);

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void HideChoices();

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void HighlightSlot(const EDialogueChoiceSlot InSlot);

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void BeginConfirmHold();

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void EndConfirmHold();

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool IsActive() const { return bActive; }

	UPROPERTY(BlueprintAssignable, Category = "Dialogue")
	FDialogueChoiceSelectedDelegate OnChoiceSelected;

protected:
	void BuildSlotMapping();
	void RefreshItems();
	void ApplyHighlight();
	void CommitIndex(const int32 ChoiceIndex);

	UDialogueChoiceItemWidget* GetItemForSlot(const EDialogueChoiceSlot InSlot) const;
	int32 GetChoiceIndexForSlot(const EDialogueChoiceSlot InSlot) const;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UDialogueChoiceItemWidget> ItemUp;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UDialogueChoiceItemWidget> ItemDown;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UDialogueChoiceItemWidget> ItemLeft;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UDialogueChoiceItemWidget> ItemRight;

	// 中央マーカー。選択肢表示中のみ出す
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UBorder> CenterMarker;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<class UWidgetAnimation> FadeInAnim;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<class UWidgetAnimation> FadeOutAnim;

	UFUNCTION()
	void OnFadeOutFinished();

	FWidgetAnimationDynamicEvent FadeOutFinishedDelegate;

	// 確定に必要な長押し時間の既定値。ノード側が 0 以下ならこれを使う
	UPROPERTY(EditDefaultsOnly, Category = "Dialogue", meta = (ClampMin = "0.0"))
	float DefaultHoldTime = 0.6f;

	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	TArray<FDialogueChoice> Choices;

	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	int32 HighlightIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	bool bActive = false;

	// スロット → Choices の index
	UPROPERTY(Transient)
	TArray<int32> SlotToChoice;

	UPROPERTY(Transient)
	int32 HoldingIndex = INDEX_NONE;

	UPROPERTY(Transient)
	float HoldElapsed = 0.f;

	UPROPERTY(Transient)
	float HoldRequired = 0.f;

	TWeakObjectPtr<APawn> PlayerCharacter;
};


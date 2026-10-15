// Copyright 2022 wevet works All Rights Reserved.

#include "Cut/DialogueChoiceWidget.h"
#include "Cut/DialogueChoiceItemWidget.h"
#include "Components/Border.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DialogueChoiceWidget)


void UDialogueChoiceWidget::NativeConstruct()
{
	Super::NativeConstruct();

	FadeOutFinishedDelegate.BindDynamic(this, &ThisClass::OnFadeOutFinished);
}


void UDialogueChoiceWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bActive || HoldingIndex == INDEX_NONE || HoldRequired <= 0.f)
	{
		return;
	}

	HoldElapsed += InDeltaTime;
	const float Progress = FMath::Clamp(HoldElapsed / HoldRequired, 0.f, 1.f);

	for (int32 SlotIndex = 0; SlotIndex < (int32)EDialogueChoiceSlot::Max; ++SlotIndex)
	{
		UDialogueChoiceItemWidget* Item = GetItemForSlot((EDialogueChoiceSlot)SlotIndex);
		if (Item && Item->GetChoiceIndex() == HoldingIndex)
		{
			Item->SetHoldProgress(Progress);
			break;
		}
	}

	if (Progress >= 1.f)
	{
		const int32 Committed = HoldingIndex;
		HoldingIndex = INDEX_NONE;
		HoldElapsed = 0.f;
		CommitIndex(Committed);
	}
}

void UDialogueChoiceWidget::Initializer(APawn* NewPlayerCharacter)
{
	PlayerCharacter = NewPlayerCharacter;
}

void UDialogueChoiceWidget::ShowChoices(const UDialogueNode* Node, const TArray<FDialogueChoice>& InChoices)
{
	Choices = InChoices;
	bActive = (Choices.Num() > 0);

	if (!bActive)
	{
		HideChoices();
		return;
	}

	HoldRequired = DefaultHoldTime;
	if (IsValid(Node) && Node->ChoiceHoldTime > 0.f)
	{
		HoldRequired = Node->ChoiceHoldTime;
	}

	// 最初は無選択。誤って決定キーを押しても何も起きない
	HighlightIndex = INDEX_NONE;
	HoldingIndex = INDEX_NONE;
	HoldElapsed = 0.f;

	BuildSlotMapping();
	RefreshItems();

	if (CenterMarker)
	{
		CenterMarker->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);

	// フェードアウト中だった場合に備えて止める
	if (FadeOutAnim)
	{
		UnbindAllFromAnimationFinished(FadeOutAnim);
		StopAnimation(FadeOutAnim);
	}

	if (FadeInAnim)
	{
		PlayAnimation(FadeInAnim);
	}
	else
	{
		SetRenderOpacity(1.f);
	}
}

void UDialogueChoiceWidget::HideChoices()
{
	bActive = false;
	HighlightIndex = INDEX_NONE;
	HoldingIndex = INDEX_NONE;
	HoldElapsed = 0.f;

	if (FadeInAnim)
	{
		StopAnimation(FadeInAnim);
	}

	if (FadeOutAnim)
	{
		BindToAnimationFinished(FadeOutAnim, FadeOutFinishedDelegate);
		PlayAnimation(FadeOutAnim);
	}
	else
	{
		OnFadeOutFinished();
	}

}

void UDialogueChoiceWidget::OnFadeOutFinished()
{
	if (FadeOutAnim)
	{
		UnbindAllFromAnimationFinished(FadeOutAnim);
	}

	Choices.Reset();
	SlotToChoice.Reset();

	for (int32 SlotIndex = 0; SlotIndex < (int32)EDialogueChoiceSlot::Max; ++SlotIndex)
	{
		if (UDialogueChoiceItemWidget* Item = GetItemForSlot((EDialogueChoiceSlot)SlotIndex))
		{
			Item->ClearItem();
		}
	}

	if (CenterMarker)
	{
		CenterMarker->SetVisibility(ESlateVisibility::Hidden);
	}

	SetVisibility(ESlateVisibility::Hidden);
}

void UDialogueChoiceWidget::BuildSlotMapping()
{
	SlotToChoice.Init(INDEX_NONE, (int32)EDialogueChoiceSlot::Max);

	const int32 Count = FMath::Min(Choices.Num(), (int32)EDialogueChoiceSlot::Max);

	// 1択: 下 / 2択: 上下 / 3択: triangle / 4択: cross
	static const EDialogueChoiceSlot Layout1[] = {
		EDialogueChoiceSlot::Down };

	static const EDialogueChoiceSlot Layout2[] = {
		EDialogueChoiceSlot::Up, EDialogueChoiceSlot::Down };

	static const EDialogueChoiceSlot Layout3[] = {
		EDialogueChoiceSlot::Up, EDialogueChoiceSlot::Left, EDialogueChoiceSlot::Right };

	static const EDialogueChoiceSlot Layout4[] = {
		EDialogueChoiceSlot::Up, EDialogueChoiceSlot::Left,
		EDialogueChoiceSlot::Right, EDialogueChoiceSlot::Down };

	const EDialogueChoiceSlot* Layout = nullptr;
	switch (Count)
	{
		case 1:
			Layout = Layout1; 	
			break;
		case 2: 
			Layout = Layout2; 
			break;
		case 3:
			Layout = Layout3;
			break;
		case 4: 
			Layout = Layout4; 
			break;
		default: return;
	}

	for (int32 i = 0; i < Count; ++i)
	{
		SlotToChoice[(int32)Layout[i]] = i;
	}
}

UDialogueChoiceItemWidget* UDialogueChoiceWidget::GetItemForSlot(const EDialogueChoiceSlot InSlot) const
{
	switch (InSlot)
	{
		case EDialogueChoiceSlot::Up:
		return ItemUp;
		case EDialogueChoiceSlot::Down:
		return ItemDown;
		case EDialogueChoiceSlot::Left:
		return ItemLeft;
		case EDialogueChoiceSlot::Right:
		return ItemRight;
		default: return nullptr;
	}
}

int32 UDialogueChoiceWidget::GetChoiceIndexForSlot(const EDialogueChoiceSlot InSlot) const
{
	return SlotToChoice.IsValidIndex((int32)InSlot) ? SlotToChoice[(int32)InSlot] : INDEX_NONE;
}

void UDialogueChoiceWidget::RefreshItems()
{
	for (int32 SlotIndex = 0; SlotIndex < (int32)EDialogueChoiceSlot::Max; ++SlotIndex)
	{
		const EDialogueChoiceSlot CurrentSlot = (EDialogueChoiceSlot)SlotIndex;

		UDialogueChoiceItemWidget* Item = GetItemForSlot(CurrentSlot);
		if (!Item)
		{
			continue;
		}

		const int32 Index = GetChoiceIndexForSlot(CurrentSlot);
		if (Choices.IsValidIndex(Index))
		{
			Item->Setup(Index, CurrentSlot, Choices[Index].Text);
		}
		else
		{
			Item->ClearItem();
		}
	}
}

void UDialogueChoiceWidget::ApplyHighlight()
{
	for (int32 SlotIndex = 0; SlotIndex < (int32)EDialogueChoiceSlot::Max; ++SlotIndex)
	{
		UDialogueChoiceItemWidget* Item = GetItemForSlot((EDialogueChoiceSlot)SlotIndex);
		if (!Item)
		{
			continue;
		}

		Item->SetHighlighted(Item->IsAssigned() && Item->GetChoiceIndex() == HighlightIndex);
	}
}

void UDialogueChoiceWidget::HighlightSlot(const EDialogueChoiceSlot InSlot)
{
	if (!bActive)
	{
		return;
	}

	const int32 ChoiceIndex = GetChoiceIndexForSlot(InSlot);
	if (!Choices.IsValidIndex(ChoiceIndex) || HighlightIndex == ChoiceIndex)
	{
		return;
	}

	// ハイライトが移ったらホールドは中断する
	EndConfirmHold();

	HighlightIndex = ChoiceIndex;
	ApplyHighlight();
}

void UDialogueChoiceWidget::BeginConfirmHold()
{
	if (!bActive || !Choices.IsValidIndex(HighlightIndex))
	{
		return;
	}

	if (HoldRequired <= 0.f)
	{
		CommitIndex(HighlightIndex);
		return;
	}

	HoldingIndex = HighlightIndex;
	HoldElapsed = 0.f;
}

void UDialogueChoiceWidget::EndConfirmHold()
{
	if (HoldingIndex == INDEX_NONE)
	{
		return;
	}

	for (int32 SlotIndex = 0; SlotIndex < (int32)EDialogueChoiceSlot::Max; ++SlotIndex)
	{
		UDialogueChoiceItemWidget* Item = GetItemForSlot((EDialogueChoiceSlot)SlotIndex);
		if (Item && Item->GetChoiceIndex() == HoldingIndex)
		{
			Item->SetHoldProgress(0.f);
			break;
		}
	}

	HoldingIndex = INDEX_NONE;
	HoldElapsed = 0.f;
}

void UDialogueChoiceWidget::CommitIndex(const int32 ChoiceIndex)
{
	if (!Choices.IsValidIndex(ChoiceIndex))
	{
		return;
	}

	// 二重入力を防ぐため、先に閉じる
	HideChoices();

	OnChoiceSelected.Broadcast(ChoiceIndex);
}


// Copyright 2022 wevet works All Rights Reserved.


#include "Cut/DialogueChoiceItemWidget.h"
#include "Components/TextBlock.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DialogueChoiceItemWidget)

void UDialogueChoiceItemWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (IsDesignTime() && ChoiceText)
	{
		ChoiceText->SetText(PreviewText);
	}
}

void UDialogueChoiceItemWidget::Setup(const int32 InChoiceIndex, const EDialogueChoiceSlot InChoiceSlot, const FText& InText)
{
	const bool bWasAssigned = IsAssigned();

	ChoiceIndex = InChoiceIndex;
	ChoiceSlot = InChoiceSlot;

	if (ChoiceText)
	{
		ChoiceText->SetText(InText);
	}

	SetHighlighted(false);
	SetHoldProgress(0.f);

	SetVisibility(ESlateVisibility::HitTestInvisible);

	if (!bWasAssigned)
	{
		OnAssignedStateChanged(true);
	}
}


void UDialogueChoiceItemWidget::ClearItem()
{
	const bool bWasAssigned = IsAssigned();

	ChoiceIndex = INDEX_NONE;
	bHighlighted = false;
	HoldProgress = 0.f;

	if (ChoiceText)
	{
		ChoiceText->SetText(FText::GetEmpty());
	}

	SetVisibility(ESlateVisibility::Collapsed);

	if (bWasAssigned)
	{
		OnAssignedStateChanged(false);
	}
}


void UDialogueChoiceItemWidget::SetHighlighted(bool bInHighlighted)
{
	if (bHighlighted == bInHighlighted)
	{
		return;
	}

	bHighlighted = bInHighlighted;
	OnHighlightStateChanged(bHighlighted);

	// ハイライトが外れたら進捗も落とす
	if (!bHighlighted && HoldProgress > 0.f)
	{
		SetHoldProgress(0.f);
	}
}


void UDialogueChoiceItemWidget::SetHoldProgress(float InProgress)
{
	const float Clamped = FMath::Clamp(InProgress, 0.f, 1.f);
	if (FMath::IsNearlyEqual(HoldProgress, Clamped))
	{
		return;
	}

	HoldProgress = Clamped;
	OnHoldProgressChanged(HoldProgress);
}


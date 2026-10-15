// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DialogueSystemTypes.h"
#include "DialogueChoiceItemWidget.generated.h"

class UTextBlock;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueItemInteractDelegate, int32, ChoiceIndex);


/**
 *
 */
UCLASS()
class VELOURS_API UDialogueChoiceItemWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativePreConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "Dialogue|Item")
	void Setup(const int32 InChoiceIndex, const EDialogueChoiceSlot InChoiceSlot, const FText& InText);

	UFUNCTION(BlueprintCallable, Category = "Dialogue|Item")
	void ClearItem();

	UFUNCTION(BlueprintCallable, Category = "Dialogue|Item")
	void SetHighlighted(bool bInHighlighted);

	UFUNCTION(BlueprintCallable, Category = "Dialogue|Item")
	void SetHoldProgress(float InProgress);

	UFUNCTION(BlueprintPure, Category = "Dialogue|Item")
	int32 GetChoiceIndex() const { return ChoiceIndex; }

	UFUNCTION(BlueprintPure, Category = "Dialogue|Item")
	EDialogueChoiceSlot GetSlot() const { return ChoiceSlot; }

	UFUNCTION(BlueprintPure, Category = "Dialogue|Item")
	bool IsAssigned() const { return ChoiceIndex != INDEX_NONE; }

	UPROPERTY(BlueprintAssignable, Category = "Dialogue|Item")
	FDialogueItemInteractDelegate OnItemPressed;

	UPROPERTY(BlueprintAssignable, Category = "Dialogue|Item")
	FDialogueItemInteractDelegate OnItemReleased;

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Dialogue|Item")
	void OnHighlightStateChanged(bool bInHighlighted);

	UFUNCTION(BlueprintImplementableEvent, Category = "Dialogue|Item")
	void OnHoldProgressChanged(float InProgress);

	UFUNCTION(BlueprintImplementableEvent, Category = "Dialogue|Item")
	void OnAssignedStateChanged(bool bInAssigned);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ChoiceText;

	UPROPERTY(EditAnywhere, Category = "Dialogue|Item|Preview")
	FText PreviewText = FText::FromString(TEXT("Text"));

	UPROPERTY(BlueprintReadOnly, Category = "Dialogue|Item")
	int32 ChoiceIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Dialogue|Item")
	EDialogueChoiceSlot ChoiceSlot = EDialogueChoiceSlot::Up;

	UPROPERTY(BlueprintReadOnly, Category = "Dialogue|Item")
	bool bHighlighted = false;

	UPROPERTY(BlueprintReadOnly, Category = "Dialogue|Item")
	float HoldProgress = 0.f;

};

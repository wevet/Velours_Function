// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UMGManager.generated.h"


// UI ManagerLayer
UENUM()
enum class EUMGLayerType : uint8
{
	Default = 0,
	Vehicle = 1,
	Overlay = 2,
	Dialog,
};

class ABasePawn;
class UCombatUIController;
class UMinimapUIController;
class UDialogueChoiceWidget;
/**
 *
 */
UCLASS()
class VELOURS_API UUMGManager : public UUserWidget
{
	GENERATED_BODY()


public:
	UUMGManager(const FObjectInitializer& ObjectInitializer);
	virtual void NativeConstruct() override;
	virtual void BeginDestroy() override;
	virtual void RemoveFromParent() override;


protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:
	void Initializer(ABasePawn* NewCharacter);
	void VisibleCombatUI(const bool bIsVisible);
	void VisibleMinimapUI(const bool bIsVisible);
	void VisibleDialogChoiceWidget(const bool bIsVisible);

	UDialogueChoiceWidget* GetDialogueChoiceWidget() const { return DialogueChoiceWidget; }


protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Controller)
	TSubclassOf<class UCombatUIController> CombatUIControllerTemplate;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UMinimapUIController> MinimapUIController{ nullptr };

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UDialogueChoiceWidget> DialogueChoiceWidget{ nullptr };

private:
	UPROPERTY()
	TObjectPtr<class UCombatUIController> CombatUIController{ nullptr };

	void SetTickableWhenPaused();


};


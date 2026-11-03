// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BendsInteractionPromptWidget.generated.h"

class UTextBlock;
class UProgressBar;
class UBendsInteractorComponent;
class UBendsInteractableComponent;

/**
 * インタラクトの表示（小さな印と名詞）。インタラクターが作り、ビューポートに置く。
 *
 * 対象の位置を画面に投影して、そこに出す（TPSでもFPSでも、物に付いて見える）。
 * FPSでは、照準のまわり（CenterAvoidRadius）を避けて外側に押し出し、視界の中央を空ける。
 * 画面端はScreenMarginの内側に収める。
 *
 * UMGでの作り方:
 *   ・Content（必須）  … 印と名詞をまとめるパネル。表示の切り替えはここで行う
 *   ・NameText（必須） … 名詞
 *   ・HoldProgress（任意） … 長押しの進み具合
 *   Alignmentは、印が左端なら(0, 0.5)。印が対象の位置に来るように合わせる
 */
UCLASS(Abstract)
class BENDSINTERACTION_API UBendsInteractionPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetInteractor(UBendsInteractorComponent* InInteractor);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> Content;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> HoldProgress;

	// ウィジェットのどこを対象の位置に合わせるか
	UPROPERTY(EditAnywhere, Category = "Prompt")
	FVector2D Alignment = FVector2D(0.f, 0.5f);

	// FPSで避ける、画面中央からの半径（ウィジェットの単位）
	UPROPERTY(EditAnywhere, Category = "Prompt", meta = (ClampMin = 0.0))
	float CenterAvoidRadius = 48.f;

	// 画面端の余白（画面サイズに対する割合）
	UPROPERTY(EditAnywhere, Category = "Prompt", meta = (ClampMin = 0.0, ClampMax = 0.4))
	float ScreenMargin = 0.06f;

private:
	void SetContentVisible(bool bVisible);

	TWeakObjectPtr<UBendsInteractorComponent> Interactor;
	TWeakObjectPtr<UBendsInteractableComponent> Shown;
};


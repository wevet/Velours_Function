// Copyright 2022 wevet works All Rights Reserved.

#include "UI/BendsInteractionPromptWidget.h"
#include "Interaction/BendsInteractorComponent.h"
#include "Interaction/BendsInteractableComponent.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BendsInteractionPromptWidget)

void UBendsInteractionPromptWidget::SetInteractor(UBendsInteractorComponent* InInteractor)
{
	Interactor = InInteractor;
}

void UBendsInteractionPromptWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	SetAlignmentInViewport(Alignment);
	SetContentVisible(false);
}

void UBendsInteractionPromptWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UBendsInteractorComponent* Comp = Interactor.Get();
	UBendsInteractableComponent* Target = Comp ? Comp->GetFocused() : nullptr;
	APlayerController* PC = GetOwningPlayer();

	FVector2D ScreenPosition;
	if (!Target || !PC || !PC->ProjectWorldLocationToScreen(Target->GetPromptLocation(), ScreenPosition, true))
	{
		Shown.Reset();
		SetContentVisible(false);
		return;
	}

	int32 ViewportX = 0;
	int32 ViewportY = 0;
	PC->GetViewportSize(ViewportX, ViewportY);
	const FVector2D ViewportSize(ViewportX, ViewportY);

	// FPS‚ÍÆ€‚Ì‚Ü‚í‚è‚ð‹ó‚¯‚é
	if (Comp->GetViewMode() == EBendsViewMode::FirstPerson)
	{
		const FVector2D Center = ViewportSize * 0.5f;
		const float Radius = CenterAvoidRadius * UWidgetLayoutLibrary::GetViewportScale(this);
		FVector2D FromCenter = ScreenPosition - Center;
		const float Distance = FromCenter.Size();
		if (Distance < Radius)
		{
			FromCenter = (Distance > KINDA_SMALL_NUMBER) ? FromCenter / Distance : FVector2D(1.f, 0.f);
			ScreenPosition = Center + FromCenter * Radius;
		}
	}

	const FVector2D Margin = ViewportSize * ScreenMargin;
	ScreenPosition.X = FMath::Clamp(ScreenPosition.X, Margin.X, ViewportSize.X - Margin.X);
	ScreenPosition.Y = FMath::Clamp(ScreenPosition.Y, Margin.Y, ViewportSize.Y - Margin.Y);
	SetPositionInViewport(ScreenPosition, true);

	if (Shown.Get() != Target)
	{
		Shown = Target;
		NameText->SetText(Target->DisplayName);
	}

	if (HoldProgress)
	{
		const bool bHold = Target->Mode == EBendsInteractionMode::Hold;
		HoldProgress->SetVisibility(bHold ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		HoldProgress->SetPercent(Comp->GetHoldProgress());
	}

	SetContentVisible(true);
}

void UBendsInteractionPromptWidget::SetContentVisible(bool bVisible)
{
	if (Content)
	{
		Content->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}



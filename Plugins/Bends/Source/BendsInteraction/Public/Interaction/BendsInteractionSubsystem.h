// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "BendsInteractionSubsystem.generated.h"

class UBendsInteractableComponent;

/**
 * ワールド内のインタラクト対象の一覧。
 * 対象はBeginPlayで登録され、インタラクターはここから候補を選ぶ（物理の当たり判定を使わない）。
 */
UCLASS()
class BENDSINTERACTION_API UBendsInteractionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void Register(UBendsInteractableComponent* Interactable);
	void Unregister(UBendsInteractableComponent* Interactable);

	const TArray<TWeakObjectPtr<UBendsInteractableComponent>>& GetInteractables() const { return Interactables; }

private:
	TArray<TWeakObjectPtr<UBendsInteractableComponent>> Interactables;
};


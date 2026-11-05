// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WvVisualComponent.generated.h"


UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VELOURS_API UWvVisualComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UWvVisualComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;



	UFUNCTION(BlueprintCallable, Category = "VisualComponent")
	void RefreshVisual();
};

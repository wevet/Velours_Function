// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "VTacticalAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class VELOURS_API UVTacticalAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	UVTacticalAnimInstance();
	virtual ~UVTacticalAnimInstance() {}

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUninitializeAnimation() override;
	virtual void NativeBeginPlay() override;

	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VTacticalAnimInstance")
	TObjectPtr<class APawn> OwnedPawn{nullptr};

	UFUNCTION(BlueprintPure, Category = "VTacticalAnimInstance", meta = (BlueprintThreadSafe))
	AActor* FindTacticalOwnerActor() const;
};

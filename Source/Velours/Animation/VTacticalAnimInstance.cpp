// Copyright 2022 wevet works All Rights Reserved.


#include "Animation/VTacticalAnimInstance.h"
#include "Character/VMetaHumanVisualActor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(VTacticalAnimInstance)

UVTacticalAnimInstance::UVTacticalAnimInstance()
{

}

void UVTacticalAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
}

void UVTacticalAnimInstance::NativeUninitializeAnimation()
{
	Super::NativeUninitializeAnimation();
}

void UVTacticalAnimInstance::NativeBeginPlay()
{
	Super::NativeBeginPlay();
}

void UVTacticalAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);


	if (!OwnedPawn)
	{
		if (AVMetaHumanVisualActor* RawOwner = Cast<AVMetaHumanVisualActor>(GetOwningActor()))
		{
			OwnedPawn = Cast<APawn>(RawOwner->GetOwner());
		}
	}
}

void UVTacticalAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);
}

AActor* UVTacticalAnimInstance::FindTacticalOwnerActor() const
{
	if (AVMetaHumanVisualActor* RawOwner = Cast<AVMetaHumanVisualActor>(GetOwningActor()))
	{
		return RawOwner->GetOwner();
	}

	return GetOwningActor();
}


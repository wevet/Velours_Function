
// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "VFirstPersonVisual.generated.h"

class USceneComponent;

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UVFirstPersonVisual : public UInterface
{
	GENERATED_BODY()
};

class VELOURS_API IVFirstPersonVisual
{
	GENERATED_BODY()

public:
	virtual void SetFirstPersonView(bool bIsFirstPerson) = 0;

	virtual bool GetFirstPersonCameraAttachment(USceneComponent*& OutParent, FName& OutSocket) const = 0;
};


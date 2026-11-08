// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "VVisualOverrideProvider.generated.h"

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UVVisualOverrideProvider : public UInterface
{
	GENERATED_BODY()
};

class VELOURS_API IVVisualOverrideProvider
{
	GENERATED_BODY()

public:
	virtual const TArray<TSoftClassPtr<AActor>>& GetVisualOverrides() const = 0;
};


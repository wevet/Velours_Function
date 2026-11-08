// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WvVisualComponent.generated.h"

class IConsoleVariable;
struct FStreamableHandle;


UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VELOURS_API UWvVisualComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWvVisualComponent();

	UFUNCTION(BlueprintCallable, Category = "Visual")
	void RefreshVisual();

	void FindAndApplyVisualOverride();

	void LoadVisualOverride(const TSoftClassPtr<AActor>& AssetClass);

	void ApplyVisualOverride();

	TSubclassOf<AActor> GetCurrentVisualOverride() const { return CurrentVisualOverride; }

	UPROPERTY(EditDefaultsOnly, Category = "VisualOverride")
	bool bEnableVisualOverride = true;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, Category = "VisualOverride", meta = (EditCondition = "bEnableVisualOverride"))
	FName VisualOverrideTag = TEXT("VisualOverride");

	UPROPERTY(EditDefaultsOnly, Category = "VisualOverride", meta = (EditCondition = "bEnableVisualOverride"))
	FString CVarName = TEXT("DDCvar.VisualOverride");

	UPROPERTY(EditDefaultsOnly, Category = "VisualOverride", meta = (EditCondition = "bEnableVisualOverride"))
	TArray<TSoftClassPtr<AActor>> DefaultVisualOverrides;


private:
	const TArray<TSoftClassPtr<AActor>>& GetVisualOverrides() const;
	TSoftClassPtr<AActor> GetVisualOverrideWithCVar(const TArray<TSoftClassPtr<AActor>>& VisualOverrides) const;

	void HandleLoaded(TSoftClassPtr<AActor> AssetClass);
	void HandleCVarChanged(IConsoleVariable* Variable);
	void SetVisualOverride(TSubclassOf<AActor> NewClass);
	void CancelLoad();
	void UnbindCVar();

	UPROPERTY(Transient)
	TSubclassOf<AActor> CurrentVisualOverride;

	TSharedPtr<FStreamableHandle> LoadHandle;
	FDelegateHandle CVarChangedHandle;
};


// Copyright 2022 wevet works All Rights Reserved.

#include "Component/WvVisualComponent.h"
#include "Character/BasePawn.h"
#include "Character/VVisualOverrideProvider.h"
#include "Components/ChildActorComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "HAL/IConsoleManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvVisualComponent)

DEFINE_LOG_CATEGORY_STATIC(LogWvVisual, Log, All);


UWvVisualComponent::UWvVisualComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWvVisualComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!bEnableVisualOverride)
	{
		return;
	}

	FindAndApplyVisualOverride();

	// CVarが変わったら選び直す
	if (IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(*CVarName))
	{
		CVarChangedHandle = Variable->OnChangedDelegate().AddUObject(this, &UWvVisualComponent::HandleCVarChanged);
	}
	else
	{
		UE_LOG(LogWvVisual, Warning, TEXT("[%s] CVar %s not found."), *FString(__FUNCTION__), *CVarName);
	}
}

void UWvVisualComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelLoad();
	UnbindCVar();
	Super::EndPlay(EndPlayReason);
}

void UWvVisualComponent::RefreshVisual()
{
	// ← 今のRefreshVisualの中身に合わせる
	if (ABasePawn* Pawn = Cast<ABasePawn>(GetOwner()))
	{
		Pawn->RefreshVisual();
	}
}


#pragma region VisualOverride
void UWvVisualComponent::HandleCVarChanged(IConsoleVariable* Variable)
{
	FindAndApplyVisualOverride();
}

void UWvVisualComponent::FindAndApplyVisualOverride()
{
	LoadVisualOverride(GetVisualOverrideWithCVar(GetVisualOverrides()));
}

const TArray<TSoftClassPtr<AActor>>& UWvVisualComponent::GetVisualOverrides() const
{
	if (const UWorld* World = GetWorld())
	{
		if (const IVVisualOverrideProvider* Provider = Cast<IVVisualOverrideProvider>(World->GetAuthGameMode()))
		{
			return Provider->GetVisualOverrides();
		}
	}
	return DefaultVisualOverrides;
}

TSoftClassPtr<AActor> UWvVisualComponent::GetVisualOverrideWithCVar(const TArray<TSoftClassPtr<AActor>>& VisualOverrides) const
{
	const IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(*CVarName);
	const int32 Index = Variable ? Variable->GetInt() : INDEX_NONE;
	return VisualOverrides.IsValidIndex(Index) ? VisualOverrides[Index] : TSoftClassPtr<AActor>();
}

void UWvVisualComponent::LoadVisualOverride(const TSoftClassPtr<AActor>& AssetClass)
{
	CancelLoad();

	if (AssetClass.IsNull())
	{
		SetVisualOverride(nullptr);
		return;
	}

	// 読み込み済みなら、すぐに差し替える
	if (UClass* Loaded = AssetClass.Get())
	{
		SetVisualOverride(Loaded);
		return;
	}

	FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
	LoadHandle = Streamable.RequestAsyncLoad(
		AssetClass.ToSoftObjectPath(),
		FStreamableDelegate::CreateUObject(this, &UWvVisualComponent::HandleLoaded, AssetClass));
}

void UWvVisualComponent::HandleLoaded(TSoftClassPtr<AActor> AssetClass)
{
	LoadHandle.Reset();

	UClass* Loaded = AssetClass.Get();
	if (!Loaded)
	{
		UE_LOG(LogWvVisual, Warning, TEXT("[%s] Failed to load %s."), *FString(__FUNCTION__), *AssetClass.ToString());
	}
	SetVisualOverride(Loaded);
}

void UWvVisualComponent::SetVisualOverride(TSubclassOf<AActor> NewClass)
{
	CurrentVisualOverride = NewClass;
	ApplyVisualOverride();
}

void UWvVisualComponent::ApplyVisualOverride()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	const bool bHasOverride = CurrentVisualOverride != nullptr;

	TArray<UActorComponent*> Components = Owner->GetComponentsByTag(UChildActorComponent::StaticClass(), VisualOverrideTag);
	for (UActorComponent* Component : Components)
	{
		UChildActorComponent* ChildActor = Cast<UChildActorComponent>(Component);
		if (!ChildActor)
		{
			continue;
		}

		// 子アクターはここで作り直される（BeginPlayも済んだ状態で戻る）
		ChildActor->SetChildActorClass(CurrentVisualOverride);

		// 差し替えがあるときは、付け先の体（マネキン）を隠す。アニメーションは動かし続ける必要がある
		if (USkeletalMeshComponent* BaseMesh = Cast<USkeletalMeshComponent>(ChildActor->GetAttachParent()))
		{
			BaseMesh->SetVisibility(!bHasOverride);
		}
	}

	UE_LOG(LogWvVisual, Log, TEXT("[%s] VisualOverride => %s"), *FString(__FUNCTION__), *GetNameSafe(CurrentVisualOverride.Get()));

	RefreshVisual();
}

void UWvVisualComponent::CancelLoad()
{
	if (LoadHandle.IsValid())
	{
		LoadHandle->CancelHandle();
		LoadHandle.Reset();
	}
}

void UWvVisualComponent::UnbindCVar()
{
	if (!CVarChangedHandle.IsValid())
	{
		return;
	}

	if (IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(*CVarName))
	{
		Variable->OnChangedDelegate().Remove(CVarChangedHandle);
	}
	CVarChangedHandle.Reset();
}

#pragma endregion


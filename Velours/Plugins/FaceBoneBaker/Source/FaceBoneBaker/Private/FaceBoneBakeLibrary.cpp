// Fill out your copyright notice in the Description page of Project Settings.


#include "FaceBoneBakeLibrary.h"

#include "Animation/AnimSequence.h"
#include "Animation/AnimInstance.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "Animation/AnimData/IAnimationDataModel.h"
#include "Animation/AnimCurveTypes.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "PreviewScene.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Misc/ScopedSlowTask.h"
#include "Misc/PackageName.h"

#define LOCTEXT_NAMESPACE "FaceBoneBaker"

DEFINE_LOG_CATEGORY_STATIC(LogFaceBoneBaker, Log, All);

namespace FaceBoneBake
{
	// 初めて出てきたカーブは全フレーム0で初期化してから値を入れる
	static void StoreCurves(
		const TMap<FName, float>& InCurves, 
		int32 Key, 
		int32 NumKeys,
		TMap<FName, TArray<float>>& OutCurves)
	{
		for (const TPair<FName, float>& Pair : InCurves)
		{
			TArray<float>* Values = OutCurves.Find(Pair.Key);
			if (!Values)
			{
				Values = &OutCurves.Add(Pair.Key);
				Values->SetNumZeroed(NumKeys);
			}

			(*Values)[Key] = Pair.Value;
		}
	}
}

UAnimSequence* UFaceBoneBakeLibrary::BakeFaceCurvesToBones(UAnimSequence* Source, const FFaceBoneBakeOptions& Options)
{
	if (!Source || !Source->GetSkeleton())
	{
		UE_LOG(LogFaceBoneBaker, Error, TEXT("Invalid source animation."));
		return nullptr;
	}

	// ---- 評価用メッシュ決定 ----
	USkeletalMesh* Mesh = Options.FaceMesh;
	if (!Mesh)
	{
		Mesh = Source->GetPreviewMesh(); 
	}

	if (!Mesh) 
	{ 
		Mesh = Source->GetSkeleton()->GetPreviewMesh(true); 
	}

	if (!Mesh)
	{
		UE_LOG(LogFaceBoneBaker, Error, TEXT("%s: no face mesh found. Set a preview mesh or pass FaceMesh."), *Source->GetName());
		return nullptr;
	}

	if (!Mesh->GetPostProcessAnimBlueprint())
	{
		UE_LOG(LogFaceBoneBaker, Warning, TEXT("%s has no Post Process AnimBP. RigLogic will not run."), *Mesh->GetName());
	}

	const IAnimationDataModel* SrcModel = Source->GetDataModel();
	const int32 NumKeys = SrcModel->GetNumberOfKeys();
	const FFrameRate FrameRate = SrcModel->GetFrameRate();
	if (NumKeys <= 0)
	{
		return nullptr;
	}

	// ---- 一時プレビューシーンで評価環境を作る ----
	FPreviewScene::ConstructionValues CV;
	CV.bDefaultLighting = false;
	CV.bCreatePhysicsScene = false;
	CV.bShouldSimulatePhysics = false;
	CV.bTransactional = false;
	FPreviewScene Scene(CV);

	USkeletalMeshComponent* Comp = NewObject<USkeletalMeshComponent>(GetTransientPackage(), NAME_None, RF_Transient);
	Comp->SetSkeletalMesh(Mesh);
	Comp->SetForcedLOD(1); // 1 = LOD0固定
	Comp->bEnableUpdateRateOptimizations = false;
	Comp->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	Comp->SetUpdateAnimationInEditor(true);
	Comp->SetDisablePostProcessBlueprint(false);
	Scene.AddComponent(Comp, FTransform::Identity);

	Comp->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	Comp->SetAnimation(Source);
	Comp->Stop();

	const FReferenceSkeleton& MeshRefSkel = Mesh->GetRefSkeleton();
	const int32 NumBones = MeshRefSkel.GetNum();

	TArray<TArray<FVector3f>> Positions; 
	TArray<TArray<FQuat4f>> Rotations;
	TArray<TArray<FVector3f>> Scales;
	Positions.SetNum(NumBones);
	Rotations.SetNum(NumBones);
	Scales.SetNum(NumBones);

	for (int32 b = 0; b < NumBones; ++b)
	{
		Positions[b].Reserve(NumKeys);
		Rotations[b].Reserve(NumKeys);
		Scales[b].Reserve(NumKeys);
	}

	TMap<FName, TArray<float>> MorphCurves;
	TMap<FName, TArray<float>> MaterialCurves;

	// ---- フレームごとに評価してサンプリング ----
	FScopedSlowTask Task(NumKeys, FText::Format(LOCTEXT("Baking", "Baking {0}..."), FText::FromString(Source->GetName())));
	Task.MakeDialog(true);

	auto Evaluate = [Comp](double Time)
		{
			Comp->SetPosition(static_cast<float>(Time), false);
			Comp->TickAnimation(0.f, false);
			// TickFunction = nullptr → 並列評価されずゲームスレッドで即時評価（Post Process込み）
			Comp->RefreshBoneTransforms(nullptr);
		};

	Evaluate(0.0); // ウォームアップ

	for (int32 Key = 0; Key < NumKeys; ++Key)
	{
		if (Task.ShouldCancel())
		{
			UE_LOG(LogFaceBoneBaker, Warning, TEXT("Bake cancelled."));
			return nullptr;
		}
		Task.EnterProgressFrame();

		const double Time = FrameRate.AsSeconds(FFrameTime(FFrameNumber(Key)));
		Evaluate(Time);

		const TArray<FTransform>& Local = Comp->GetBoneSpaceTransforms();
		for (int32 b = 0; b < NumBones; ++b)
		{
			const FTransform& T = Local.IsValidIndex(b) ? Local[b] : MeshRefSkel.GetRefBonePose()[b];
			Positions[b].Add(FVector3f(T.GetLocation()));
			Rotations[b].Add(FQuat4f(T.GetRotation().GetNormalized()));
			Scales[b].Add(FVector3f(T.GetScale3D()));
		}

		if (Options.bRecordRigLogicCurves)
		{
			if (UAnimInstance* PP = Comp->GetPostProcessInstance())
			{
				FaceBoneBake::StoreCurves(PP->GetAnimationCurveList(EAnimCurveType::MorphTargetCurve), Key, NumKeys, MorphCurves);
				FaceBoneBake::StoreCurves(PP->GetAnimationCurveList(EAnimCurveType::MaterialCurve), Key, NumKeys, MaterialCurves);
			}
		}
	}

	// ---- 元アセットを複製（Notify・既存カーブ等を引き継ぐ）----
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	const FString PackagePath = FPackageName::GetLongPackagePath(Source->GetPackage()->GetName());
	FString UniquePackageName, UniqueAssetName;
	AssetTools.CreateUniqueAssetName(PackagePath / (Source->GetName() + Options.Suffix), TEXT(""), UniquePackageName, UniqueAssetName);

	UAnimSequence* Baked = Cast<UAnimSequence>(AssetTools.DuplicateAsset(UniqueAssetName, PackagePath, Source));
	if (!Baked)
	{
		return nullptr;
	}

	USkeleton* Skeleton = Baked->GetSkeleton();
	const FReferenceSkeleton& SkelRefSkel = Skeleton->GetReferenceSkeleton();

	IAnimationDataController& Ctrl = Baked->GetController();
	Ctrl.OpenBracket(LOCTEXT("BakeBracket", "Bake Face Curves To Bones"));

	// ボーントラック
	Ctrl.RemoveAllBoneTracks();
	for (int32 b = 0; b < NumBones; ++b)
	{
		const FName BoneName = MeshRefSkel.GetBoneName(b);
		if (SkelRefSkel.FindBoneIndex(BoneName) == INDEX_NONE)
		{
			continue;
		}
		Ctrl.AddBoneCurve(BoneName);
		Ctrl.SetBoneTrackKeys(BoneName, Positions[b], Rotations[b], Scales[b]);
	}

	// CTRL_* カーブ削除
	if (Options.bRemoveControlCurves && !Options.ControlCurvePrefix.IsEmpty())
	{
		TArray<FName> ToRemove;
		for (const FFloatCurve& Curve : Baked->GetDataModel()->GetFloatCurves())
		{
			if (Curve.GetName().ToString().StartsWith(Options.ControlCurvePrefix))
			{
				ToRemove.Add(Curve.GetName());
			}
		}

		for (const FName& Name : ToRemove)
		{
			Ctrl.RemoveCurve(FAnimationCurveIdentifier(Name, ERawCurveTrackTypes::RCT_Float));
		}
	}

	// RigLogic出力カーブ書き込み
	auto WriteCurves = [&](const TMap<FName, TArray<float>>& Curves, bool bMaterial, bool bMorph)
		{
			for (const TPair<FName, TArray<float>>& Pair : Curves)
			{
				const FAnimationCurveIdentifier Id(Pair.Key, ERawCurveTrackTypes::RCT_Float);
				if (!Baked->GetDataModel()->FindFloatCurve(Id))
				{
					Ctrl.AddCurve(Id);
				}

				TArray<FRichCurveKey> Keys;
				Keys.Reserve(NumKeys);
				for (int32 k = 0; k < NumKeys; ++k)
				{
					const float T = static_cast<float>(FrameRate.AsSeconds(FFrameTime(FFrameNumber(k))));
					Keys.Add(FRichCurveKey(T, Pair.Value[k]));
				}
				Ctrl.SetCurveKeys(Id, Keys);

				Skeleton->AccumulateCurveMetaData(Pair.Key, bMaterial, bMorph);
			}
		};

	WriteCurves(MorphCurves, /*bMaterial*/false, /*bMorph*/true);
	WriteCurves(MaterialCurves, /*bMaterial*/true, /*bMorph*/false);

	Ctrl.CloseBracket();

	Baked->MarkPackageDirty();
	Skeleton->MarkPackageDirty();

	UE_LOG(LogFaceBoneBaker, Log, TEXT("Baked %s -> %s (%d keys, %d bones, %d morph, %d material curves)"),
		*Source->GetName(),
		*Baked->GetName(), 
		NumKeys, 
		NumBones,
		MorphCurves.Num(), 
		MaterialCurves.Num());

	return Baked;
}

#undef LOCTEXT_NAMESPACE



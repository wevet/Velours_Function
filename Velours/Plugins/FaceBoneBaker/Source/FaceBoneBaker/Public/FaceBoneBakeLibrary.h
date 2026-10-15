// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FaceBoneBakeLibrary.generated.h"

class UAnimSequence;
class USkeletalMesh;

USTRUCT(BlueprintType)
struct FFaceBoneBakeOptions
{
    GENERATED_BODY()

    /** 評価に使うFaceメッシュ。未指定ならAnimSequence → Skeletonのプレビューメッシュを使う */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bake")
    TObjectPtr<USkeletalMesh> FaceMesh = nullptr;

    /** RigLogicが出力するMorph Target / Materialカーブも焼く */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bake")
    bool bRecordRigLogicCurves = true;

    /** 入力側のコントロールカーブ（CTRL_*）を削除する */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bake")
    bool bRemoveControlCurves = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bake")
    FString ControlCurvePrefix = TEXT("CTRL_");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bake")
    FString Suffix = TEXT("_BoneBaked");
};

/**
 * 
 */
UCLASS()
class FACEBONEBAKER_API UFaceBoneBakeLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
    /**
    * Evaluate MetaHuman face curve animations using RigLogic and create a new AnimSequence by baking them into the bone track.
    */
    UFUNCTION(BlueprintCallable, Category = "MetaHuman|Bake")
    static UAnimSequence* BakeFaceCurvesToBones(UAnimSequence* Source, const FFaceBoneBakeOptions& Options);
};

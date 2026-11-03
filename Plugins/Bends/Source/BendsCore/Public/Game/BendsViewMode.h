// Copyright 2022 wevet works All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "BendsViewMode.generated.h"

UENUM(BlueprintType)
enum class EBendsViewMode : uint8
{
	FirstPerson,
	ThirdPerson,
};

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UBendsViewModeProvider : public UInterface
{
	GENERATED_BODY()
};

/**
 * 今の視点（FPSかTPSか）を返すインターフェース。プレイヤーのポーンが実装する。
 * インタラクトの探し方、字幕の位置、カメラの演出など、視点で振る舞いを変える仕組みが使う。
 * Pt1はFPSのみだが、Pt2のTPSに備えて、視点を知りたい側は必ずここを通す。
 */
class BENDSCORE_API IBendsViewModeProvider
{
	GENERATED_BODY()

public:
	virtual EBendsViewMode GetViewMode() const { return EBendsViewMode::FirstPerson; }

	// アクターが実装していなければFPSとして扱う
	static EBendsViewMode GetViewModeOf(const AActor* Actor);
};




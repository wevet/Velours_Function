// Copyright Epic Games, Inc. All Rights Reserved.

#include "FaceBoneBaker.h"
#include "Modules/ModuleManager.h"
#include "ToolMenus.h"
#include "Animation/AnimSequence.h"
#include "Styling/AppStyle.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "ContentBrowserMenuContexts.h"
#include "FaceBoneBakeLibrary.h"

#define LOCTEXT_NAMESPACE "FaceBoneBaker"

void FFaceBoneBakerModule::StartupModule()
{
	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FFaceBoneBakerModule::RegisterMenus));

}

void FFaceBoneBakerModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
}

void FFaceBoneBakerModule::RegisterMenus()
{
    FToolMenuOwnerScoped OwnerScoped(this);

    UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("ContentBrowser.AssetContextMenu.AnimSequence");
    FToolMenuSection& Section = Menu->FindOrAddSection("GetAssetActions");

    Section.AddDynamicEntry("FaceBoneBake", FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
        {
            if (!InSection.FindContext<UContentBrowserAssetContextMenuContext>())
            {
                return;
            }

            InSection.AddMenuEntry(
                "BakeFaceCurvesToBones",
                LOCTEXT("BakeLabel", "Bake Face Curves To Bones (RigLogic)"),
                LOCTEXT("BakeTooltip", "MetaHumanのFaceカーブをRigLogicで評価し、ボーントラックに焼いた新規AnimSequenceを作成します"),
                FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.AnimSequence"),
                FToolMenuExecuteAction::CreateStatic(&FFaceBoneBakerModule::ExecuteBake));
        }));
}

void FFaceBoneBakerModule::ExecuteBake(const FToolMenuContext& MenuContext)
{
    const UContentBrowserAssetContextMenuContext* Ctx = MenuContext.FindContext<UContentBrowserAssetContextMenuContext>();
    if (!Ctx)
    {
        return;
    }

    const TArray<UAnimSequence*> Sequences = Ctx->LoadSelectedObjects<UAnimSequence>();

    int32 Succeeded = 0;
    const FFaceBoneBakeOptions Options; // 既定値。必要ならダイアログ化
    for (UAnimSequence* Seq : Sequences)
    {
        if (UFaceBoneBakeLibrary::BakeFaceCurvesToBones(Seq, Options))
        {
            ++Succeeded;
        }
    }

    FNotificationInfo Info(FText::Format(
        LOCTEXT("BakeDone", "Face bone bake: {0} / {1} succeeded"),
        FText::AsNumber(Succeeded), FText::AsNumber(Sequences.Num())));

    Info.ExpireDuration = 4.f;
    FSlateNotificationManager::Get().AddNotification(Info);
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FFaceBoneBakerModule, FaceBoneBaker)
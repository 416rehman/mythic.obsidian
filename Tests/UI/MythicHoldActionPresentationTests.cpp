#include "Misc/AutomationTest.h"

#include "CommonInputTypeEnum.h"
#include "CommonUITypes.h"
#include "Engine/DataTable.h"
#include "Materials/MaterialInterface.h"
#include "UI/Widgets/MythicBoundActionButton.h"
#include "UI/Widgets/MythicInputGlyph.h"

namespace {
const TCHAR *GenericActionButtonClassPath =
    TEXT("/Game/Mythic/UI/Globals/styles/Buttons/WBPMythic_GenericActionButton.WBPMythic_GenericActionButton_C");
const TCHAR *InteractionActionsPath =
    TEXT("/Game/Mythic/UI/Interaction/DT_InteractionActions.DT_InteractionActions");
const TCHAR *HoldProgressMaterialPath =
    TEXT("/Game/Mythic/UI/Globals/materials/M_DownProgressMaterial.M_DownProgressMaterial");
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSharedHoldActionPresentationTest,
    "Mythic.UI.CommonUI.HoldActionPresentation",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicSharedHoldActionPresentationTest::RunTest(
    const FString & /*Parameters*/) {
    UClass *ButtonClass = LoadClass<UObject>(nullptr,
                                             GenericActionButtonClassPath);
    if (!TestNotNull(TEXT("the shared CommonUI action button loads"),
                     ButtonClass)) {
        return false;
    }
    TestTrue(TEXT("the shared action button uses Mythic's hold-capable native contract"),
             ButtonClass->IsChildOf(UMythicBoundActionButton::StaticClass()));

    const UMythicBoundActionButton *ButtonDefaults =
        ButtonClass->GetDefaultObject<UMythicBoundActionButton>();
    if (TestNotNull(TEXT("the shared action button has defaults"),
                    ButtonDefaults)) {
        TestTrue(TEXT("represented hold mappings drive pointer and Blueprint hold state"),
                 ButtonDefaults->IsRepresentedHoldLinked());
    }

    const UMythicInputGlyph *GlyphDefaults =
        GetDefault<UMythicInputGlyph>();
    TestTrue(TEXT("every Mythic input glyph has the shared hold presentation configured"),
             GlyphDefaults->HasConfiguredHoldPresentation());

    UMaterialInterface *ProgressMaterial =
        LoadObject<UMaterialInterface>(nullptr, HoldProgressMaterialPath);
    if (TestNotNull(TEXT("the canonical bottom-to-top hold material loads"),
                    ProgressMaterial)) {
        float Percentage = -1.0f;
        TestTrue(TEXT("the hold material exposes CommonUI's percentage scalar"),
                 ProgressMaterial->GetScalarParameterValue(
                     FMaterialParameterInfo(TEXT("percentage")), Percentage));
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicWorldInteractionHoldRowsTest,
    "Mythic.UI.CommonUI.WorldInteractionHoldRows",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicWorldInteractionHoldRowsTest::RunTest(
    const FString & /*Parameters*/) {
    const UDataTable *Actions =
        LoadObject<UDataTable>(nullptr, InteractionActionsPath);
    if (!TestNotNull(TEXT("the world-interaction CommonUI table loads"),
                     Actions)) {
        return false;
    }
    TestTrue(TEXT("the interaction table uses CommonUI action rows"),
             Actions->GetRowStruct()
                 == FCommonInputActionDataBase::StaticStruct());

    static const TArray<FName> HoldRows = {
        TEXT("pickup_hold"), TEXT("use_hold"), TEXT("rest_hold"),
        TEXT("talk_hold"), TEXT("dismantle_hold")};
    for (const FName RowName : HoldRows) {
        const FCommonInputActionDataBase *Row =
            Actions->FindRow<FCommonInputActionDataBase>(
                RowName, TEXT("Mythic hold presentation test"));
        if (!TestNotNull(*FString::Printf(TEXT("%s exists"),
                                          *RowName.ToString()),
                         Row)) {
            continue;
        }
        const FCommonInputTypeInfo &Keyboard = Row->GetInputTypeInfo(
            ECommonInputType::MouseAndKeyboard, NAME_None);
        const FCommonInputTypeInfo &Gamepad =
            Row->GetDefaultGamepadInputTypeInfo();
        TestTrue(*FString::Printf(TEXT("%s requires a keyboard hold"),
                                  *RowName.ToString()),
                 Keyboard.bActionRequiresHold);
        TestTrue(*FString::Printf(TEXT("%s has deliberate keyboard dwell"),
                                  *RowName.ToString()),
                 Keyboard.HoldTime >= 0.25f);
        TestTrue(*FString::Printf(TEXT("%s requires a controller hold"),
                                  *RowName.ToString()),
                 Gamepad.bActionRequiresHold);
        TestTrue(*FString::Printf(TEXT("%s has deliberate controller dwell"),
                                  *RowName.ToString()),
                 Gamepad.HoldTime >= 0.25f);
    }

    return true;
}

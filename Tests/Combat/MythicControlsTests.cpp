#include "Misc/AutomationTest.h"
#include "Input/MythicControlsComponent.h"
#include "Input/MythicControlUserSettings.h"
#include "Input/MythicControlsSettings.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "PlayerMappableKeySettings.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Engine.h"
#include "Subsystem/SaveSystem/MythicSaveGame.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicControlsDefaultContracts,"Mythic.Controls.DefaultContracts",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::ProductFilter)
bool FMythicControlsDefaultContracts::RunTest(const FString&) {
    const auto& Bindings=GetDefault<UMythicControlsSettings>()->Bindings;
    const auto Resolve=[&](FKey Key,bool Layer){return UMythicControlsComponent::ResolvePress(Bindings,Key,Layer);};
    TestEqual(TEXT("RT primary without modifier"),Resolve(EKeys::Gamepad_RightTrigger,false),EMythicControl::Primary);
    TestEqual(TEXT("LB RT ability Q"),Resolve(EKeys::Gamepad_RightTrigger,true),EMythicControl::SkillQ);
    TestEqual(TEXT("LB X ability R owns interaction key"),Resolve(EKeys::Gamepad_FaceButton_Left,true),EMythicControl::SkillR);
    TestEqual(TEXT("X interacts without modifier"),Resolve(EKeys::Gamepad_FaceButton_Left,false),EMythicControl::Interact);
    TestEqual(TEXT("LB A still jumps"),Resolve(EKeys::Gamepad_FaceButton_Bottom,true),EMythicControl::Jump);
    TestEqual(TEXT("LB B still dashes"),Resolve(EKeys::Gamepad_FaceButton_Right,true),EMythicControl::Dash);
    TestEqual(TEXT("LB LT still secondary"),Resolve(EKeys::Gamepad_LeftTrigger,true),EMythicControl::Secondary);
    TestEqual(TEXT("R is slot three, no reload"),Resolve(EKeys::R,false),EMythicControl::SkillR);
    TestEqual(TEXT("T is slot four"),Resolve(EKeys::T,false),EMythicControl::SkillT);
    TestEqual(TEXT("Shift only dashes"),Resolve(EKeys::LeftShift,false),EMythicControl::Dash);
    TestEqual(TEXT("Z selects healing"),Resolve(EKeys::Z,false),EMythicControl::Heal);
    TestEqual(TEXT("V has no heavy action"),Resolve(EKeys::V,false),EMythicControl::Count);
    TestEqual(TEXT("L3 has no sprint or zoom"),Resolve(EKeys::Gamepad_LeftThumbstick,false),EMythicControl::Count);
    TestEqual(TEXT("invalid key never triggers an unauthored cell"),Resolve(EKeys::Invalid,true),EMythicControl::Count);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicControlProfileClear,"Mythic.Controls.ProfileClearAndReset",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::ProductFilter)
bool FMythicControlProfileClear::RunTest(const FString&) {
    // Pure in-memory Enhanced Input profile. No automation write touches the user's saved bindings.
    auto* Settings=NewObject<UMythicControlUserSettings>(); auto* Local=NewObject<ULocalPlayer>(GEngine); Settings->Initialize(Local);
    auto* Profile=Cast<UMythicControlKeyProfile>(Settings->GetActiveKeyProfile());
    if (!TestNotNull(TEXT("configured profile preserves explicit unbinds"),Profile)) return false;
    auto* Context=NewObject<UInputMappingContext>(); auto* Action=NewObject<UInputAction>();
    auto& Mapping=Context->MapKey(Action,EKeys::Gamepad_LeftShoulder);
    TestTrue(TEXT("mapping metadata configured"),MythicControlMapping::Configure(Mapping,Context,UMythicControlsSettings::MappingName(EMythicControl::AbilityLayer,true),FText::GetEmpty()));
    TestTrue(TEXT("registered authored modifier"),Settings->RegisterInputMappingContext(Context));
    FMapPlayerKeyArgs Args; Args.MappingName=Mapping.GetMappingName(); Args.ProfileIdString=Settings->GetActiveKeyProfileId(); Args.Slot=EPlayerMappableKeySlot::First; Args.NewKey=EKeys::Invalid; Args.bCreateMatchingSlotIfNeeded=false;
    FGameplayTagContainer Failures; Settings->MapPlayerKey(Args,Failures);
    TestTrue(TEXT("clear accepted"),Failures.IsEmpty());
    TArray<FKey> Keys; Profile->GetPlayerMappedKeysForRebuildControlMappings(Mapping,Keys);
    TestEqual(TEXT("rebuild receives one explicit sentinel"),Keys.Num(),1);
    TestTrue(TEXT("cleared gamepad key does not resurrect LB"),Keys.Num()==1 && !Keys[0].IsValid());
    Settings->ResetAllPlayerKeysInRow(Args,Failures); Keys.Reset(); Profile->GetPlayerMappedKeysForRebuildControlMappings(Mapping,Keys);
    TestTrue(TEXT("reset restores authored LB"),Keys.Num()==1 && Keys[0]==EKeys::Gamepad_LeftShoulder);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicFourSlotMigration,"Mythic.Controls.LegacyFourSlotMigration",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::ProductFilter)
bool FMythicFourSlotMigration::RunTest(const FString&) {
    const TArray<FSoftObjectPath> Original={FSoftObjectPath(TEXT("/Game/Test/Q.Q")),FSoftObjectPath(TEXT("/Game/Test/E.E")),FSoftObjectPath(TEXT("/Game/Test/R.R")),FSoftObjectPath(TEXT("/Game/Test/T.T"))};
    for (int32 OldCount:{0,1,2}) {
        auto* Save=NewObject<UMythicSaveGame>(); Save->CharacterData.DataVersion=static_cast<int32>(EMythicCharacterSaveVersion::PreCombatControls);
        Save->CharacterData.UnlockedSkillSlots=OldCount; Save->CharacterData.EquippedSkills=Original; Save->FixupData();
        TestEqual(TEXT("legacy socket availability is four"),Save->CharacterData.UnlockedSkillSlots,4);
        TestTrue(TEXT("legacy Q/E retain exact positions"),Save->CharacterData.EquippedSkills.Num()==4 && Save->CharacterData.EquippedSkills[0]==Original[0] && Save->CharacterData.EquippedSkills[1]==Original[1]);
        TestTrue(TEXT("legacy trailing garbage cannot equip R/T"),Save->CharacterData.EquippedSkills[2].IsNull() && Save->CharacterData.EquippedSkills[3].IsNull());
    }
    auto* Current=NewObject<UMythicSaveGame>(); Current->CharacterData.DataVersion=static_cast<int32>(CurrentCharacterSaveVersion); Current->CharacterData.EquippedSkills=Original; Current->FixupData();
    TestTrue(TEXT("current four assigned positions survive fixup"),Current->CharacterData.EquippedSkills==Original);
    Current->FixupData(); TestTrue(TEXT("migration is idempotent"),Current->CharacterData.EquippedSkills==Original);
    return true;
}
#endif

// Copyright Stellar Games. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "Settings/MythicCombatSettings.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicCoreAffixCentralBandTest,
                                 "Mythic.Itemization.CoreAffixScaling.CentralBand",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMythicCoreAffixCentralBandTest::RunTest(const FString &Parameters) {
    const UMythicCombatSettings *Settings = GetDefault<UMythicCombatSettings>();
    const FGameplayAttribute Sword = UMythicAttributeSet_Offense::GetBonusSwordDamageAttribute();

    const FMythicCoreAffixScaling *Row = Settings->CoreAffixScaling.Find(Sword);
    if (!TestNotNull(TEXT("Sword damage has a central row"), Row)) {
        return false;
    }

    // Migration safety: at level 1 the central band answers exactly what the shipped items authored by hand,
    // so converting an item to zeros changes no level-1 roll.
    const float CurveAt1 = MythicCombat::SampleOpenEnded(Settings->CoreAffixLevelCurve, 1.0f, Settings->CoreAffixTailGrowth);
    float Min = 0.0f;
    float Max = 0.0f;
    TestTrue(TEXT("Unauthored (zero) band resolves centrally"),
             MythicCombat::ResolveCoreAffixBand(Sword, 0.0f, 0.0f, 1.0f, Min, Max));
    TestTrue(TEXT("Level 1 min matches the hand-authored 0.10"), FMath::IsNearlyEqual(Min, 0.10f * CurveAt1, 0.001f));
    TestTrue(TEXT("Level 1 max matches the hand-authored 0.25"), FMath::IsNearlyEqual(Max, 0.25f * CurveAt1, 0.001f));

    // The level dependence is fully central: the same family at level 20 scales by the shared curve alone.
    const float CurveAt20 = MythicCombat::SampleOpenEnded(Settings->CoreAffixLevelCurve, 20.0f, Settings->CoreAffixTailGrowth);
    float Min20 = 0.0f;
    float Max20 = 0.0f;
    MythicCombat::ResolveCoreAffixBand(Sword, 0.0f, 0.0f, 20.0f, Min20, Max20);
    TestTrue(TEXT("Level 20 band is the base scaled by the shared curve"),
             FMath::IsNearlyEqual(Min20, 0.10f * CurveAt20, 0.001f) && FMath::IsNearlyEqual(Max20, 0.25f * CurveAt20, 0.001f));
    if (!Settings->CoreAffixLevelCurve.IsNull()) {
        TestTrue(TEXT("A level 20 drop is meaningfully stronger than a level 1 drop"), Min20 > Min * 2.0f);
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicCoreAffixIdentityTest,
                                 "Mythic.Itemization.CoreAffixScaling.AuthoredIdentity",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMythicCoreAffixIdentityTest::RunTest(const FString &Parameters) {
    const UMythicCombatSettings *Settings = GetDefault<UMythicCombatSettings>();
    const FGameplayAttribute Armor = UMythicAttributeSet_Defense::GetArmorAttribute();

    // A chestplate that authors 14-30 keeps that identity as its level-1 base; only the level scaling is shared.
    const float CurveAt10 = MythicCombat::SampleOpenEnded(Settings->CoreAffixLevelCurve, 10.0f, Settings->CoreAffixTailGrowth);
    float Min = 0.0f;
    float Max = 0.0f;
    TestTrue(TEXT("Armor resolves centrally"), MythicCombat::ResolveCoreAffixBand(Armor, 14.0f, 30.0f, 10.0f, Min, Max));
    TestTrue(TEXT("The authored band survives as the base"),
             FMath::IsNearlyEqual(Min, 14.0f * CurveAt10, 0.01f) && FMath::IsNearlyEqual(Max, 30.0f * CurveAt10, 0.01f));

    // An attribute with no central row keeps its authored path entirely.
    const FGameplayAttribute Uncovered = UMythicAttributeSet_Offense::GetCriticalHitDamageAttribute();
    TestFalse(TEXT("Uncovered attribute reports no central band"),
              MythicCombat::ResolveCoreAffixBand(Uncovered, 0.5f, 1.0f, 10.0f, Min, Max));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicCoreAffixCoverageTest,
                                 "Mythic.Itemization.CoreAffixScaling.FamilyCoverage",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMythicCoreAffixCoverageTest::RunTest(const FString &Parameters) {
    // Every core affix family the shipped 40 item definitions actually use has a central row, and every row is
    // a sane band. The denominator is printed so an empty map can never read as a clean pass.
    const UMythicCombatSettings *Settings = GetDefault<UMythicCombatSettings>();
    const int32 Families = Settings->CoreAffixScaling.Num();
    TestTrue(TEXT("All 14 surveyed core affix families are covered"), Families >= 14);
    AddInfo(FString::Printf(TEXT("%d central core affix families"), Families));

    for (const TPair<FGameplayAttribute, FMythicCoreAffixScaling> &Pair : Settings->CoreAffixScaling) {
        TestTrue(TEXT("Attribute valid"), Pair.Key.IsValid());
        TestTrue(TEXT("Band ordered and positive"),
                 Pair.Value.BaseMin > 0.0f && Pair.Value.BaseMax > Pair.Value.BaseMin);
    }
    TestTrue(TEXT("Tail growth authored above flat"), Settings->CoreAffixTailGrowth > 1.0f);
    return true;
}

#endif

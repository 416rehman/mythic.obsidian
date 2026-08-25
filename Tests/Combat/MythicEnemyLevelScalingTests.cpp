// Copyright Stellar Games. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Curves/SimpleCurve.h"
#include "Engine/CurveTable.h"
#include "Settings/MythicCombatSettings.h"

namespace {
// A throwaway level-keyed curve table row: value 1 at level 1 rising to 10 at level 20, last key at 20.
UCurveTable *MakeLevelTable(FName RowName) {
    UCurveTable *Table = NewObject<UCurveTable>(GetTransientPackage());
    FSimpleCurve &Row = Table->AddSimpleCurve(RowName);
    Row.AddKey(1.0f, 1.0f);
    Row.AddKey(10.0f, 4.0f);
    Row.AddKey(20.0f, 10.0f);
    return Table;
}

FCurveTableRowHandle MakeHandle(UCurveTable *Table, FName RowName) {
    FCurveTableRowHandle Handle;
    Handle.CurveTable = Table;
    Handle.RowName = RowName;
    return Handle;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicOpenEndedSampleTest,
                                 "Mythic.Combat.EnemyScaling.OpenEndedSample",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMythicOpenEndedSampleTest::RunTest(const FString &Parameters) {
    UCurveTable *Table = MakeLevelTable(TEXT("HP"));
    const FCurveTableRowHandle Handle = MakeHandle(Table, TEXT("HP"));

    // Inside the authored range the curve answers directly.
    TestEqual(TEXT("Level 1 reads the first key"), MythicCombat::SampleOpenEnded(Handle, 1.0f, 1.05f), 1.0f);
    TestEqual(TEXT("Level 20 reads the last key"), MythicCombat::SampleOpenEnded(Handle, 20.0f, 1.05f), 10.0f);

    // The tail is continuous at the last key and compounds beyond it.
    const float AtEdge = MythicCombat::SampleOpenEnded(Handle, 20.0f, 1.05f);
    const float OnePast = MythicCombat::SampleOpenEnded(Handle, 21.0f, 1.05f);
    TestTrue(TEXT("One level past the edge grows by exactly the tail rate"),
             FMath::IsNearlyEqual(OnePast, AtEdge * 1.05f, 0.001f));

    const float FarPast = MythicCombat::SampleOpenEnded(Handle, 60.0f, 1.05f);
    TestTrue(TEXT("Level 60 keeps compounding (open-ended, never flat)"),
             FMath::IsNearlyEqual(FarPast, 10.0f * FMath::Pow(1.05f, 40.0f), 0.01f));

    // A tail below 1 never shrinks the curve: growth is clamped to at-least-flat.
    TestTrue(TEXT("Tail below 1.0 clamps to flat, never decays"),
             FMath::IsNearlyEqual(MythicCombat::SampleOpenEnded(Handle, 30.0f, 0.5f), 10.0f, 0.001f));

    // Unset and empty handles read 1.0 so an unauthored curve scales nothing.
    TestEqual(TEXT("Null handle reads 1.0"), MythicCombat::SampleOpenEnded(FCurveTableRowHandle(), 15.0f, 1.05f), 1.0f);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicEnemyLevelBandsTest,
                                 "Mythic.Combat.EnemyScaling.DangerLevelBands",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMythicEnemyLevelBandsTest::RunTest(const FString &Parameters) {
    const UMythicCombatSettings *Settings = GetDefault<UMythicCombatSettings>();

    // Every danger tier has an authored level, and the ladder strictly climbs with danger.
    int32 Prev = 0;
    for (const EMythicDangerTier Tier : {EMythicDangerTier::Safe, EMythicDangerTier::Low, EMythicDangerTier::Moderate,
                                         EMythicDangerTier::High, EMythicDangerTier::Extreme}) {
        const int32 *Level = Settings->EnemyLevelByDangerTier.Find(Tier);
        if (!TestNotNull(TEXT("Danger tier has an authored level"), Level)) {
            return false;
        }
        TestTrue(TEXT("Level ladder strictly climbs with danger"), *Level > Prev);
        Prev = *Level;
    }

    TestTrue(TEXT("Health tail growth is authored above flat"), Settings->EnemyHealthTailGrowth > 1.0f);
    TestTrue(TEXT("Damage tail growth is authored above flat"), Settings->EnemyDamageTailGrowth > 1.0f);
    // Health must outgrow damage in the tail or high-level fights get one-shotty in both directions.
    TestTrue(TEXT("Health compounds at least as fast as damage"),
             Settings->EnemyHealthTailGrowth >= Settings->EnemyDamageTailGrowth);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicEnemyLevelRatioTest,
                                 "Mythic.Combat.EnemyScaling.EffectiveHpRatio",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMythicEnemyLevelRatioTest::RunTest(const FString &Parameters) {
    // The acceptance shape of #119, run through the exact sampler ApplyCombatScaling uses: a level 20 enemy must
    // carry meaningfully more health than a level 10 one, and the band's floor at 20 must clear the ceiling at 10
    // so the roll can never invert the ladder.
    UCurveTable *Table = MakeLevelTable(TEXT("HP"));
    const FCurveTableRowHandle Handle = MakeHandle(Table, TEXT("HP"));

    const float At10 = MythicCombat::SampleOpenEnded(Handle, 10.0f, 1.05f);
    const float At20 = MythicCombat::SampleOpenEnded(Handle, 20.0f, 1.05f);
    TestTrue(TEXT("Level 20 health multiplier is at least double level 10"), At20 >= At10 * 2.0f);

    // And the tail preserves the same relationship indefinitely: 10 levels always buys the same growth factor.
    const float At40 = MythicCombat::SampleOpenEnded(Handle, 40.0f, 1.05f);
    const float At50 = MythicCombat::SampleOpenEnded(Handle, 50.0f, 1.05f);
    TestTrue(TEXT("Ten tail levels buy a constant factor"),
             FMath::IsNearlyEqual(At50 / At40, FMath::Pow(1.05f, 10.0f), 0.01f));

    return true;
}

#endif

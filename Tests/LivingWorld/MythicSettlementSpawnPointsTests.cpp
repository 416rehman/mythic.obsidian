// Mythic Living World — Settlement Spawn-Point Tests
// Covers: AMythicSettlement::DerivePurpose (pure, deterministic spawn-point purpose derivation).
// Run via: Session Frontend → Automation → Mythic.LivingWorld.SpawnPoints

#include "Misc/AutomationTest.h"
#include "World/LivingWorld/Settlements/MythicSettlement.h"
#include "World/LivingWorld/LivingWorldTypes.h"

// ─────────────────────────────────────────────────────────────
// DerivePurpose — hostile camps are uniformly enemy-occupied
// ─────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSpawnPointHostileAllEnemyTest,
    "Mythic.LivingWorld.SpawnPoints.DerivePurpose.HostileAllEnemy",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSpawnPointHostileAllEnemyTest::RunTest(const FString &Parameters) {
    const FMythicCellCoord Cell(12, 7);
    for (int32 Index = 0; Index < 32; ++Index) {
        const EMythicSpawnPointPurpose Purpose = AMythicSettlement::DerivePurpose(Cell, Index, /*bHostile=*/true);
        TestEqual(TEXT("Hostile camp point is always Enemy"), Purpose, EMythicSpawnPointPurpose::Enemy);
    }
    return true;
}

// ─────────────────────────────────────────────────────────────
// DerivePurpose — peaceful settlements only produce Guard/Civilian
// ─────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSpawnPointPeacefulGuardCivilianTest,
    "Mythic.LivingWorld.SpawnPoints.DerivePurpose.PeacefulGuardCivilian",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSpawnPointPeacefulGuardCivilianTest::RunTest(const FString &Parameters) {
    int32 GuardCount = 0;
    int32 CivilianCount = 0;
    const int32 Samples = 64;

    // Sweep cells AND indices so the purpose roll is exercised across distinct seeds.
    for (int32 C = 0; C < Samples; ++C) {
        const FMythicCellCoord Cell(C % 11, C / 11);
        for (int32 Index = 0; Index < 4; ++Index) {
            const EMythicSpawnPointPurpose Purpose = AMythicSettlement::DerivePurpose(Cell, Index, /*bHostile=*/false);
            TestTrue(TEXT("Peaceful point is never Enemy"), Purpose != EMythicSpawnPointPurpose::Enemy);
            TestTrue(TEXT("Peaceful point is Guard or Civilian"),
                     Purpose == EMythicSpawnPointPurpose::Guard || Purpose == EMythicSpawnPointPurpose::Civilian);
            if (Purpose == EMythicSpawnPointPurpose::Guard) {
                ++GuardCount;
            }
            else {
                ++CivilianCount;
            }
        }
    }

    // Guards are a minority share (~25%); civilians dominate. We don't assert an exact ratio (it's a hash distribution),
    // only that BOTH kinds appear and guards are the minority — proving the roll isn't degenerate (all-one-kind).
    TestTrue(TEXT("At least one Guard point appears"), GuardCount > 0);
    TestTrue(TEXT("At least one Civilian point appears"), CivilianCount > 0);
    TestTrue(TEXT("Guards are the minority"), GuardCount < CivilianCount);

    return true;
}

// ─────────────────────────────────────────────────────────────
// DerivePurpose — deterministic: same inputs → same purpose
// ─────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSpawnPointDeterministicTest,
    "Mythic.LivingWorld.SpawnPoints.DerivePurpose.Deterministic",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSpawnPointDeterministicTest::RunTest(const FString &Parameters) {
    const FMythicCellCoord Cell(3, 99);
    for (int32 Index = 0; Index < 8; ++Index) {
        const EMythicSpawnPointPurpose First = AMythicSettlement::DerivePurpose(Cell, Index, /*bHostile=*/false);
        const EMythicSpawnPointPurpose Second = AMythicSettlement::DerivePurpose(Cell, Index, /*bHostile=*/false);
        TestEqual(TEXT("DerivePurpose is deterministic for identical inputs"), First, Second);
    }
    return true;
}

// ─────────────────────────────────────────────────────────────
// DerivePurpose — distinct indices in one cell get independent rolls (not all identical)
// ─────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSpawnPointPerIndexVarietyTest,
    "Mythic.LivingWorld.SpawnPoints.DerivePurpose.PerIndexVariety",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSpawnPointPerIndexVarietyTest::RunTest(const FString &Parameters) {
    // Across many cells, at least one cell must yield a Guard at SOME index while another index is Civilian — proving
    // the per-index salt actually varies the roll within a cell (no all-guard/all-civilian cells by construction).
    bool bFoundMixedCell = false;
    for (int32 C = 0; C < 256 && !bFoundMixedCell; ++C) {
        const FMythicCellCoord Cell(C, C * 2 + 1);
        bool bSawGuard = false;
        bool bSawCivilian = false;
        for (int32 Index = 0; Index < 8; ++Index) {
            const EMythicSpawnPointPurpose Purpose = AMythicSettlement::DerivePurpose(Cell, Index, /*bHostile=*/false);
            bSawGuard |= (Purpose == EMythicSpawnPointPurpose::Guard);
            bSawCivilian |= (Purpose == EMythicSpawnPointPurpose::Civilian);
        }
        bFoundMixedCell = bSawGuard && bSawCivilian;
    }
    TestTrue(TEXT("At least one cell yields both Guard and Civilian across its indices"), bFoundMixedCell);
    return true;
}

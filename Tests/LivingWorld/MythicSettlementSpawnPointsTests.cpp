
#include "Misc/AutomationTest.h"
#include "World/LivingWorld/Settlements/MythicSettlement.h"
#include "World/LivingWorld/LivingWorldTypes.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSpawnPointHostileAllEnemyTest,
    "Mythic.LivingWorld.SpawnPoints.DerivePurpose.HostileAllEnemy",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSpawnPointHostileAllEnemyTest::RunTest(const FString &Parameters) {
    const FMythicCellCoord Cell(12, 7);
    for (int32 Index = 0; Index < 32; ++Index) {
        const EMythicSpawnPointPurpose Purpose = AMythicSettlement::DerivePurpose(Cell, Index,true);
        TestEqual(TEXT("Hostile camp point is always Enemy"), Purpose, EMythicSpawnPointPurpose::Enemy);
    }
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSpawnPointPeacefulGuardCivilianTest,
    "Mythic.LivingWorld.SpawnPoints.DerivePurpose.PeacefulGuardCivilian",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSpawnPointPeacefulGuardCivilianTest::RunTest(const FString &Parameters) {
    int32 GuardCount = 0;
    int32 CivilianCount = 0;
    const int32 Samples = 64;

    for (int32 C = 0; C < Samples; ++C) {
        const FMythicCellCoord Cell(C % 11, C / 11);
        for (int32 Index = 0; Index < 4; ++Index) {
            const EMythicSpawnPointPurpose Purpose = AMythicSettlement::DerivePurpose(Cell, Index,false);
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

    TestTrue(TEXT("At least one Guard point appears"), GuardCount > 0);
    TestTrue(TEXT("At least one Civilian point appears"), CivilianCount > 0);
    TestTrue(TEXT("Guards are the minority"), GuardCount < CivilianCount);

    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSpawnPointDeterministicTest,
    "Mythic.LivingWorld.SpawnPoints.DerivePurpose.Deterministic",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSpawnPointDeterministicTest::RunTest(const FString &Parameters) {
    const FMythicCellCoord Cell(3, 99);
    for (int32 Index = 0; Index < 8; ++Index) {
        const EMythicSpawnPointPurpose First = AMythicSettlement::DerivePurpose(Cell, Index,false);
        const EMythicSpawnPointPurpose Second = AMythicSettlement::DerivePurpose(Cell, Index,false);
        TestEqual(TEXT("DerivePurpose is deterministic for identical inputs"), First, Second);
    }
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSpawnPointPerIndexVarietyTest,
    "Mythic.LivingWorld.SpawnPoints.DerivePurpose.PerIndexVariety",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSpawnPointPerIndexVarietyTest::RunTest(const FString &Parameters) {
    bool bFoundMixedCell = false;
    for (int32 C = 0; C < 256 && !bFoundMixedCell; ++C) {
        const FMythicCellCoord Cell(C, C * 2 + 1);
        bool bSawGuard = false;
        bool bSawCivilian = false;
        for (int32 Index = 0; Index < 8; ++Index) {
            const EMythicSpawnPointPurpose Purpose = AMythicSettlement::DerivePurpose(Cell, Index,false);
            bSawGuard |= (Purpose == EMythicSpawnPointPurpose::Guard);
            bSawCivilian |= (Purpose == EMythicSpawnPointPurpose::Civilian);
        }
        bFoundMixedCell = bSawGuard && bSawCivilian;
    }
    TestTrue(TEXT("At least one cell yields both Guard and Civilian across its indices"), bFoundMixedCell);
    return true;
}

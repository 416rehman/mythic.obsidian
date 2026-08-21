#include "Misc/AutomationTest.h"
#include "GAS/Mounts/MythicMountTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicMountTest,
    "Mythic.World.Mounts",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicMountTest::RunTest(const FString &Parameters) {
    namespace S = MythicMountStatics;

    const float RangeSq = 350.0f * 350.0f;
    const float NearSq = 100.0f * 100.0f;
    const float FarSq = 400.0f * 400.0f;

    TestEqual(TEXT("all gates pass → Ok"),
              S::CanMount(true, false, false, true, NearSq, RangeSq), EMountGateResult::Ok);
    TestEqual(TEXT("dead mount → Dead"),
              S::CanMount(false, false, false, true, NearSq, RangeSq), EMountGateResult::Dead);
    TestEqual(TEXT("rider already mounted → AlreadyMounted"),
              S::CanMount(true, true, false, true, NearSq, RangeSq), EMountGateResult::AlreadyMounted);
    TestEqual(TEXT("rider in combat → InCombat"),
              S::CanMount(true, false, true, true, NearSq, RangeSq), EMountGateResult::InCombat);
    TestEqual(TEXT("not the owner → NotOwner"),
              S::CanMount(true, false, false, false, NearSq, RangeSq), EMountGateResult::NotOwner);
    TestEqual(TEXT("too far → OutOfRange"),
              S::CanMount(true, false, false, true, FarSq, RangeSq), EMountGateResult::OutOfRange);

    TestEqual(TEXT("exactly at range → Ok (inclusive)"),
              S::CanMount(true, false, false, true, RangeSq, RangeSq), EMountGateResult::Ok);
    TestEqual(TEXT("RangeSq <= 0 disables the range gate"),
              S::CanMount(true, false, false, true, FarSq, 0.0f), EMountGateResult::Ok);

    TestEqual(TEXT("dead + not-owner → Dead wins"),
              S::CanMount(false, true, true, false, FarSq, RangeSq), EMountGateResult::Dead);
    TestEqual(TEXT("not-owner + mounted + combat + far → NotOwner wins"),
              S::CanMount(true, true, true, false, FarSq, RangeSq), EMountGateResult::NotOwner);
    TestEqual(TEXT("mounted + combat + far → AlreadyMounted wins"),
              S::CanMount(true, true, true, true, FarSq, RangeSq), EMountGateResult::AlreadyMounted);
    TestEqual(TEXT("combat + far → InCombat wins"),
              S::CanMount(true, false, true, true, FarSq, RangeSq), EMountGateResult::InCombat);

    TestFalse(TEXT("no active mount → cannot summon"), S::CanSummon(false, false, 999.0, 30.0));
    TestFalse(TEXT("in combat → cannot summon"), S::CanSummon(true, true, 999.0, 30.0));
    TestFalse(TEXT("inside cooldown → cannot summon"), S::CanSummon(true, false, 29.9, 30.0));
    TestTrue(TEXT("cooldown boundary (== cooldown) → may summon (inclusive)"), S::CanSummon(true, false, 30.0, 30.0));
    TestTrue(TEXT("past cooldown → may summon"), S::CanSummon(true, false, 30.1, 30.0));
    TestTrue(TEXT("cooldown <= 0 disables the cooldown gate"), S::CanSummon(true, false, 0.0, 0.0));
    TestFalse(TEXT("combat blocks even with cooldown disabled"), S::CanSummon(true, true, 0.0, 0.0));

    for (int32 L = 0; L < S::MaxBondLevel; ++L) {
        TestTrue(FString::Printf(TEXT("BondXPForLevel strictly increases at L=%d"), L),
                 S::BondXPForLevel(L + 1) > S::BondXPForLevel(L));
    }
    for (int32 L = 0; L <= S::MaxBondLevel; ++L) {
        TestEqual(FString::Printf(TEXT("round-trip BondLevelFromXP(BondXPForLevel(%d)) == %d"), L, L),
                  S::BondLevelFromXP(S::BondXPForLevel(L)), L);
    }
    TestEqual(TEXT("one XP short of a level stays the level below"),
              S::BondLevelFromXP(S::BondXPForLevel(3) - 1), 2);
    TestEqual(TEXT("zero XP → level 0"), S::BondLevelFromXP(0), 0);
    TestEqual(TEXT("negative XP clamps to level 0"), S::BondLevelFromXP(-500), 0);
    TestEqual(TEXT("huge XP caps at MaxBondLevel"), S::BondLevelFromXP(50000000), S::MaxBondLevel);
    {
        int32 Prev = S::BondLevelFromXP(0);
        for (int32 XP = 0; XP <= S::BondXPForLevel(S::MaxBondLevel) + 500; XP += 37) {
            const int32 Cur = S::BondLevelFromXP(XP);
            TestTrue(TEXT("BondLevelFromXP monotonic non-decreasing"), Cur >= Prev);
            Prev = Cur;
        }
    }

    const float Base = 1100.0f;
    for (int32 L = 0; L < S::MaxBondLevel; ++L) {
        TestTrue(FString::Printf(TEXT("gallop speed monotonic non-decreasing in bond at L=%d"), L),
                 S::ComputeGallopSpeed(Base, L + 1, 1.0f) >= S::ComputeGallopSpeed(Base, L, 1.0f));
    }
    TestTrue(TEXT("higher bond is strictly faster (full stamina)"),
             S::ComputeGallopSpeed(Base, 5, 1.0f) > S::ComputeGallopSpeed(Base, 0, 1.0f));
    TestEqual(TEXT("zero stamina gates the bond bonus away (falls back to Base)"),
              S::ComputeGallopSpeed(Base, S::MaxBondLevel, 0.0f), Base);
    TestTrue(TEXT("any stamina >= gated (exhausted) speed"),
             S::ComputeGallopSpeed(Base, 5, 0.5f) >= S::ComputeGallopSpeed(Base, 5, 0.0f));
    TestEqual(TEXT("bond level 0 at full stamina == Base (no bonus to gain)"),
              S::ComputeGallopSpeed(Base, 0, 1.0f), Base);
    TestEqual(TEXT("non-positive base → 0"), S::ComputeGallopSpeed(0.0f, 5, 1.0f), 0.0f);
    TestEqual(TEXT("bond above the cap clamps to the cap"),
              S::ComputeGallopSpeed(Base, S::MaxBondLevel + 50, 1.0f), S::ComputeGallopSpeed(Base, S::MaxBondLevel, 1.0f));

    const float Max = 100.0f;
    TestEqual(TEXT("drain: simple tick math"), S::DrainStamina(50.0f, 0.25f, 12.0f, Max), 50.0f - 3.0f);
    TestEqual(TEXT("drain clamps at 0 (never negative)"), S::DrainStamina(1.0f, 1.0f, 50.0f, Max), 0.0f);
    TestEqual(TEXT("drain from 0 stays 0"), S::DrainStamina(0.0f, 1.0f, 50.0f, Max), 0.0f);
    TestEqual(TEXT("regen: simple tick math"), S::RegenStamina(50.0f, 0.25f, 18.0f, Max), 50.0f + 4.5f);
    TestEqual(TEXT("regen clamps at Max (never overfills)"), S::RegenStamina(99.0f, 1.0f, 50.0f, Max), Max);
    TestEqual(TEXT("regen from Max stays Max"), S::RegenStamina(Max, 1.0f, 50.0f, Max), Max);
    TestEqual(TEXT("negative delta treated as zero (no free gain via drain)"), S::DrainStamina(50.0f, -1.0f, 12.0f, Max), 50.0f);
    TestEqual(TEXT("negative rate treated as zero (no drain via regen)"), S::RegenStamina(50.0f, 1.0f, -12.0f, Max), 50.0f);
    TestEqual(TEXT("over-Max current clamps down to Max on next tick"), S::RegenStamina(150.0f, 0.0f, 0.0f, Max), Max);

    return true;
}

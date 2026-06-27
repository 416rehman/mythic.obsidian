// Mythic Living World — Activity Catalog Tests
// Covers the pure, deterministic helpers of the context-driven activity catalog (Step 3):
//   - MythicActivityDefaults::BuildDefaultActivities (well-formed, valid tags, positive weights, one always-eligible)
//   - MythicActivityDefaults::ActivityEligible (role / biome / water / time / phase / merchant gates)
//   - MythicActivityDefaults::PickActivityIndex (eligible-only weighted pick, deterministic, INDEX_NONE on none)
//   - AMythicAIController::IsDayHour (boundary [6,20))
// Run via: Session Frontend → Automation → Mythic.LivingWorld.Activities

#include "Misc/AutomationTest.h"
#include "World/LivingWorld/Activities/ActivityTypes.h"
#include "World/LivingWorld/MythicTags_LivingWorld.h"
#include "World/LivingWorld/Territory/MythicBiome.h"
#include "Mass/Fragments/MythicMassFragments.h" // EMythicSchedulePhase
#include "AI/NPCs/MythicAIController.h"          // IsDayHour

namespace {
    // A baseline daytime, Work-phase, plains, no-merchant context with a well-distributed NameHash.
    FMythicActivityContext MakeBaseContext() {
        FMythicActivityContext Ctx;
        Ctx.Role = FGameplayTag();
        Ctx.Biome = EMythicBiome::Plains;
        Ctx.bIsDay = true;
        Ctx.Phase = EMythicSchedulePhase::Work;
        Ctx.bHasNearbyMerchant = false;
        Ctx.NameHash = 0x12345678u;
        return Ctx;
    }
} // namespace

// ─────────────────────────────────────────────────────────────
// Code defaults are well-formed (+ at least one always-eligible)
// ─────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicActivityDefaultsWellFormedTest,
    "Mythic.LivingWorld.Activities.Defaults.WellFormed",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicActivityDefaultsWellFormedTest::RunTest(const FString &Parameters) {
    TArray<FMythicActivityDef> Activities;
    MythicActivityDefaults::BuildDefaultActivities(Activities);

    TestTrue(TEXT("Code defaults produce at least one activity"), Activities.Num() > 0);

    for (const FMythicActivityDef &A : Activities) {
        TestTrue(TEXT("Activity has a valid tag"), A.ActivityTag.IsValid());
        TestTrue(TEXT("Activity weight is positive"), A.RelativeWeight > 0.0f);
    }

    // There must be at least one always-eligible activity (no role/biome/water/merchant gate, Any time, Any phase) so a
    // selection NEVER starves regardless of context — the Wander fallback. Verify by checking it survives a wildly
    // restrictive context (night, no merchant, a role no default activity targets, a desert biome).
    FMythicActivityContext Hostile;
    Hostile.Role = TAG_NPC_ROLE_BEGGAR; // no default activity gates on beggar
    Hostile.Biome = EMythicBiome::Desert;
    Hostile.bIsDay = false;
    Hostile.Phase = EMythicSchedulePhase::Travel; // no phase-gated activity satisfies Travel
    Hostile.bHasNearbyMerchant = false;
    Hostile.NameHash = 99u;

    int32 EligibleCount = 0;
    for (const FMythicActivityDef &A : Activities) {
        if (MythicActivityDefaults::ActivityEligible(A, Hostile)) {
            ++EligibleCount;
        }
    }
    TestTrue(TEXT("At least one always-eligible activity survives a maximally restrictive context"), EligibleCount >= 1);

    // And the picker must still succeed for that context.
    const int32 Picked = MythicActivityDefaults::PickActivityIndex(Activities, Hostile);
    TestTrue(TEXT("PickActivityIndex succeeds even in a maximally restrictive context"), Activities.IsValidIndex(Picked));
    return true;
}

// ─────────────────────────────────────────────────────────────
// ActivityEligible — each gate rejects independently
// ─────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicActivityEligibilityTest,
    "Mythic.LivingWorld.Activities.Eligible.Gates",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicActivityEligibilityTest::RunTest(const FString &Parameters) {
    // Role gate.
    {
        FMythicActivityDef GuardOnly;
        GuardOnly.ActivityTag = TAG_NPC_ACTIVITY_PATROL;
        GuardOnly.EligibleRoles.AddTag(TAG_NPC_ROLE_GUARD);

        FMythicActivityContext Ctx = MakeBaseContext();
        Ctx.Role = TAG_NPC_ROLE_GUARD;
        TestTrue(TEXT("Guard role passes a guard-only activity"), MythicActivityDefaults::ActivityEligible(GuardOnly, Ctx));

        Ctx.Role = TAG_NPC_ROLE_FARMER;
        TestFalse(TEXT("Farmer role fails a guard-only activity"), MythicActivityDefaults::ActivityEligible(GuardOnly, Ctx));

        // Empty role on the def => any role.
        FMythicActivityDef AnyRole;
        AnyRole.ActivityTag = TAG_NPC_ACTIVITY_WANDER;
        Ctx.Role = TAG_NPC_ROLE_FARMER;
        TestTrue(TEXT("Empty EligibleRoles accepts any role"), MythicActivityDefaults::ActivityEligible(AnyRole, Ctx));
    }

    // Biome gate.
    {
        FMythicActivityDef ForestOnly;
        ForestOnly.ActivityTag = TAG_NPC_ACTIVITY_WANDER;
        ForestOnly.EligibleBiomes.Add(EMythicBiome::Forest);

        FMythicActivityContext Ctx = MakeBaseContext();
        Ctx.Biome = EMythicBiome::Forest;
        TestTrue(TEXT("Forest biome passes a forest-only activity"), MythicActivityDefaults::ActivityEligible(ForestOnly, Ctx));
        Ctx.Biome = EMythicBiome::Plains;
        TestFalse(TEXT("Plains biome fails a forest-only activity"), MythicActivityDefaults::ActivityEligible(ForestOnly, Ctx));
    }

    // Water-adjacency gate (v1 stub: Wetland).
    {
        FMythicActivityDef Fish;
        Fish.ActivityTag = TAG_NPC_ACTIVITY_FISH;
        Fish.bRequiresWaterAdjacent = 1;

        FMythicActivityContext Ctx = MakeBaseContext();
        Ctx.Biome = EMythicBiome::Wetland;
        TestTrue(TEXT("Wetland satisfies water-adjacency"), MythicActivityDefaults::ActivityEligible(Fish, Ctx));
        Ctx.Biome = EMythicBiome::Plains;
        TestFalse(TEXT("Plains fails water-adjacency"), MythicActivityDefaults::ActivityEligible(Fish, Ctx));
    }

    // Time gate.
    {
        FMythicActivityDef DayOnly;
        DayOnly.ActivityTag = TAG_NPC_ACTIVITY_WORK;
        DayOnly.TimeWindow = EMythicActivityTimeWindow::Day;

        FMythicActivityContext Ctx = MakeBaseContext();
        Ctx.bIsDay = true;
        TestTrue(TEXT("Day passes a day-only activity"), MythicActivityDefaults::ActivityEligible(DayOnly, Ctx));
        Ctx.bIsDay = false;
        TestFalse(TEXT("Night fails a day-only activity"), MythicActivityDefaults::ActivityEligible(DayOnly, Ctx));

        FMythicActivityDef NightOnly;
        NightOnly.ActivityTag = TAG_NPC_ACTIVITY_REST;
        NightOnly.TimeWindow = EMythicActivityTimeWindow::Night;
        Ctx.bIsDay = false;
        TestTrue(TEXT("Night passes a night-only activity"), MythicActivityDefaults::ActivityEligible(NightOnly, Ctx));
        Ctx.bIsDay = true;
        TestFalse(TEXT("Day fails a night-only activity"), MythicActivityDefaults::ActivityEligible(NightOnly, Ctx));
    }

    // Phase gate.
    {
        FMythicActivityDef WorkPhase;
        WorkPhase.ActivityTag = TAG_NPC_ACTIVITY_WORK;
        WorkPhase.RequiredPhase = EMythicActivitySchedulePhase::Work;

        FMythicActivityContext Ctx = MakeBaseContext();
        Ctx.Phase = EMythicSchedulePhase::Work;
        TestTrue(TEXT("Work phase passes a Work-gated activity"), MythicActivityDefaults::ActivityEligible(WorkPhase, Ctx));
        Ctx.Phase = EMythicSchedulePhase::Rest;
        TestFalse(TEXT("Rest phase fails a Work-gated activity"), MythicActivityDefaults::ActivityEligible(WorkPhase, Ctx));
        Ctx.Phase = EMythicSchedulePhase::Travel;
        TestFalse(TEXT("Travel phase fails a Work-gated activity"), MythicActivityDefaults::ActivityEligible(WorkPhase, Ctx));

        // Any phase accepts every actual phase (incl. Travel/Idle).
        FMythicActivityDef AnyPhase;
        AnyPhase.ActivityTag = TAG_NPC_ACTIVITY_WANDER;
        Ctx.Phase = EMythicSchedulePhase::Travel;
        TestTrue(TEXT("Any phase accepts Travel"), MythicActivityDefaults::ActivityEligible(AnyPhase, Ctx));
        Ctx.Phase = EMythicSchedulePhase::Idle;
        TestTrue(TEXT("Any phase accepts Idle"), MythicActivityDefaults::ActivityEligible(AnyPhase, Ctx));
    }

    // Merchant gate.
    {
        FMythicActivityDef Barter;
        Barter.ActivityTag = TAG_NPC_ACTIVITY_BARTER;
        Barter.bRequiresNearbyMerchant = 1;

        FMythicActivityContext Ctx = MakeBaseContext();
        Ctx.bHasNearbyMerchant = true;
        TestTrue(TEXT("Nearby merchant passes a barter activity"), MythicActivityDefaults::ActivityEligible(Barter, Ctx));
        Ctx.bHasNearbyMerchant = false;
        TestFalse(TEXT("No merchant fails a barter activity"), MythicActivityDefaults::ActivityEligible(Barter, Ctx));
    }

    // Zero/negative-weight def is never eligible.
    {
        FMythicActivityDef Disabled;
        Disabled.ActivityTag = TAG_NPC_ACTIVITY_WANDER;
        Disabled.RelativeWeight = 0.0f;
        TestFalse(TEXT("Zero-weight activity is ineligible"),
                  MythicActivityDefaults::ActivityEligible(Disabled, MakeBaseContext()));
    }
    return true;
}

// ─────────────────────────────────────────────────────────────
// PickActivityIndex — eligible-only, deterministic, INDEX_NONE on none
// ─────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicActivityPickTest,
    "Mythic.LivingWorld.Activities.Pick.EligibleDeterministic",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicActivityPickTest::RunTest(const FString &Parameters) {
    TArray<FMythicActivityDef> Activities;
    MythicActivityDefaults::BuildDefaultActivities(Activities);

    // Deterministic: same context → same index.
    {
        FMythicActivityContext Ctx = MakeBaseContext();
        const int32 A = MythicActivityDefaults::PickActivityIndex(Activities, Ctx);
        const int32 B = MythicActivityDefaults::PickActivityIndex(Activities, Ctx);
        TestEqual(TEXT("PickActivityIndex is deterministic for a fixed context"), A, B);
    }

    // The picker only ever returns an ELIGIBLE index. Sweep many NameHashes in a fisher/wetland/day/work context and
    // confirm every chosen def passes ActivityEligible, and that the weighted pick reaches BOTH Fish and Wander.
    {
        FMythicActivityContext Ctx;
        Ctx.Role = TAG_NPC_ROLE_FISHER;
        Ctx.Biome = EMythicBiome::Wetland;
        Ctx.bIsDay = true;
        Ctx.Phase = EMythicSchedulePhase::Work;
        Ctx.bHasNearbyMerchant = false;

        bool bSawFish = false;
        bool bSawWander = false;
        for (uint32 i = 0; i < 256; ++i) {
            Ctx.NameHash = (i + 1) * 2654435761u; // spread the seed space (mirror the group test idiom)
            const int32 Idx = MythicActivityDefaults::PickActivityIndex(Activities, Ctx);
            TestTrue(TEXT("Picked index is valid"), Activities.IsValidIndex(Idx));
            if (Activities.IsValidIndex(Idx)) {
                TestTrue(TEXT("Picked activity is eligible"),
                         MythicActivityDefaults::ActivityEligible(Activities[Idx], Ctx));
                bSawFish |= (Activities[Idx].ActivityTag == TAG_NPC_ACTIVITY_FISH);
                bSawWander |= (Activities[Idx].ActivityTag == TAG_NPC_ACTIVITY_WANDER);
            }
        }
        TestTrue(TEXT("Weighted pick reaches Fish over the seed sweep"), bSawFish);
        TestTrue(TEXT("Weighted pick reaches the Wander fallback over the seed sweep"), bSawWander);
    }

    // INDEX_NONE on an empty array.
    {
        TArray<FMythicActivityDef> Empty;
        const int32 Idx = MythicActivityDefaults::PickActivityIndex(Empty, MakeBaseContext());
        TestEqual(TEXT("Empty activity array yields INDEX_NONE"), Idx, (int32)INDEX_NONE);
    }

    // INDEX_NONE when nothing is eligible (a single role-gated def + a mismatched role).
    {
        TArray<FMythicActivityDef> OneGated;
        FMythicActivityDef GuardOnly;
        GuardOnly.ActivityTag = TAG_NPC_ACTIVITY_PATROL;
        GuardOnly.EligibleRoles.AddTag(TAG_NPC_ROLE_GUARD);
        OneGated.Add(GuardOnly);

        FMythicActivityContext Ctx = MakeBaseContext();
        Ctx.Role = TAG_NPC_ROLE_FARMER; // not a guard
        const int32 Idx = MythicActivityDefaults::PickActivityIndex(OneGated, Ctx);
        TestEqual(TEXT("No-eligible-activity context yields INDEX_NONE"), Idx, (int32)INDEX_NONE);
    }
    return true;
}

// ─────────────────────────────────────────────────────────────
// IsDayHour — boundary [6,20)
// ─────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicActivityIsDayHourTest,
    "Mythic.LivingWorld.Activities.IsDayHour.Boundaries",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicActivityIsDayHourTest::RunTest(const FString &Parameters) {
    TestFalse(TEXT("Midnight is night"), AMythicAIController::IsDayHour(0.0f));
    TestFalse(TEXT("5:59 is night"), AMythicAIController::IsDayHour(5.99f));
    TestTrue(TEXT("6:00 is day (inclusive start)"), AMythicAIController::IsDayHour(6.0f));
    TestTrue(TEXT("Noon is day"), AMythicAIController::IsDayHour(12.0f));
    TestTrue(TEXT("19:59 is day"), AMythicAIController::IsDayHour(19.99f));
    TestFalse(TEXT("20:00 is night (exclusive end)"), AMythicAIController::IsDayHour(20.0f));
    TestFalse(TEXT("23:00 is night"), AMythicAIController::IsDayHour(23.0f));
    return true;
}

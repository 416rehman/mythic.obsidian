
#include "Misc/AutomationTest.h"
#include "World/LivingWorld/Bounty/MythicBountyRules.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicBountyTierTest,
    "Mythic.World.Bounty.ResolveTier",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicBountyTierTest::RunTest(const FString &Parameters) {
    using namespace MythicBounty;

    const TArray<float> Thresholds = {150.0f, 300.0f, 500.0f};

    TestEqual(TEXT("empty thresholds → -1"), ResolveBountyTier(1.0e9f, TArray<float>()), -1);
    TestEqual(TEXT("zero notoriety → -1"), ResolveBountyTier(0.0f, Thresholds), -1);
    TestEqual(TEXT("just below first threshold → -1"), ResolveBountyTier(149.99f, Thresholds), -1);
    TestEqual(TEXT("negative notoriety → -1"), ResolveBountyTier(-50.0f, Thresholds), -1);

    TestEqual(TEXT("exactly at first threshold → tier 0"), ResolveBountyTier(150.0f, Thresholds), 0);
    TestEqual(TEXT("exactly at second threshold → tier 1"), ResolveBountyTier(300.0f, Thresholds), 1);
    TestEqual(TEXT("exactly at top threshold → tier 2"), ResolveBountyTier(500.0f, Thresholds), 2);

    TestEqual(TEXT("between tiers 0 and 1 → tier 0"), ResolveBountyTier(299.0f, Thresholds), 0);
    TestEqual(TEXT("between tiers 1 and 2 → tier 1"), ResolveBountyTier(499.0f, Thresholds), 1);
    TestEqual(TEXT("far above top → top tier"), ResolveBountyTier(1.0e6f, Thresholds), 2);

    {
        int32 Prev = -1;
        for (float N = 0.0f; N <= 600.0f; N += 10.0f) {
            const int32 Tier = ResolveBountyTier(N, Thresholds);
            TestTrue(TEXT("tier never decreases as notoriety rises"), Tier >= Prev);
            Prev = Tier;
        }
    }

    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicBountyDispatchGateTest,
    "Mythic.World.Bounty.DispatchGates",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicBountyDispatchGateTest::RunTest(const FString &Parameters) {
    using namespace MythicBounty;

    TestTrue(TEXT("all gates pass → dispatch"),
             ShouldDispatchHunters( 0, 1000.0, 900.0, 0, 4,
 0.10f, 0.15f));

    TestFalse(TEXT("tier -1 → never"),
              ShouldDispatchHunters(-1, 1000.0, 900.0, 0, 4, 0.0f, 1.0f));

    TestFalse(TEXT("inside cooldown → no"),
              ShouldDispatchHunters(0, 899.0, 900.0, 0, 4, 0.0f, 1.0f));
    TestTrue(TEXT("exactly at cooldown boundary → yes"),
             ShouldDispatchHunters(0, 900.0, 900.0, 0, 4, 0.0f, 1.0f));

    TestFalse(TEXT("at hunter cap → no"),
              ShouldDispatchHunters(2, 1000.0, 900.0, 4, 4, 0.0f, 1.0f));
    TestFalse(TEXT("over hunter cap → no"),
              ShouldDispatchHunters(2, 1000.0, 900.0, 9, 4, 0.0f, 1.0f));
    TestTrue(TEXT("one under cap → yes"),
             ShouldDispatchHunters(2, 1000.0, 900.0, 3, 4, 0.0f, 1.0f));

    TestFalse(TEXT("chance 0 never fires (even roll 0)"),
              ShouldDispatchHunters(0, 1000.0, 900.0, 0, 4, 0.0f, 0.0f));
    TestTrue(TEXT("chance 1 always fires (even roll 1)"),
             ShouldDispatchHunters(0, 1000.0, 900.0, 0, 4, 1.0f, 1.0f));
    TestFalse(TEXT("roll above chance → no"),
              ShouldDispatchHunters(0, 1000.0, 900.0, 0, 4, 0.16f, 0.15f));
    TestTrue(TEXT("roll at chance → yes (inclusive)"),
             ShouldDispatchHunters(0, 1000.0, 900.0, 0, 4, 0.15f, 0.15f));

    for (int32 i = 0; i < 8; ++i) {
        TestTrue(TEXT("deterministic under repetition"),
                 ShouldDispatchHunters(1, 5000.0, 900.0, 1, 4, 0.05f, 0.15f));
    }

    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicBountyHunterCountTest,
    "Mythic.World.Bounty.HunterCount",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicBountyHunterCountTest::RunTest(const FString &Parameters) {
    using namespace MythicBounty;

    TestEqual(TEXT("tier -1 → 0 hunters"), HunterCountForTier(-1, 1, 1, 4), 0);

    TestEqual(TEXT("tier 0 → base pack"), HunterCountForTier(0, 1, 1, 4), 1);
    TestEqual(TEXT("tier 1 → base+1"), HunterCountForTier(1, 1, 1, 4), 2);
    TestEqual(TEXT("tier 2 → base+2"), HunterCountForTier(2, 1, 1, 4), 3);
    TestEqual(TEXT("huge tier clamps to the cap"), HunterCountForTier(50, 1, 1, 4), 4);

    {
        int32 Prev = 0;
        for (int32 Tier = -1; Tier <= 20; ++Tier) {
            const int32 Count = HunterCountForTier(Tier, 2, 1, 6);
            TestTrue(TEXT("pack size never shrinks as the tier rises"), Count >= Prev);
            Prev = Count;
        }
    }

    TestEqual(TEXT("zero base still sends one hunter"), HunterCountForTier(0, 0, 1, 4), 1);
    TestEqual(TEXT("negative per-tier treated as 0"), HunterCountForTier(3, 2, -5, 8), 2);
    TestEqual(TEXT("cap floors at 1"), HunterCountForTier(5, 3, 2, 0), 1);

    return true;
}

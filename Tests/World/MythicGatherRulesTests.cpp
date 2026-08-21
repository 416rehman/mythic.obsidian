
#include "Misc/AutomationTest.h"
#include "GAS/MythicTags_GAS.h"
#include "World/Gathering/MythicGatherRules.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicGatherRulesTest,
    "Mythic.World.GatherRules",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicGatherRulesTest::RunTest(const FString &Parameters) {
    using Rules = FMythicGatherRules;

    {
        const FGameplayTag RequiredTool = GAS_STATE_SPRINTING;
        const FGameplayTag OtherTool = GAS_STATE_DOWNED;

        FGameplayTagContainer MatchingProbe;
        MatchingProbe.AddTag(RequiredTool);

        FGameplayTagContainer EmptyProbe;
        TestTrue(TEXT("empty required tag: empty probe still gathers"), Rules::CanGather(EmptyProbe, FGameplayTag()));
        TestTrue(TEXT("empty required tag: any probe gathers"), Rules::CanGather(MatchingProbe, FGameplayTag()));

        TestTrue(TEXT("matching tool gathers"), Rules::CanGather(MatchingProbe, RequiredTool));

        FGameplayTagContainer WrongProbe;
        WrongProbe.AddTag(OtherTool);
        TestFalse(TEXT("wrong tool does not gather"), Rules::CanGather(WrongProbe, RequiredTool));
        TestFalse(TEXT("no tool does not gather a gated node"), Rules::CanGather(EmptyProbe, RequiredTool));
    }

    {
        TestEqual(TEXT("tier 0 yield is 1.0 (unchanged)"), Rules::TierYieldMultiplier(0), 1.0f);
        TestEqual(TEXT("negative tier clamps to tier 0"), Rules::TierYieldMultiplier(-5), 1.0f);

        float Prev = Rules::TierYieldMultiplier(0);
        for (int32 Tier = 1; Tier <= 10; ++Tier) {
            const float Cur = Rules::TierYieldMultiplier(Tier);
            TestTrue(TEXT("yield multiplier is monotonic non-decreasing"), Cur >= Prev);
            TestTrue(TEXT("yield multiplier is always >= 1"), Cur >= 1.0f);
            Prev = Cur;
        }
        TestTrue(TEXT("tier 2 out-yields tier 0"), Rules::TierYieldMultiplier(2) > Rules::TierYieldMultiplier(0));
    }

    {
        const float Base = 300.0f;
        TestEqual(TEXT("tier 0 respawn delay is unchanged"), Rules::ScaledRespawnDelay(Base, 0), Base);
        TestEqual(TEXT("negative tier clamps to tier 0"), Rules::ScaledRespawnDelay(Base, -3), Base);

        TestEqual(TEXT("zero base delay stays zero"), Rules::ScaledRespawnDelay(0.0f, 5), 0.0f);

        float Prev = Rules::ScaledRespawnDelay(Base, 0);
        for (int32 Tier = 1; Tier <= 10; ++Tier) {
            const float Cur = Rules::ScaledRespawnDelay(Base, Tier);
            TestTrue(TEXT("respawn delay is monotonic non-decreasing"), Cur >= Prev);
            TestTrue(TEXT("respawn delay is always >= base"), Cur >= Base);
            Prev = Cur;
        }
        TestTrue(TEXT("tier 2 respawns slower than tier 0"), Rules::ScaledRespawnDelay(Base, 2) > Rules::ScaledRespawnDelay(Base, 0));
    }

    return true;
}


#include "Misc/AutomationTest.h"
#include "World/LivingWorld/Vendetta/MythicVendettaTypes.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicVendettaThreatMathTest,
    "Mythic.LivingWorld.Vendetta.ThreatMath",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicVendettaThreatMathTest::RunTest(const FString &Parameters) {
    using namespace MythicVendetta;

    {
        float T = 0.0f;
        float Prev = T;
        for (int32 i = 0; i < 20; ++i) {
            T = AccumulatePlayerThreat(T, 25.0f, 1.0f);
            TestTrue(TEXT("accumulation never decreases"), T >= Prev - KINDA_SMALL_NUMBER);
            Prev = T;
        }
        TestTrue(TEXT("accumulating positive deeds rises above zero"), T > 0.0f);
    }

    {
        TestTrue(TEXT("negative severity → no negative threat"), AccumulatePlayerThreat(0.0f, -50.0f, 1.0f) >= 0.0f);
        TestTrue(TEXT("negative weight → no negative threat"), AccumulatePlayerThreat(0.0f, 50.0f, -1.0f) >= 0.0f);
        TestTrue(TEXT("negative prior clamps to >= 0"), AccumulatePlayerThreat(-100.0f, 10.0f, 1.0f) >= 0.0f);
        TestEqual(TEXT("negative severity contributes nothing"), AccumulatePlayerThreat(30.0f, -50.0f, 1.0f), 30.0f);
    }

    {
        const float Cap = 100.0f;
        float T = 0.0f;
        for (int32 i = 0; i < 1000; ++i) {
            T = AccumulatePlayerThreat(T, 999.0f, 1.0f, Cap);
            TestTrue(TEXT("threat never exceeds the soft cap"), T <= Cap + KINDA_SMALL_NUMBER);
        }
        TestTrue(TEXT("one huge deed is capped"), AccumulatePlayerThreat(0.0f, 1.0e9f, 1.0f, Cap) <= Cap + KINDA_SMALL_NUMBER);
        const float A = AccumulatePlayerThreat(Cap, 50.0f, 1.0f, Cap);
        TestTrue(TEXT("at-cap feedback does not decrease"), A >= Cap - KINDA_SMALL_NUMBER);
    }

    {
        TestEqual(TEXT("uncapped accumulation is additive"), AccumulatePlayerThreat(40.0f, 10.0f, 2.0f, 0.0f), 60.0f);
    }

    {
        float T = 100.0f;
        float Prev = T;
        for (int32 i = 0; i < 60; ++i) {
            T = DecayThreat(T, 2.0f, 1.0f);
            TestTrue(TEXT("decay never increases threat"), T <= Prev + KINDA_SMALL_NUMBER);
            TestTrue(TEXT("decay never goes negative"), T >= 0.0f);
            Prev = T;
        }
        TestEqual(TEXT("threat fully decays to 0"), T, 0.0f);
        TestEqual(TEXT("decay overshoot clamps at 0"), DecayThreat(10.0f, 1.0f, 1000.0f), 0.0f);
        TestEqual(TEXT("negative rate does not raise threat"), DecayThreat(10.0f, -5.0f, 1.0f), 10.0f);
        TestEqual(TEXT("negative dt does not raise threat"), DecayThreat(10.0f, 5.0f, -1.0f), 10.0f);
    }

    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicVendettaSelectTest,
    "Mythic.LivingWorld.Vendetta.Select",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicVendettaSelectTest::RunTest(const FString &Parameters) {
    using namespace MythicVendetta;

    FMythicVendettaThresholds T;
    const float OffCooldown = T.CooldownSeconds + 1.0f;
    const float StrongMil = 1.0f;
    const float WeakMil = 0.0f;

    TestEqual(TEXT("below floor → None"),
              SelectVendetta(T.BountyAt - 1.0f, StrongMil, OffCooldown, T), EMythicVendettaType::None);
    TestEqual(TEXT("zero threat → None"),
              SelectVendetta(0.0f, StrongMil, OffCooldown, T), EMythicVendettaType::None);

    TestEqual(TEXT("at BountyAt → BountyPosting"),
              SelectVendetta(T.BountyAt, StrongMil, OffCooldown, T), EMythicVendettaType::BountyPosting);
    TestEqual(TEXT("between bounty and assassin → BountyPosting"),
              SelectVendetta(T.AssassinAt - 1.0f, StrongMil, OffCooldown, T), EMythicVendettaType::BountyPosting);
    TestEqual(TEXT("at AssassinAt → AssassinDispatch"),
              SelectVendetta(T.AssassinAt, StrongMil, OffCooldown, T), EMythicVendettaType::AssassinDispatch);
    TestEqual(TEXT("between assassin and raid → AssassinDispatch"),
              SelectVendetta(T.RaidAt - 1.0f, StrongMil, OffCooldown, T), EMythicVendettaType::AssassinDispatch);
    TestEqual(TEXT("at RaidAt with strong military → RetaliationRaid"),
              SelectVendetta(T.RaidAt, StrongMil, OffCooldown, T), EMythicVendettaType::RetaliationRaid);
    TestEqual(TEXT("far above RaidAt with strong military → RetaliationRaid"),
              SelectVendetta(T.RaidAt + 500.0f, StrongMil, OffCooldown, T), EMythicVendettaType::RetaliationRaid);

    TestEqual(TEXT("raid-tier threat but weak military → AssassinDispatch (raid suppressed)"),
              SelectVendetta(T.RaidAt + 100.0f, WeakMil, OffCooldown, T), EMythicVendettaType::AssassinDispatch);
    TestEqual(TEXT("military exactly at the gate qualifies for raid"),
              SelectVendetta(T.RaidAt, T.MilitaryGateForRaid, OffCooldown, T), EMythicVendettaType::RetaliationRaid);

    TestEqual(TEXT("within cooldown suppresses even a raid-tier grudge"),
              SelectVendetta(T.RaidAt + 500.0f, StrongMil, 0.0f, T), EMythicVendettaType::None);
    TestEqual(TEXT("just inside cooldown → None"),
              SelectVendetta(T.RaidAt + 500.0f, StrongMil, T.CooldownSeconds - 0.01f, T), EMythicVendettaType::None);
    TestEqual(TEXT("exactly at cooldown → allowed"),
              SelectVendetta(T.BountyAt, StrongMil, T.CooldownSeconds, T), EMythicVendettaType::BountyPosting);

    for (int32 i = 0; i < 8; ++i) {
        TestEqual(TEXT("SelectVendetta is deterministic"),
                  SelectVendetta(T.AssassinAt, StrongMil, OffCooldown, T),
                  SelectVendetta(T.AssassinAt, StrongMil, OffCooldown, T));
    }

    return true;
}


#include "Misc/AutomationTest.h"
#include "World/LivingWorld/Acquaintance/MythicAcquaintanceTypes.h"
#include "World/LivingWorld/Acquaintance/MythicMourningRules.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAcquaintanceApplyTest,
    "Mythic.World.Acquaintance.ApplyInteraction",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicAcquaintanceApplyTest::RunTest(const FString &Parameters) {
    FMythicAcquaintanceConfig Config;
    TArray<FMythicNpcRelation> Relations;

    TestEqual(TEXT("zero hash → refused (0 warmth)"),
              FMythicAcquaintanceRules::ApplyInteraction(Relations, 0, FGameplayTag(), EMythicNpcInteraction::Met, 10.0, Config), 0.0f);
    TestEqual(TEXT("zero hash → no row"), Relations.Num(), 0);

    const float AfterMet =
        FMythicAcquaintanceRules::ApplyInteraction(Relations, 111, FGameplayTag(), EMythicNpcInteraction::Met, 10.0, Config);
    TestEqual(TEXT("Met applies MetDelta"), AfterMet, Config.MetDelta);
    TestEqual(TEXT("one row"), Relations.Num(), 1);
    TestTrue(TEXT("Met flag stamped"), Relations[0].HasFlag(EMythicNpcRelationFlags::Met));
    TestFalse(TEXT("Wronged flag not stamped"), Relations[0].HasFlag(EMythicNpcRelationFlags::Wronged));
    TestEqual(TEXT("anchor stamped"), Relations[0].LastInteractionTime, 10.0);

    FMythicAcquaintanceRules::ApplyInteraction(Relations, 111, FGameplayTag(), EMythicNpcInteraction::Saved, 10.0, Config);
    TestEqual(TEXT("Met + Saved accumulate"), Relations[0].Warmth, Config.MetDelta + Config.SavedDelta);
    TestTrue(TEXT("Saved flag stamped"), Relations[0].HasFlag(EMythicNpcRelationFlags::Saved));

    for (int32 i = 0; i < 20; ++i) {
        FMythicAcquaintanceRules::ApplyInteraction(Relations, 111, FGameplayTag(), EMythicNpcInteraction::Saved, 10.0, Config);
    }
    TestEqual(TEXT("warmth clamps at +100"), Relations[0].Warmth, FMythicAcquaintanceRules::WarmthMax);

    for (int32 i = 0; i < 20; ++i) {
        FMythicAcquaintanceRules::ApplyInteraction(Relations, 111, FGameplayTag(), EMythicNpcInteraction::Killed, 10.0, Config);
    }
    TestEqual(TEXT("warmth clamps at -100"), Relations[0].Warmth, FMythicAcquaintanceRules::WarmthMin);
    TestTrue(TEXT("Wronged flag stamped"), Relations[0].HasFlag(EMythicNpcRelationFlags::Wronged));
    TestTrue(TEXT("flags are sticky (Saved survives)"), Relations[0].HasFlag(EMythicNpcRelationFlags::Saved));

    TestFalse(TEXT("no faction cached yet"), Relations[0].Faction.IsValid());

    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAcquaintanceLruTest,
    "Mythic.World.Acquaintance.LruEviction",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicAcquaintanceLruTest::RunTest(const FString &Parameters) {
    FMythicAcquaintanceConfig Config;
    Config.MaxRelations = 3;
    TArray<FMythicNpcRelation> Relations;

    FMythicAcquaintanceRules::ApplyInteraction(Relations, 1, FGameplayTag(), EMythicNpcInteraction::Met, 1.0, Config);
    FMythicAcquaintanceRules::ApplyInteraction(Relations, 2, FGameplayTag(), EMythicNpcInteraction::Met, 2.0, Config);
    FMythicAcquaintanceRules::ApplyInteraction(Relations, 3, FGameplayTag(), EMythicNpcInteraction::Met, 3.0, Config);
    TestEqual(TEXT("at cap"), Relations.Num(), 3);

    FMythicAcquaintanceRules::ApplyInteraction(Relations, 1, FGameplayTag(), EMythicNpcInteraction::Traded, 4.0, Config);

    FMythicAcquaintanceRules::ApplyInteraction(Relations, 4, FGameplayTag(), EMythicNpcInteraction::Met, 5.0, Config);
    TestEqual(TEXT("still at cap"), Relations.Num(), 3);
    TestNull(TEXT("LRU row (hash 2) evicted"), FMythicAcquaintanceRules::FindRelation(Relations, 2));
    TestNotNull(TEXT("refreshed row (hash 1) kept"), FMythicAcquaintanceRules::FindRelation(Relations, 1));
    TestNotNull(TEXT("hash 3 kept"), FMythicAcquaintanceRules::FindRelation(Relations, 3));
    TestNotNull(TEXT("new row (hash 4) added"), FMythicAcquaintanceRules::FindRelation(Relations, 4));

    FMythicAcquaintanceRules::ApplyInteraction(Relations, 3, FGameplayTag(), EMythicNpcInteraction::Traded, 6.0, Config);
    TestEqual(TEXT("existing update evicts nothing"), Relations.Num(), 3);

    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAcquaintanceTierTest,
    "Mythic.World.Acquaintance.WarmthTier",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicAcquaintanceTierTest::RunTest(const FString &Parameters) {
    using R = FMythicAcquaintanceRules;

    TestTrue(TEXT("-100 → Hostile"), R::WarmthTier(-100.0f) == EMythicWarmthTier::Hostile);
    TestTrue(TEXT("-50 → Hostile (inclusive)"), R::WarmthTier(-50.0f) == EMythicWarmthTier::Hostile);
    TestTrue(TEXT("-49.9 → Wary"), R::WarmthTier(-49.9f) == EMythicWarmthTier::Wary);
    TestTrue(TEXT("-15 → Wary (inclusive)"), R::WarmthTier(-15.0f) == EMythicWarmthTier::Wary);
    TestTrue(TEXT("-14.9 → Stranger"), R::WarmthTier(-14.9f) == EMythicWarmthTier::Stranger);
    TestTrue(TEXT("0 → Stranger"), R::WarmthTier(0.0f) == EMythicWarmthTier::Stranger);
    TestTrue(TEXT("14.9 → Stranger"), R::WarmthTier(14.9f) == EMythicWarmthTier::Stranger);
    TestTrue(TEXT("15 → Acquaintance (inclusive)"), R::WarmthTier(15.0f) == EMythicWarmthTier::Acquaintance);
    TestTrue(TEXT("44.9 → Acquaintance"), R::WarmthTier(44.9f) == EMythicWarmthTier::Acquaintance);
    TestTrue(TEXT("45 → Friend (inclusive)"), R::WarmthTier(45.0f) == EMythicWarmthTier::Friend);
    TestTrue(TEXT("79.9 → Friend"), R::WarmthTier(79.9f) == EMythicWarmthTier::Friend);
    TestTrue(TEXT("80 → Confidant (inclusive)"), R::WarmthTier(80.0f) == EMythicWarmthTier::Confidant);
    TestTrue(TEXT("100 → Confidant"), R::WarmthTier(100.0f) == EMythicWarmthTier::Confidant);

    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAcquaintanceDecayTest,
    "Mythic.World.Acquaintance.LazyDecay",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicAcquaintanceDecayTest::RunTest(const FString &Parameters) {
    using R = FMythicAcquaintanceRules;
    constexpr float DayLen = 1200.0f;
    constexpr float Rate = 2.0f;

    TestEqual(TEXT("one day decays Rate toward 0 (positive)"), R::DecayedWarmth(10.0f, 0.0, DayLen, Rate, DayLen), 8.0f);
    TestEqual(TEXT("one day decays Rate toward 0 (negative)"), R::DecayedWarmth(-10.0f, 0.0, DayLen, Rate, DayLen), -8.0f);

    TestEqual(TEXT("positive never crosses 0"), R::DecayedWarmth(1.0f, 0.0, 100.0 * DayLen, Rate, DayLen), 0.0f);
    TestEqual(TEXT("negative never crosses 0"), R::DecayedWarmth(-1.0f, 0.0, 100.0 * DayLen, Rate, DayLen), 0.0f);

    TestEqual(TEXT("zero elapsed → unchanged"), R::DecayedWarmth(10.0f, 50.0, 50.0, Rate, DayLen), 10.0f);
    TestEqual(TEXT("negative elapsed → unchanged"), R::DecayedWarmth(10.0f, 500.0, 100.0, Rate, DayLen), 10.0f);
    TestEqual(TEXT("rate 0 → unchanged"), R::DecayedWarmth(10.0f, 0.0, DayLen, 0.0f, DayLen), 10.0f);
    TestEqual(TEXT("day length 0 → unchanged"), R::DecayedWarmth(10.0f, 0.0, DayLen, Rate, 0.0f), 10.0f);

    {
        FMythicAcquaintanceConfig Config;
        Config.DecayPerDay = Rate;
        Config.SecondsPerWorldDay = DayLen;
        Config.TradedDelta = 5.0f;
        Config.MetDelta = 10.0f;
        TArray<FMythicNpcRelation> Relations;
        FMythicAcquaintanceRules::ApplyInteraction(Relations, 7, FGameplayTag(), EMythicNpcInteraction::Met, 0.0, Config);
        TestEqual(TEXT("seeded at +10"), Relations[0].Warmth, 10.0f);
        const float After =
            FMythicAcquaintanceRules::ApplyInteraction(Relations, 7, FGameplayTag(), EMythicNpcInteraction::Traded, DayLen, Config);
        TestEqual(TEXT("decay settles before the delta (8 + 5)"), After, 13.0f);
        TestEqual(TEXT("anchor rebased"), Relations[0].LastInteractionTime, static_cast<double>(DayLen));
    }

    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAvengerGateTest,
    "Mythic.World.Acquaintance.Avenger",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicAvengerGateTest::RunTest(const FString &Parameters) {
    using namespace MythicMourning;

    FMythicAvengerConfig Config;

    TestEqual(TEXT("zero notoriety → base chance"), AvengerChance(0.0f, Config), Config.BaseChance);
    TestEqual(TEXT("100 notoriety → base + step"), AvengerChance(100.0f, Config), Config.BaseChance + Config.ChancePerNotoriety100);
    TestEqual(TEXT("absurd notoriety clamps at MaxChance"), AvengerChance(1.0e6f, Config), Config.MaxChance);
    TestEqual(TEXT("negative notoriety clamps to base"), AvengerChance(-500.0f, Config), Config.BaseChance);
    {
        float Prev = -1.0f;
        for (float N = 0.0f; N <= 800.0f; N += 50.0f) {
            const float C = AvengerChance(N, Config);
            TestTrue(TEXT("chance never decreases as notoriety rises"), C >= Prev);
            Prev = C;
        }
    }

    TestTrue(TEXT("all gates pass → avenger"),
             ShouldSpawnAvenger( true, 0.0f, 1000.0, 0, 0.05f, Config));

    {
        FMythicAvengerConfig Sure = Config;
        Sure.BaseChance = 1.0f;
        Sure.MaxChance = 1.0f;
        TestFalse(TEXT("non-notable → never"), ShouldSpawnAvenger(false, 1000.0f, 1.0e9, 0, 0.0f, Sure));
    }

    TestFalse(TEXT("inside cooldown → no"), ShouldSpawnAvenger(true, 0.0f, Config.CooldownSeconds - 1.0, 0, 0.0f, Config));
    TestTrue(TEXT("at cooldown boundary → yes"), ShouldSpawnAvenger(true, 0.0f, Config.CooldownSeconds, 0, 0.05f, Config));

    TestFalse(TEXT("at live cap → no"),
              ShouldSpawnAvenger(true, 0.0f, 1.0e9, Config.MaxSimultaneousAvengers, 0.0f, Config));

    {
        FMythicAvengerConfig Zero = Config;
        Zero.BaseChance = 0.0f;
        Zero.ChancePerNotoriety100 = 0.0f;
        TestFalse(TEXT("chance 0 never fires (even roll 0)"), ShouldSpawnAvenger(true, 0.0f, 1.0e9, 0, 0.0f, Zero));
    }
    TestFalse(TEXT("roll above chance → no"), ShouldSpawnAvenger(true, 0.0f, 1.0e9, 0, Config.BaseChance + 0.01f, Config));
    TestTrue(TEXT("roll at chance → yes (inclusive)"), ShouldSpawnAvenger(true, 0.0f, 1.0e9, 0, Config.BaseChance, Config));

    return true;
}


#include "Misc/AutomationTest.h"
#include "World/Camping/MythicCampsiteCore.h"
#include "World/Camping/MythicCampfireFuel.h"
#include "GameplayTagContainer.h"

namespace {
FGameplayTag CampFireTag() { return FGameplayTag::RequestGameplayTag(FName("Comfort.Fire"), false); }
FGameplayTag CampShelterTag() { return FGameplayTag::RequestGameplayTag(FName("Comfort.Shelter"), false); }
FGameplayTag CampRackTag() { return FGameplayTag::RequestGameplayTag(FName("Comfort.Rack"), false); }
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicCampsiteComfortFoldTest,
    "Mythic.World.Campsite.ComfortFold",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicCampsiteComfortFoldTest::RunTest(const FString &Parameters) {
    using namespace MythicCampsite;

    const FMythicComfortScale Camp = FMythicComfortScale::Camp();
    const FGameplayTag Fire = CampFireTag();
    const FGameplayTag Shelter = CampShelterTag();
    const FGameplayTag Rack = CampRackTag();
    if (!TestTrue(TEXT("native comfort tags registered"), Fire.IsValid() && Shelter.IsValid() && Rack.IsValid())) {
        return false;
    }

    TestEqual(TEXT("empty sources → 0 points"), FoldComfortPoints({}, Camp), 0.0f);
    TestEqual(TEXT("empty sources → tier 0"), ComputeComfortTier({}, Camp), 0);

    {
        const TArray<FMythicComfortSource> Sources = {{Fire, 1.0f}};
        TestEqual(TEXT("lone fire → 1 point"), FoldComfortPoints(Sources, Camp), 1.0f);
        TestEqual(TEXT("lone fire → tier 1"), ComputeComfortTier(Sources, Camp), 1);
    }

    {
        const TArray<FMythicComfortSource> Two = {{Fire, 1.0f}, {Shelter, 1.0f}};
        TestEqual(TEXT("fire+shelter → tier 2"), ComputeComfortTier(Two, Camp), 2);
        const TArray<FMythicComfortSource> Three = {{Fire, 1.0f}, {Shelter, 1.0f}, {Rack, 1.0f}};
        TestEqual(TEXT("fire+shelter+rack → tier 3 (camp max)"), ComputeComfortTier(Three, Camp), 3);
    }

    {
        const TArray<FMythicComfortSource> TwoTents = {{Shelter, 1.0f}, {Shelter, 1.0f}};
        TestEqual(TEXT("two tents fold to 1.5 (factor 0.5)"), FoldComfortPoints(TwoTents, Camp), 1.5f);

        const TArray<FMythicComfortSource> FireTwoTents = {{Fire, 1.0f}, {Shelter, 1.0f}, {Shelter, 1.0f}};
        const TArray<FMythicComfortSource> FireTentRack = {{Fire, 1.0f}, {Shelter, 1.0f}, {Rack, 1.0f}};
        TestTrue(TEXT("diversity beats duplication (3 distinct > fire+2 tents)"),
                 FoldComfortPoints(FireTentRack, Camp) > FoldComfortPoints(FireTwoTents, Camp));
        TestEqual(TEXT("fire+2 tents stays tier 2"), ComputeComfortTier(FireTwoTents, Camp), 2);
    }

    {
        const TArray<FMythicComfortSource> Mixed = {{Shelter, 1.0f}, {Shelter, 2.0f}};
        TestEqual(TEXT("best-first fold: 2 + 1*0.5 = 2.5, category-capped to 2"), FoldComfortPoints(Mixed, Camp), 2.0f);
    }

    {
        const TArray<FMythicComfortSource> FireSpam = {{Fire, 5.0f}, {Fire, 5.0f}, {Fire, 5.0f}};
        TestEqual(TEXT("category cap clamps fire spam to 2"), FoldComfortPoints(FireSpam, Camp), 2.0f);
        TestEqual(TEXT("fire spam alone caps at tier 2"), ComputeComfortTier(FireSpam, Camp), 2);
    }

    {
        const TArray<FMythicComfortSource> Junk = {{FGameplayTag(), 10.0f}, {Fire, -5.0f}};
        TestEqual(TEXT("invalid tag skipped + negative points clamp → 0"), FoldComfortPoints(Junk, Camp), 0.0f);
    }

    {
        TArray<FMythicComfortSource> Growing;
        int32 PrevTier = 0;
        const FGameplayTag Cats[3] = {Fire, Shelter, Rack};
        for (int32 i = 0; i < 9; ++i) {
            Growing.Add({Cats[i % 3], 1.0f});
            const int32 Tier = ComputeComfortTier(Growing, Camp);
            TestTrue(TEXT("tier never decreases as sources are added"), Tier >= PrevTier);
            PrevTier = Tier;
        }
    }

    {
        const FMythicComfortScale Homestead = FMythicComfortScale::Homestead();
        TestEqual(TEXT("homestead: empty → tier 0"), ComputeComfortTier({}, Homestead), 0);
        const TArray<FMythicComfortSource> Small = {{Fire, 1.0f}};
        TestEqual(TEXT("homestead: lone fire → tier 1"), ComputeComfortTier(Small, Homestead), 1);

        TArray<FMythicComfortSource> Estate;
        const FName EstateCats[5] = {FName("Comfort.Fire"), FName("Comfort.Shelter"), FName("Comfort.Rack"),
                                     FName("Status.Rested"), FName("Influence.Shelter")};
        for (const FName &Cat : EstateCats) {
            const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(Cat, false);
            Estate.Add({Tag, 4.0f});
        }
        TestEqual(TEXT("homestead: rich diverse estate → tier 5 (homestead max)"), ComputeComfortTier(Estate, Homestead), 5);
        TestEqual(TEXT("camp scale caps the SAME estate at 3"), ComputeComfortTier(Estate, Camp), 3);
    }

    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicCampsiteRestBonusTest,
    "Mythic.World.Campsite.RestBonus",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicCampsiteRestBonusTest::RunTest(const FString &Parameters) {
    using namespace MythicCampsite;

    FMythicRestBonusConfig Config;

    const FMythicRestBonus T0 = ResolveRestBonus(0, Config);
    TestEqual(TEXT("tier 0 XP mult = base"), T0.XpMultiplier, Config.BaseXpMultiplier);
    TestEqual(TEXT("tier 0 duration = base"), T0.DurationSeconds, Config.BaseDurationSeconds);
    const FMythicRestBonus TNeg = ResolveRestBonus(-3, Config);
    TestEqual(TEXT("negative tier clamps to tier 0 (XP)"), TNeg.XpMultiplier, T0.XpMultiplier);
    TestEqual(TEXT("negative tier clamps to tier 0 (duration)"), TNeg.DurationSeconds, T0.DurationSeconds);

    const FMythicRestBonus T2 = ResolveRestBonus(2, Config);
    TestEqual(TEXT("tier 2 XP mult = base + 2*step"), T2.XpMultiplier, 1.15f);
    TestEqual(TEXT("tier 2 duration = base + 2*step"), T2.DurationSeconds, 660.0f);
    {
        FMythicRestBonus Prev = ResolveRestBonus(0, Config);
        for (int32 Tier = 1; Tier <= 10; ++Tier) {
            const FMythicRestBonus Cur = ResolveRestBonus(Tier, Config);
            TestTrue(TEXT("XP mult never decreases with tier"), Cur.XpMultiplier >= Prev.XpMultiplier);
            TestTrue(TEXT("duration never decreases with tier"), Cur.DurationSeconds >= Prev.DurationSeconds);
            Prev = Cur;
        }
    }

    const FMythicRestBonus THuge = ResolveRestBonus(1000, Config);
    TestEqual(TEXT("XP mult clamps at MaxXpMultiplier"), THuge.XpMultiplier, Config.MaxXpMultiplier);
    TestEqual(TEXT("duration clamps at MaxDurationSeconds"), THuge.DurationSeconds, Config.MaxDurationSeconds);

    FMythicRestBonusConfig Hostile;
    Hostile.BaseXpMultiplier = 0.2f;
    Hostile.XpMultiplierPerTier = 0.0f;
    Hostile.MaxXpMultiplier = 0.5f;
    TestTrue(TEXT("XP mult never drops below 1.0"), ResolveRestBonus(0, Hostile).XpMultiplier >= 1.0f);

    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicCampsiteEventGatesTest,
    "Mythic.World.Campsite.EventGates",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicCampsiteEventGatesTest::RunTest(const FString &Parameters) {
    using namespace MythicCampsite;

    TestTrue(TEXT("all gates pass + roll under chance → fires"),
             CanFireHostileCampEvent( false, true, true, true, 0.1f, 0.5f));

    TestFalse(TEXT("(a) pacing Rest phase blocks"),
              CanFireHostileCampEvent(true, true, true, true, 0.1f, 0.5f));
    TestFalse(TEXT("(c) nobody online/near blocks"),
              CanFireHostileCampEvent(false, false, true, true, 0.1f, 0.5f));
    TestFalse(TEXT("(b) untelegraphed blocks"),
              CanFireHostileCampEvent(false, true, false, true, 0.1f, 0.5f));
    TestFalse(TEXT("(d) master toggle off blocks"),
              CanFireHostileCampEvent(false, true, true, false, 0.1f, 0.5f));

    TestFalse(TEXT("chance 0 never fires (roll 0)"), CanFireHostileCampEvent(false, true, true, true, 0.0f, 0.0f));
    TestTrue(TEXT("chance 1 always fires (roll 1)"), CanFireHostileCampEvent(false, true, true, true, 1.0f, 1.0f));
    TestFalse(TEXT("roll above chance misses"), CanFireHostileCampEvent(false, true, true, true, 0.9f, 0.5f));

    FMythicCampEventConfig Events;
    TestEqual(TEXT("danger tier 0 (Safe) → chance 0"), ComputeAmbushChance(0, Events), 0.0f);
    TestEqual(TEXT("negative tier → chance 0"), ComputeAmbushChance(-2, Events), 0.0f);
    float Prev = 0.0f;
    for (int32 Tier = 1; Tier <= 8; ++Tier) {
        const float Chance = ComputeAmbushChance(Tier, Events);
        TestTrue(TEXT("chance never decreases with danger"), Chance >= Prev);
        TestTrue(TEXT("chance never exceeds the max clamp"), Chance <= Events.AmbushMaxChance);
        Prev = Chance;
    }
    TestEqual(TEXT("tier 2 = base + 2*step"), ComputeAmbushChance(2, Events), 0.08f);
    TestEqual(TEXT("huge tier clamps at max"), ComputeAmbushChance(100, Events), Events.AmbushMaxChance);

    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicCampsiteFuelTest,
    "Mythic.World.Campsite.CampfireFuel",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicCampsiteFuelTest::RunTest(const FString &Parameters) {
    using namespace MythicCampfireFuel;

    TestEqual(TEXT("future deadline → remaining"), RemainingBurnSeconds(1000.0, 400.0), 600.0);
    TestEqual(TEXT("past deadline → 0"), RemainingBurnSeconds(100.0, 400.0), 0.0);
    TestEqual(TEXT("exact deadline → 0"), RemainingBurnSeconds(400.0, 400.0), 0.0);
    TestTrue(TEXT("lit while remaining > 0"), IsBurning(401.0, 400.0));
    TestFalse(TEXT("out at the deadline"), IsBurning(400.0, 400.0));

    TestEqual(TEXT("lit fire extends from deadline"), AddFuelSeconds( 500.0, 400.0, 100.0, 0.0), 600.0);
    TestEqual(TEXT("dead fire re-anchors at now"), AddFuelSeconds( 100.0, 400.0, 100.0, 0.0), 500.0);

    TestEqual(TEXT("cap clamps remaining to now+max"), AddFuelSeconds(500.0, 400.0, 10000.0, 300.0), 700.0);
    TestEqual(TEXT("uncapped when max <= 0"), AddFuelSeconds(500.0, 400.0, 10000.0, 0.0), 10500.0);
    TestEqual(TEXT("negative add never shortens the burn"), AddFuelSeconds(500.0, 400.0, -50.0, 0.0), 500.0);

    {
        TArray<uint8> Data;
        SerializeFuel(Data, 123.5);
        TestTrue(TEXT("payload carries the version byte"), Data.Num() > 0 && Data[0] == FuelPayloadVersion);
        TestEqual(TEXT("v1 round-trip restores seconds-remaining"), DeserializeFuel(Data, 999.0), 123.5);
    }
    {
        const TArray<uint8> Empty;
        TestEqual(TEXT("empty payload → default"), DeserializeFuel(Empty, 600.0), 600.0);
    }
    {
        TArray<uint8> Future;
        SerializeFuel(Future, 50.0);
        Future[0] = 99;
        TestEqual(TEXT("unknown version → default"), DeserializeFuel(Future, 600.0), 600.0);

        TArray<uint8> Truncated;
        SerializeFuel(Truncated, 50.0);
        Truncated.SetNum(3);
        TestEqual(TEXT("truncated payload → default"), DeserializeFuel(Truncated, 600.0), 600.0);
    }
    {
        TArray<uint8> Data;
        SerializeFuel(Data, -25.0);
        TestEqual(TEXT("negative remaining clamps to 0 across the trip"), DeserializeFuel(Data, 600.0), 0.0);
    }

    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicCampsiteRoadSpeedTest,
    "Mythic.World.Campsite.RoadSpeed",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicCampsiteRoadSpeedTest::RunTest(const FString &Parameters) {
    using namespace MythicCampsite;

    TestEqual(TEXT("off-road is always 1.0"), RoadSpeedMultiplier(false), 1.0f);
    TestEqual(TEXT("on-road with the default multiplier is 1.0 (neutral)"), RoadSpeedMultiplier(true), 1.0f);

    TestEqual(TEXT("tuned on-road multiplier passes through"), RoadSpeedMultiplier(true, 1.3f), 1.3f);
    TestEqual(TEXT("off-road ignores the tuned value"), RoadSpeedMultiplier(false, 1.3f), 1.0f);
    TestEqual(TEXT("mis-authored 0 clamps to the floor"), RoadSpeedMultiplier(true, 0.0f), 0.1f);

    return true;
}

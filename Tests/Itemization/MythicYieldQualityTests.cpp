
#include "Misc/AutomationTest.h"
#include "World/Gathering/MythicYieldQuality.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicYieldQualityMultiplierTest,
    "Mythic.Itemization.YieldQuality.Multipliers",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicYieldQualityMultiplierTest::RunTest(const FString &Parameters) {
    using Q = FMythicYieldQuality;
    FMythicYieldQualityRules Rules;

    TestEqual(TEXT("Common potency is 1.0 by default (inert)"), Q::PotencyMultiplier(Rules, EMythicYieldQuality::Common), 1.0f);
    TestEqual(TEXT("Common price is 1.0 by default (inert)"), Q::PriceMultiplier(Rules, EMythicYieldQuality::Common), 1.0f);

    for (int32 i = 1; i < Q::NumTiers; i++) {
        const EMythicYieldQuality Lo = Q::TierFromIndex(i - 1);
        const EMythicYieldQuality Hi = Q::TierFromIndex(i);
        TestTrue(FString::Printf(TEXT("potency monotonic at tier %d"), i),
                 Q::PotencyMultiplier(Rules, Hi) >= Q::PotencyMultiplier(Rules, Lo));
        TestTrue(FString::Printf(TEXT("price monotonic at tier %d"), i),
                 Q::PriceMultiplier(Rules, Hi) >= Q::PriceMultiplier(Rules, Lo));
    }

    FMythicYieldQualityRules Inverted;
    Inverted.FinePotencyMultiplier = 0.5f;
    Inverted.PristinePotencyMultiplier = 0.1f;
    TestTrue(TEXT("sanitized: Fine >= Common despite inverted config"),
             Q::PotencyMultiplier(Inverted, EMythicYieldQuality::Fine) >= Q::PotencyMultiplier(Inverted, EMythicYieldQuality::Common));
    TestTrue(TEXT("sanitized: Pristine >= Fine despite inverted config"),
             Q::PotencyMultiplier(Inverted, EMythicYieldQuality::Pristine) >= Q::PotencyMultiplier(Inverted, EMythicYieldQuality::Fine));

    FMythicYieldQualityRules Negative;
    Negative.RaggedPotencyMultiplier = -5.0f;
    TestTrue(TEXT("negative config floors at 0"), Q::PotencyMultiplier(Negative, EMythicYieldQuality::Ragged) >= 0.0f);

    const float CommonMult = Q::PotencyMultiplier(Rules, EMythicYieldQuality::Common);
    const float FineMult = Q::PotencyMultiplier(Rules, EMythicYieldQuality::Fine);
    TestEqual(TEXT("tier-value 1.0 == Common multiplier"), Q::PotencyMultiplierForTierValue(Rules, 1.0f), CommonMult);
    TestEqual(TEXT("tier-value 2.0 == Fine multiplier"), Q::PotencyMultiplierForTierValue(Rules, 2.0f), FineMult);
    TestEqual(TEXT("tier-value 1.5 == midway Common..Fine"), Q::PotencyMultiplierForTierValue(Rules, 1.5f), (CommonMult + FineMult) * 0.5f);
    TestEqual(TEXT("tier-value clamps high"), Q::PotencyMultiplierForTierValue(Rules, 99.0f), Q::PotencyMultiplier(Rules, EMythicYieldQuality::Pristine));
    TestEqual(TEXT("tier-value clamps low"), Q::PotencyMultiplierForTierValue(Rules, -3.0f), Q::PotencyMultiplier(Rules, EMythicYieldQuality::Ragged));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicYieldQualityRollTest,
    "Mythic.Itemization.YieldQuality.RollAndFloors",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicYieldQualityRollTest::RunTest(const FString &Parameters) {
    using Q = FMythicYieldQuality;
    FMythicYieldQualityRules Rules;

    for (int32 Rep = 0; Rep < 3; Rep++) {
        TestEqual(TEXT("deterministic: rand 0.5 at level 0 is Common"),
                  Q::RollQuality(Rules, 0.5f, 0), EMythicYieldQuality::Common);
    }
    TestEqual(TEXT("rand 0.0 → Pristine"), Q::RollQuality(Rules, 0.0f, 0), EMythicYieldQuality::Pristine);
    TestEqual(TEXT("rand 0.019 → Pristine"), Q::RollQuality(Rules, 0.019f, 0), EMythicYieldQuality::Pristine);
    TestEqual(TEXT("rand 0.02 → Fine (band edge)"), Q::RollQuality(Rules, 0.02f, 0), EMythicYieldQuality::Fine);
    TestEqual(TEXT("rand 0.119 → Fine"), Q::RollQuality(Rules, 0.119f, 0), EMythicYieldQuality::Fine);
    TestEqual(TEXT("rand 0.121 → Common (past the band)"), Q::RollQuality(Rules, 0.121f, 0), EMythicYieldQuality::Common);
    TestEqual(TEXT("rand 1.0 → Common"), Q::RollQuality(Rules, 1.0f, 0), EMythicYieldQuality::Common);

    TestEqual(TEXT("worst roll from a Common base is Common, never Ragged"),
              Q::RollQuality(Rules, 1.0f, 0, EMythicYieldQuality::Common), EMythicYieldQuality::Common);
    TestEqual(TEXT("hunting can pass a Ragged base through"),
              Q::RollQuality(Rules, 1.0f, 0, EMythicYieldQuality::Ragged, EMythicYieldQuality::Ragged), EMythicYieldQuality::Ragged);

    TestEqual(TEXT("fine chance caps"), Q::FineChanceAtLevel(Rules, 100000), Rules.MaxFineChance);
    TestEqual(TEXT("pristine chance caps"), Q::PristineChanceAtLevel(Rules, 100000), Rules.MaxPristineChance);
    TestTrue(TEXT("fine chance grows with mastery"), Q::FineChanceAtLevel(Rules, 20) > Q::FineChanceAtLevel(Rules, 0));

    TestEqual(TEXT("floors disabled by default: floor == source floor"),
              Q::MasteryFloor(Rules, 999, EMythicYieldQuality::Common), EMythicYieldQuality::Common);

    FMythicYieldQualityRules Floored = Rules;
    Floored.FineFloorAtMasteryLevel = 20;
    TestEqual(TEXT("below the floor level: Common"), Q::MasteryFloor(Floored, 19, EMythicYieldQuality::Common), EMythicYieldQuality::Common);
    TestEqual(TEXT("at the floor level: Fine"), Q::MasteryFloor(Floored, 20, EMythicYieldQuality::Common), EMythicYieldQuality::Fine);
    TestEqual(TEXT("a bad roll at floored mastery is LIFTED to Fine"),
              Q::RollQuality(Floored, 0.999f, 20), EMythicYieldQuality::Fine);
    TestEqual(TEXT("a Pristine roll above the Fine floor stands"),
              Q::RollQuality(Floored, 0.0f, 20), EMythicYieldQuality::Pristine);

    TestEqual(TEXT("tag name for Fine"), Q::QualityTagName(EMythicYieldQuality::Fine), FName(TEXT("Itemization.Quality.Fine")));
    EMythicYieldQuality Parsed = EMythicYieldQuality::Ragged;
    TestTrue(TEXT("parse Pristine tag name"), Q::TierFromTagName(FName(TEXT("Itemization.Quality.Pristine")), Parsed));
    TestEqual(TEXT("parsed Pristine"), Parsed, EMythicYieldQuality::Pristine);
    TestFalse(TEXT("non-quality tag name does not parse"), Q::TierFromTagName(FName(TEXT("Itemization.Type.Weapon")), Parsed));

    return true;
}

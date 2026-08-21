
#include "Misc/AutomationTest.h"
#include "GAS/Progression/MythicRenownRules.h"
#include "GAS/Progression/MythicTitleRules.h"
#include "Progression/MythicTags_MetaProgression.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRenownTest,
    "Mythic.Progression.Renown",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRenownTest::RunTest(const FString &Parameters) {
    using Rules = FMythicRenownRules;

    const float Thresholds[7] = {-6000.0f, -3000.0f, 0.0f, 3000.0f, 9000.0f, 21000.0f, 42000.0f};
    const TConstArrayView<float> T = MakeArrayView(Thresholds, 7);

    {
        TestEqual(TEXT("-7000 is Hated"), Rules::TierForValue(-7000.0f, T), EMythicRenownTier::Hated);
        TestEqual(TEXT("-6001 is Hated (just below the Hostile boundary)"), Rules::TierForValue(-6001.0f, T), EMythicRenownTier::Hated);
        TestEqual(TEXT("-6000 is Hostile (boundary is inclusive)"), Rules::TierForValue(-6000.0f, T), EMythicRenownTier::Hostile);
        TestEqual(TEXT("-3000 is Unfriendly"), Rules::TierForValue(-3000.0f, T), EMythicRenownTier::Unfriendly);
        TestEqual(TEXT("-1 is Unfriendly"), Rules::TierForValue(-1.0f, T), EMythicRenownTier::Unfriendly);
        TestEqual(TEXT("0 is Neutral (a fresh character)"), Rules::TierForValue(0.0f, T), EMythicRenownTier::Neutral);
        TestEqual(TEXT("2999 is still Neutral"), Rules::TierForValue(2999.0f, T), EMythicRenownTier::Neutral);
        TestEqual(TEXT("3000 is Friendly"), Rules::TierForValue(3000.0f, T), EMythicRenownTier::Friendly);
        TestEqual(TEXT("9000 is Honored"), Rules::TierForValue(9000.0f, T), EMythicRenownTier::Honored);
        TestEqual(TEXT("21000 is Revered"), Rules::TierForValue(21000.0f, T), EMythicRenownTier::Revered);
        TestEqual(TEXT("41999 is still Revered"), Rules::TierForValue(41999.0f, T), EMythicRenownTier::Revered);
        TestEqual(TEXT("42000 is Exalted"), Rules::TierForValue(42000.0f, T), EMythicRenownTier::Exalted);
        TestEqual(TEXT("a huge value clamps at Exalted"), Rules::TierForValue(1.0e9f, T), EMythicRenownTier::Exalted);

        TestEqual(TEXT("empty thresholds put everything in Hated (callers substitute defaults before here)"),
                  Rules::TierForValue(50000.0f, TConstArrayView<float>()), EMythicRenownTier::Hated);
        const float Short[2] = {0.0f, 100.0f};
        TestEqual(TEXT("a short (2-boundary) view tops out at tier 2"),
                  Rules::TierForValue(1.0e9f, MakeArrayView(Short, 2)), EMythicRenownTier::Unfriendly);
    }

    {
        TestEqual(TEXT("Exalted-worth value capped at Neutral reads Neutral"),
                  Rules::ClampToMaxTier(50000.0f, EMythicRenownTier::Neutral, T), EMythicRenownTier::Neutral);
        TestEqual(TEXT("a Hated-worth value is unaffected by a Neutral cap (cap is a MAX, not a floor)"),
                  Rules::ClampToMaxTier(-7000.0f, EMythicRenownTier::Neutral, T), EMythicRenownTier::Hated);
        TestEqual(TEXT("an Exalted cap changes nothing"),
                  Rules::ClampToMaxTier(50000.0f, EMythicRenownTier::Exalted, T), EMythicRenownTier::Exalted);
        TestEqual(TEXT("value exactly at the cap's tier passes through"),
                  Rules::ClampToMaxTier(0.0f, EMythicRenownTier::Neutral, T), EMythicRenownTier::Neutral);
    }

    {
        const float Discounts[8] = {0.0f, 0.0f, 0.0f, 0.0f, 0.05f, 0.10f, 0.15f, 0.20f};
        const TConstArrayView<float> D = MakeArrayView(Discounts, 8);
        TestEqual(TEXT("Neutral grants no discount"), Rules::VendorDiscountForTier(EMythicRenownTier::Neutral, D), 0.0f);
        TestEqual(TEXT("Friendly grants 5%"), Rules::VendorDiscountForTier(EMythicRenownTier::Friendly, D), 0.05f);
        TestEqual(TEXT("Exalted grants 20%"), Rules::VendorDiscountForTier(EMythicRenownTier::Exalted, D), 0.20f);
        TestEqual(TEXT("an empty discount array grants 0"),
                  Rules::VendorDiscountForTier(EMythicRenownTier::Exalted, TConstArrayView<float>()), 0.0f);
        const float ShortD[2] = {0.0f, 0.01f};
        TestEqual(TEXT("a tier past a short array grants 0"),
                  Rules::VendorDiscountForTier(EMythicRenownTier::Exalted, MakeArrayView(ShortD, 2)), 0.0f);
    }

    {
        TestTrue(TEXT("2999 -> 3000 crosses (Neutral -> Friendly)"),
                 Rules::TierForValue(2999.0f, T) != Rules::TierForValue(3000.0f, T));
        TestFalse(TEXT("3000 -> 3001 does not cross (still Friendly)"),
                  Rules::TierForValue(3000.0f, T) != Rules::TierForValue(3001.0f, T));
        TestFalse(TEXT("within-tier movement never crosses"),
                  Rules::TierForValue(100.0f, T) != Rules::TierForValue(2000.0f, T));
        TestTrue(TEXT("a big grant can skip tiers (Neutral -> Honored)"),
                 Rules::TierForValue(0.0f, T) != Rules::TierForValue(10000.0f, T));
        TestEqual(TEXT("...and lands on Honored"), Rules::TierForValue(10000.0f, T), EMythicRenownTier::Honored);
        TestTrue(TEXT("a loss can cross downward (Neutral -> Unfriendly)"),
                 Rules::TierForValue(0.0f, T) != Rules::TierForValue(-1.0f, T));
    }

    {
        FGameplayTagContainer Granted;
        Granted.AddTag(TITLE_SLAYER);
        TestTrue(TEXT("a granted title is selectable"), FMythicTitleRules::CanSelectTitle(TITLE_SLAYER, Granted));
        TestFalse(TEXT("an un-granted title is not selectable"), FMythicTitleRules::CanSelectTitle(TITLE_BOSS_HUNTER, Granted));
        TestFalse(TEXT("an invalid tag is not a selection (clear is a separate verb)"),
                  FMythicTitleRules::CanSelectTitle(FGameplayTag(), Granted));
        TestFalse(TEXT("nothing is selectable from an empty granted set"),
                  FMythicTitleRules::CanSelectTitle(TITLE_SLAYER, FGameplayTagContainer()));
    }

    return true;
}

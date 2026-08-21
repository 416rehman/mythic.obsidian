
#include "Misc/AutomationTest.h"

#include "Rewards/LootScaling.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicLootScalingTest,
    "Mythic.Itemization.LootScaling",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicLootScalingTest::RunTest(const FString &Parameters) {
    // Item Rarity Find. Weights run common to mythic, so the bias must grow along the array and leave common alone.
    {
        float Weights[5] = {100.0f, 50.0f, 20.0f, 5.0f, 1.0f};
        FMythicLootScaling::AdjustWeightsForRarityFind(MakeArrayView(Weights, 5), 0.0f);
        TestEqual(TEXT("no rarity find leaves the common weight"), Weights[0], 100.0f);
        TestEqual(TEXT("no rarity find leaves the rarest weight"), Weights[4], 1.0f);
    }
    {
        float Weights[5] = {100.0f, 50.0f, 20.0f, 5.0f, 1.0f};
        FMythicLootScaling::AdjustWeightsForRarityFind(MakeArrayView(Weights, 5), 1.0f);
        TestEqual(TEXT("the commonest tier is never boosted"), Weights[0], 100.0f);
        TestEqual(TEXT("the middle tier gets half the bonus"), Weights[2], 30.0f);
        TestEqual(TEXT("the rarest tier gets the whole bonus"), Weights[4], 2.0f);
        TestTrue(TEXT("rarity find shifts the distribution rarewards, which is the point of the stat"),
                 (Weights[4] / Weights[0]) > (1.0f / 100.0f));
    }
    {
        // The stat is rolled, so a stacked value must keep scaling rather than saturating.
        float Low[5] = {100.0f, 50.0f, 20.0f, 5.0f, 1.0f};
        float High[5] = {100.0f, 50.0f, 20.0f, 5.0f, 1.0f};
        FMythicLootScaling::AdjustWeightsForRarityFind(MakeArrayView(Low, 5), 0.5f);
        FMythicLootScaling::AdjustWeightsForRarityFind(MakeArrayView(High, 5), 2.0f);
        TestTrue(TEXT("more rarity find is strictly better at the top end"), High[4] > Low[4]);
    }
    {
        // A negative can arrive from a debuff or bad data and must not invert the table.
        float Weights[3] = {10.0f, 5.0f, 1.0f};
        FMythicLootScaling::AdjustWeightsForRarityFind(MakeArrayView(Weights, 3), -3.0f);
        TestEqual(TEXT("negative rarity find changes nothing"), Weights[2], 1.0f);

        float Single[1] = {7.0f};
        FMythicLootScaling::AdjustWeightsForRarityFind(MakeArrayView(Single, 1), 5.0f);
        TestEqual(TEXT("a one-rarity table cannot be biased"), Single[0], 7.0f);
    }

    // Item Quantity Find, and the enemy rank bonus it rides alongside.
    {
        const FLootTierBonus Normal = FMythicLootScaling::ComputeTierLootBonus(1, 0.0f);
        TestEqual(TEXT("a normal enemy grants no extra drops"), Normal.ExtraDropCount, 0);
        TestEqual(TEXT("a normal enemy has no rarity bonus"), Normal.RarityMult, 1.0f);
        TestEqual(TEXT("a normal enemy guarantees nothing"), Normal.GuaranteedMinRarity, 0);

        const FLootTierBonus Boss = FMythicLootScaling::ComputeTierLootBonus(5, 0.0f);
        TestEqual(TEXT("a boss grants extra drops"), Boss.ExtraDropCount, 3);
        TestTrue(TEXT("a boss raises rarity"), Boss.RarityMult > Normal.RarityMult);
        TestEqual(TEXT("a boss guarantees above common"), Boss.GuaranteedMinRarity, 1);
    }
    {
        // Whole part becomes guaranteed drops, fraction becomes a chance at one more.
        const FLootTierBonus Two = FMythicLootScaling::ComputeTierLootBonus(1, 2.0f);
        TestEqual(TEXT("two quantity find is two guaranteed extra drops"), Two.ExtraDropCount, 2);
        TestEqual(TEXT("with nothing left over"), Two.FractionalDropChance, 0.0f);

        const FLootTierBonus Half = FMythicLootScaling::ComputeTierLootBonus(1, 1.5f);
        TestEqual(TEXT("one and a half is one guaranteed drop"), Half.ExtraDropCount, 1);
        TestEqual(TEXT("plus a half chance at another"), Half.FractionalDropChance, 0.5f, 0.001f);

        const FLootTierBonus Stacks = FMythicLootScaling::ComputeTierLootBonus(5, 2.0f);
        TestEqual(TEXT("quantity find adds to the rank bonus rather than replacing it"), Stacks.ExtraDropCount, 5);

        const FLootTierBonus Negative = FMythicLootScaling::ComputeTierLootBonus(1, -4.0f);
        TestEqual(TEXT("negative quantity find cannot remove drops"), Negative.ExtraDropCount, 0);
        TestEqual(TEXT("nor produce a negative chance"), Negative.FractionalDropChance, 0.0f);
    }

    return true;
}

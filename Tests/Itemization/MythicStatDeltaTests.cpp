#include "Misc/AutomationTest.h"
#include "Itemization/Inventory/ViewModels/MythicStatDelta.h"
#include "Itemization/Inventory/ViewModels/MythicTags_ItemMetrics.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicStatDeltaTest,
    "Mythic.Itemization.StatDelta",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicStatDeltaTest::RunTest(const FString &Parameters) {
    using Core = FMythicStatDeltaCore;

    const FGameplayTag KeyPrimary = ITEM_METRIC_WEAPON_DAMAGE_PER_SECOND.GetTag();
    const FGameplayTag KeyAttackSpeed = ITEM_METRIC_WEAPON_ATTACKS_PER_SECOND.GetTag();
    const FGameplayTag KeySecondary = ITEM_METRIC_DURABILITY.GetTag();
    const FText Label = FText::FromString(TEXT("Stat"));
    if (!TestTrue(TEXT("native comparison identities are registered"),
                  KeyPrimary.IsValid() && KeyAttackSpeed.IsValid() && KeySecondary.IsValid())) {
        return false;
    }

    {
        TArray<FMythicComparableStat> New, Current;
        New.Emplace(KeyPrimary, Label, 10.0f);
        Current.Emplace(KeyPrimary, Label, 6.0f);
        const TArray<FAttributeDiff> Diffs = Core::ComputeDiffs(New, Current);
        TestEqual(TEXT("one shared tag produces one row"), Diffs.Num(), 1);
        if (Diffs.Num() == 1) {
            TestEqual(TEXT("identity remains the canonical tag"), Diffs[0].ComparisonTag, KeyPrimary);
            TestEqual(TEXT("new value carried"), Diffs[0].NewValue, 10.0f);
            TestEqual(TEXT("current value carried"), Diffs[0].CurrentValue, 6.0f);
            TestEqual(TEXT("delta is new minus current"), Diffs[0].Delta, 4.0f);
            TestTrue(TEXT("positive higher-is-better delta upgrades"), Diffs[0].bIsUpgrade);
        }
    }

    {
        TArray<FMythicComparableStat> New, Current;
        New.Emplace(KeyPrimary, Label, 4.0f, 0.0f, EMythicStatComparisonDirection::LowerIsBetter);
        Current.Emplace(KeyPrimary, Label, 6.0f, 0.0f, EMythicStatComparisonDirection::LowerIsBetter);
        const TArray<FAttributeDiff> Diffs = Core::ComputeDiffs(New, Current);
        TestEqual(TEXT("lower-is-better produces one row"), Diffs.Num(), 1);
        if (Diffs.Num() == 1) {
            TestEqual(TEXT("lower-is-better delta stays new minus current"), Diffs[0].Delta, -2.0f);
            TestTrue(TEXT("negative delta is the upgrade when authored lower-is-better"), Diffs[0].bIsUpgrade);
        }
    }

    {
        TArray<FMythicComparableStat> New, Current;
        New.Emplace(KeyPrimary, Label, 12.0f);
        Current.Emplace(KeySecondary, Label, 8.0f);
        const TArray<FAttributeDiff> Diffs = Core::ComputeDiffs(New, Current);
        TestEqual(TEXT("tag union emits both one-sided rows"), Diffs.Num(), 2);
        if (Diffs.Num() == 2) {
            TestEqual(TEXT("gained zero-neutral stat baselines at neutral"), Diffs[0].CurrentValue, 0.0f);
            TestTrue(TEXT("gained stat upgrades"), Diffs[0].bIsUpgrade);
            TestEqual(TEXT("lost zero-neutral stat ends at neutral"), Diffs[1].NewValue, 0.0f);
            TestFalse(TEXT("lost stat is not an upgrade"), Diffs[1].bIsUpgrade);
        }
    }

    {
        TArray<FMythicComparableStat> New, Current;
        New.Emplace(KeySecondary, Label, 5.0f);
        New.Emplace(KeySecondary, Label, 5.0f);
        Current.Emplace(KeySecondary, Label, 7.0f);
        const TArray<FAttributeDiff> Diffs = Core::ComputeDiffs(New, Current);
        TestEqual(TEXT("duplicate same-side tags fold to one row"), Diffs.Num(), 1);
        if (Diffs.Num() == 1) {
            TestEqual(TEXT("zero-neutral contributions sum"), Diffs[0].NewValue, 10.0f);
            TestEqual(TEXT("summed delta"), Diffs[0].Delta, 3.0f);
        }
    }

    {
        TArray<FMythicComparableStat> New;
        New.Emplace(KeyPrimary, Label, 0.97f, 1.0f, EMythicStatComparisonDirection::LowerIsBetter, true);
        const TArray<FAttributeDiff> Diffs = Core::ComputeDiffs(New, TArray<FMythicComparableStat>());
        TestEqual(TEXT("one-neutral multiplier produces one row"), Diffs.Num(), 1);
        if (Diffs.Num() == 1) {
            TestEqual(TEXT("missing multiplier side baselines at authored one"), Diffs[0].CurrentValue, 1.0f);
            TestEqual(TEXT("multiplier delta preserves the three-percent reduction"), Diffs[0].Delta, -0.03f, 0.0001f);
            TestTrue(TEXT("lower incoming multiplier is an upgrade"), Diffs[0].bIsUpgrade);
        }
    }

    {
        TArray<FMythicComparableStat> New;
        // The final stat may be one-neutral, but AddBase is still a zero-identity item contribution.
        New.Emplace(KeyPrimary, Label, 0.10f, 0.0f, EMythicStatComparisonDirection::HigherIsBetter, true);
        const TArray<FAttributeDiff> Diffs = Core::ComputeDiffs(New, TArray<FMythicComparableStat>());
        TestEqual(TEXT("additive contribution on a multiplier-formatted stat produces one row"), Diffs.Num(), 1);
        if (Diffs.Num() == 1) {
            TestEqual(TEXT("the missing side uses operation identity rather than final-stat neutral"),
                      Diffs[0].CurrentValue, 0.0f);
            TestEqual(TEXT("the additive ten-percent contribution remains positive"),
                      Diffs[0].Delta, 0.10f, 0.0001f);
            TestTrue(TEXT("the positive additive contribution is an upgrade"), Diffs[0].bIsUpgrade);
        }
    }

    return true;
}

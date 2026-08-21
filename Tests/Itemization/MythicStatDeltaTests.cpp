
#include "Misc/AutomationTest.h"
#include "Itemization/Inventory/ViewModels/MythicStatDelta.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicStatDeltaTest,
    "Mythic.Itemization.StatDelta",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicStatDeltaTest::RunTest(const FString &Parameters) {
    using Core = FMythicStatDeltaCore;

    const FName KeyDamage(TEXT("Base.DamageMax"));
    const FName KeyArmor(TEXT("Armor"));
    const FName KeyCooldown(TEXT("CooldownReduction.Cost"));
    const FText Label = FText::FromString(TEXT("Stat"));

    {
        TArray<FMythicComparableStat> New, Current;
        New.Emplace(KeyDamage, Label, 10.0f);
        Current.Emplace(KeyDamage, Label, 6.0f);
        const TArray<FAttributeDiff> Diffs = Core::ComputeDiffs(New, Current);
        TestEqual(TEXT("one shared key -> one row"), Diffs.Num(), 1);
        if (Diffs.Num() == 1) {
            TestEqual(TEXT("NewValue carried"), Diffs[0].NewValue, 10.0f);
            TestEqual(TEXT("CurrentValue carried"), Diffs[0].CurrentValue, 6.0f);
            TestEqual(TEXT("Delta = New - Current"), Diffs[0].Delta, 4.0f);
            TestTrue(TEXT("positive delta is an upgrade"), Diffs[0].bIsUpgrade);
        }

        const TArray<FAttributeDiff> Reverse = Core::ComputeDiffs(Current, New);
        if (Reverse.Num() == 1) {
            TestEqual(TEXT("reverse delta is negative"), Reverse[0].Delta, -4.0f);
            TestFalse(TEXT("negative delta is not an upgrade"), Reverse[0].bIsUpgrade);
        }
    }

    {
        TArray<FMythicComparableStat> New, Current;
        New.Emplace(KeyCooldown, Label, 4.0f, true);
        Current.Emplace(KeyCooldown, Label, 6.0f, true);
        const TArray<FAttributeDiff> Diffs = Core::ComputeDiffs(New, Current);
        TestEqual(TEXT("lower-is-better: one row"), Diffs.Num(), 1);
        if (Diffs.Num() == 1) {
            TestEqual(TEXT("lower-is-better delta still New - Current"), Diffs[0].Delta, -2.0f);
            TestTrue(TEXT("negative delta IS the upgrade when lower is better"), Diffs[0].bIsUpgrade);
        }

        const TArray<FAttributeDiff> Worse = Core::ComputeDiffs(Current, New);
        if (Worse.Num() == 1) {
            TestFalse(TEXT("positive delta is a DOWNGRADE when lower is better"), Worse[0].bIsUpgrade);
        }
    }

    {
        TArray<FMythicComparableStat> New, Current;
        New.Emplace(KeyDamage, Label, 12.0f);
        Current.Emplace(KeyArmor, Label, 8.0f);
        const TArray<FAttributeDiff> Diffs = Core::ComputeDiffs(New, Current);
        TestEqual(TEXT("key-union emits both one-sided rows"), Diffs.Num(), 2);
        if (Diffs.Num() == 2) {
            TestEqual(TEXT("gained stat baselines Current at 0"), Diffs[0].CurrentValue, 0.0f);
            TestEqual(TEXT("gained stat delta is its full value"), Diffs[0].Delta, 12.0f);
            TestTrue(TEXT("gained stat is an upgrade"), Diffs[0].bIsUpgrade);

            TestEqual(TEXT("lost stat baselines New at 0"), Diffs[1].NewValue, 0.0f);
            TestEqual(TEXT("lost stat delta is its full negative value"), Diffs[1].Delta, -8.0f);
            TestFalse(TEXT("lost stat is a downgrade"), Diffs[1].bIsUpgrade);
        }
    }

    {
        TArray<FMythicComparableStat> New, Current;
        New.Emplace(KeyArmor, Label, 5.0f);
        New.Emplace(KeyArmor, Label, 5.0f);
        Current.Emplace(KeyArmor, Label, 7.0f);
        const TArray<FAttributeDiff> Diffs = Core::ComputeDiffs(New, Current);
        TestEqual(TEXT("duplicate same-side keys fold to one row"), Diffs.Num(), 1);
        if (Diffs.Num() == 1) {
            TestEqual(TEXT("duplicates summed before diffing"), Diffs[0].NewValue, 10.0f);
            TestEqual(TEXT("summed delta"), Diffs[0].Delta, 3.0f);
            TestTrue(TEXT("summed comparison upgrades"), Diffs[0].bIsUpgrade);
        }
    }

    {
        TArray<FMythicComparableStat> New, Current;
        New.Emplace(KeyDamage, Label, 10.0f);
        New.Emplace(KeyArmor, Label, 5.0f);
        New.Emplace(KeyCooldown, Label, 9.0f, true);
        Current.Emplace(KeyDamage, Label, 6.0f);
        Current.Emplace(KeyArmor, Label, 5.0f);
        Current.Emplace(KeyCooldown, Label, 6.0f, true);
        const TArray<FAttributeDiff> Diffs = Core::ComputeDiffs(New, Current);
        TestEqual(TEXT("three rows"), Diffs.Num(), 3);
        TestEqual(TEXT("net score: +1 (damage) + 0 (armor) - 1 (cooldown) = 0"), Core::ComputeUpgradeScore(Diffs), 0);

        TArray<FMythicComparableStat> BetterNew = New;
        BetterNew[2].Value = 3.0f;
        const TArray<FAttributeDiff> BetterDiffs = Core::ComputeDiffs(BetterNew, Current);
        TestEqual(TEXT("net score: two upgrades, one neutral = +2"), Core::ComputeUpgradeScore(BetterDiffs), 2);

        TestEqual(TEXT("no diffs -> score 0"), Core::ComputeUpgradeScore(TArray<FAttributeDiff>()), 0);

        const TArray<FAttributeDiff> VsEmpty = Core::ComputeDiffs(New, TArray<FMythicComparableStat>());
        TestEqual(TEXT("vs empty: rows for every new stat"), VsEmpty.Num(), 3);
        TestEqual(TEXT("vs empty: everything (with a real value) counts up"), Core::ComputeUpgradeScore(VsEmpty), 1 + 1 - 1);
    }

    return true;
}

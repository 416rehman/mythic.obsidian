
#include "Misc/AutomationTest.h"
#include "Itemization/Storage/MythicContainerStock.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicContainerStockTest,
    "Mythic.Itemization.ContainerStock",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

namespace {
    MythicContainerStock::FStockEntry MakeStockEntry(float Override, int32 Min = 1, int32 Max = 1, bool bStackable = false) {
        MythicContainerStock::FStockEntry E;
        E.OverrideDropChance = Override;
        E.StackMin = Min;
        E.StackMax = Max;
        E.bStackable = bStackable;
        return E;
    }
}

bool FMythicContainerStockTest::RunTest(const FString &Parameters) {
    TArray<MythicContainerStock::FStockRoll> Rolls;

    {
        FRandomStream S(1);
        MythicContainerStock::RollStock({}, 1.0f, 10, 1.0f, S, Rolls);
        TestEqual(TEXT("no entries -> no rolls"), Rolls.Num(), 0);

        TArray<MythicContainerStock::FStockEntry> One = {MakeStockEntry(1.0f)};
        MythicContainerStock::RollStock(One, 1.0f, 0, 1.0f, S, Rolls);
        TestEqual(TEXT("MaxItems 0 -> no rolls"), Rolls.Num(), 0);

        MythicContainerStock::RollStock(One, 0.0f, 10, 1.0f, S, Rolls);
        TestEqual(TEXT("table DropChance 0 never procs"), Rolls.Num(), 0);
    }

    {
        TArray<MythicContainerStock::FStockEntry> Entries;
        for (int32 i = 0; i < 6; ++i) {
            Entries.Add(MakeStockEntry(1.0f));
        }

        for (int32 Seed = 1; Seed <= 50; ++Seed) {
            FRandomStream S(Seed);
            MythicContainerStock::RollStock(Entries, 1.0f, 3, 1.0f, S, Rolls);

            TestTrue(TEXT("certain table always yields at least one roll"), Rolls.Num() >= 1);
            TestTrue(TEXT("never exceeds MaxItems"), Rolls.Num() <= 3);

            TSet<int32> Seen;
            for (const MythicContainerStock::FStockRoll &R : Rolls) {
                TestTrue(TEXT("entry index in range"), Entries.IsValidIndex(R.EntryIndex));
                TestFalse(TEXT("no entry is picked twice in one pass"), Seen.Contains(R.EntryIndex));
                Seen.Add(R.EntryIndex);
                TestEqual(TEXT("non-stackable entry yields exactly 1"), R.Quantity, 1);
            }
        }
    }

    {
        TArray<MythicContainerStock::FStockEntry> Two = {MakeStockEntry(1.0f), MakeStockEntry(1.0f)};
        for (int32 Seed = 1; Seed <= 25; ++Seed) {
            FRandomStream S(Seed);
            MythicContainerStock::RollStock(Two, 1.0f, 99, 1.0f, S, Rolls);
            TestTrue(TEXT("cannot roll more lines than there are winners"), Rolls.Num() <= 2);
        }
    }

    {
        TArray<MythicContainerStock::FStockEntry> Mixed = {MakeStockEntry(0.0001f), MakeStockEntry(1.0f)};
        int32 OverriddenSeen = 0;
        int32 DefaultSeen = 0;
        for (int32 Seed = 1; Seed <= 200; ++Seed) {
            FRandomStream S(Seed);
            MythicContainerStock::RollStock(Mixed, 1.0f, 5, 1.0f, S, Rolls);
            for (const MythicContainerStock::FStockRoll &R : Rolls) {
                (R.EntryIndex == 0 ? OverriddenSeen : DefaultSeen)++;
            }
        }
        TestTrue(TEXT("a tiny override is respected instead of falling back to DefaultChance 1.0"),
                 OverriddenSeen * 10 < DefaultSeen);

        TArray<MythicContainerStock::FStockEntry> Forced = {MakeStockEntry(1.0f)};
        FRandomStream S(7);
        MythicContainerStock::RollStock(Forced, 1.0f, 5, 0.0f, S, Rolls);
        TestEqual(TEXT("a certain override survives DefaultChance 0"), Rolls.Num(), 1);
    }

    {
        TArray<MythicContainerStock::FStockEntry> Stacked = {MakeStockEntry(1.0f, 3, 7, true)};
        for (int32 Seed = 1; Seed <= 40; ++Seed) {
            FRandomStream S(Seed);
            MythicContainerStock::RollStock(Stacked, 1.0f, 1, 1.0f, S, Rolls);
            TestEqual(TEXT("stackable entry rolled"), Rolls.Num(), 1);
            TestTrue(TEXT("quantity within the authored range"), Rolls[0].Quantity >= 3 && Rolls[0].Quantity <= 7);
        }

        TArray<MythicContainerStock::FStockEntry> Inverted = {MakeStockEntry(1.0f, 9, 2, true)};
        FRandomStream S(3);
        MythicContainerStock::RollStock(Inverted, 1.0f, 1, 1.0f, S, Rolls);
        TestEqual(TEXT("inverted range still rolls"), Rolls.Num(), 1);
        TestTrue(TEXT("inverted range clamps into [2,9]"), Rolls[0].Quantity >= 2 && Rolls[0].Quantity <= 9);

        TArray<MythicContainerStock::FStockEntry> Zeroed = {MakeStockEntry(1.0f, 0, 0, true)};
        FRandomStream S2(4);
        MythicContainerStock::RollStock(Zeroed, 1.0f, 1, 1.0f, S2, Rolls);
        TestEqual(TEXT("zero range still yields one item"), Rolls[0].Quantity, 1);
    }

    {
        TArray<MythicContainerStock::FStockEntry> Entries = {
            MakeStockEntry(0.0f, 1, 4, true), MakeStockEntry(0.0f), MakeStockEntry(0.0f), MakeStockEntry(0.0f)
        };
        TArray<MythicContainerStock::FStockRoll> A, B;
        FRandomStream S1(12345);
        FRandomStream S2(12345);
        MythicContainerStock::RollStock(Entries, 0.8f, 3, 0.5f, S1, A);
        MythicContainerStock::RollStock(Entries, 0.8f, 3, 0.5f, S2, B);
        TestEqual(TEXT("same seed -> same roll count"), A.Num(), B.Num());
        for (int32 i = 0; i < A.Num(); ++i) {
            TestEqual(TEXT("same seed -> same entry"), A[i].EntryIndex, B[i].EntryIndex);
            TestEqual(TEXT("same seed -> same quantity"), A[i].Quantity, B[i].Quantity);
        }
    }

    {
        TArray<MythicContainerStock::FStockEntry> One = {MakeStockEntry(1.0f)};
        FRandomStream S(11);
        MythicContainerStock::RollStock(One, 1.0f, 1, 1.0f, S, Rolls);
        TestEqual(TEXT("first pass yields one"), Rolls.Num(), 1);
        MythicContainerStock::RollStock(One, 0.0f, 1, 1.0f, S, Rolls);
        TestEqual(TEXT("a failed pass clears the previous result"), Rolls.Num(), 0);
    }

    return true;
}

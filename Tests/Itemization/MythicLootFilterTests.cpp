
#include "Misc/AutomationTest.h"
#include "Itemization/Inventory/MythicLootFilter.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicLootFilterTest,
    "Mythic.Itemization.LootFilter",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicLootFilterTest::RunTest(const FString &Parameters) {
    using namespace MythicLootFilter;

    constexpr int32 Common = 0, Rare = 1, Legendary = 3;
    const int32 Ceiling = DefaultMaxJunkRarity;

    TestTrue(TEXT("common, valued, takeable -> auto-junk"),
             QualifiesAsAutoJunk(Common, Ceiling, 5, false, false, true));

    TestFalse(TEXT("rare is above the junk ceiling -> not auto-junk"),
              QualifiesAsAutoJunk(Rare, Ceiling, 5, false, false, true));
    TestFalse(TEXT("legendary is never auto-junk"),
              QualifiesAsAutoJunk(Legendary, Ceiling, 999, false, false, true));

    TestFalse(TEXT("currency -> not auto-junk"),
              QualifiesAsAutoJunk(Common, Ceiling, 5, true, false, true));

    TestFalse(TEXT("equipped -> not auto-junk"),
              QualifiesAsAutoJunk(Common, Ceiling, 5, false, true, true));

    TestFalse(TEXT("not player-takeable -> not auto-junk"),
              QualifiesAsAutoJunk(Common, Ceiling, 5, false, false, false));

    TestFalse(TEXT("worthless (value 0) -> not auto-junk"),
              QualifiesAsAutoJunk(Common, Ceiling, 0, false, false, true));
    TestFalse(TEXT("negative value -> not auto-junk"),
              QualifiesAsAutoJunk(Common, Ceiling, -10, false, false, true));

    TestTrue(TEXT("rare auto-junks when the ceiling is raised to rare"),
             QualifiesAsAutoJunk(Rare, Rare, 5, false, false, true));

    TestTrue(TEXT("manual mark forces junk on a high-rarity item"),
             IsJunk( true, Legendary, Ceiling, 999, false, false, true));
    TestTrue(TEXT("manual mark forces junk even on currency"),
             IsJunk(true, Common, Ceiling, 5, true, false, true));
    TestTrue(TEXT("manual mark forces junk even on equipped gear"),
             IsJunk(true, Common, Ceiling, 5, false, true, true));
    TestTrue(TEXT("unmarked common falls back to auto-junk (true)"),
             IsJunk(false, Common, Ceiling, 5, false, false, true));
    TestFalse(TEXT("unmarked rare falls back to auto-junk (false)"),
              IsJunk(false, Rare, Ceiling, 5, false, false, true));

    {
        const FMythicSellAllPlan Empty = ComputeSellAllJunkPlan(TArray<FMythicSellAllSlot>{});
        TestEqual(TEXT("empty inventory -> no slots"), Empty.SlotsToSell.Num(), 0);
        TestEqual(TEXT("empty inventory -> 0 gold"), Empty.TotalValue, 0);
    }
    {
        TArray<FMythicSellAllSlot> Slots;
        Slots.Add({ 0, true, true, 10, 3});
        Slots.Add({1, false, true, 100, 1});
        Slots.Add({2, true, false, 50, 2});
        Slots.Add({3, true, true, 5, 4});
        Slots.Add({4, false, false, 999, 9});

        const FMythicSellAllPlan Plan = ComputeSellAllJunkPlan(Slots);
        TestEqual(TEXT("only the 2 junk+sellable slots are selected"), Plan.SlotsToSell.Num(), 2);
        TestEqual(TEXT("selection preserves order: first is slot 0"), Plan.SlotsToSell[0], 0);
        TestEqual(TEXT("selection preserves order: second is slot 3"), Plan.SlotsToSell[1], 3);
        TestEqual(TEXT("at-value gold sum = 10*3 + 5*4 = 50"), Plan.TotalValue, 50);
    }
    {
        TArray<FMythicSellAllSlot> Slots;
        Slots.Add({0, true, true, -10, 5});
        Slots.Add({1, true, true, 7, 0});
        const FMythicSellAllPlan Plan = ComputeSellAllJunkPlan(Slots);
        TestEqual(TEXT("both selected"), Plan.SlotsToSell.Num(), 2);
        TestEqual(TEXT("non-positive value/qty never make the total negative"), Plan.TotalValue, 0);
    }

    return true;
}

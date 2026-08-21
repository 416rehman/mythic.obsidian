
#include "Misc/AutomationTest.h"
#include "Itemization/Vendor/MythicVendor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicVendorBuybackTest,
    "Mythic.Itemization.VendorBuyback",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicVendorBuybackTest::RunTest(const FString &Parameters) {
    using V = AMythicVendor;

    TestTrue(TEXT("single unit sold/bought whole (1 of 1)"), V::TradeConsumesWholeStack(1, 1));
    TestTrue(TEXT("exact whole stack (3 of 3)"), V::TradeConsumesWholeStack(3, 3));
    TestTrue(TEXT("plan >= stacks is whole (5 vs 3)"), V::TradeConsumesWholeStack(5, 3));
    TestFalse(TEXT("partial stack is NOT whole (2 of 3) -> re-mint branch"), V::TradeConsumesWholeStack(2, 3));
    TestFalse(TEXT("partial by one (2 of 3)"), V::TradeConsumesWholeStack(2, 3));
    TestFalse(TEXT("non-positive plan -> false"), V::TradeConsumesWholeStack(0, 3));
    TestFalse(TEXT("negative plan -> false"), V::TradeConsumesWholeStack(-1, 3));
    TestFalse(TEXT("no stacks available -> false"), V::TradeConsumesWholeStack(3, 0));
    TestFalse(TEXT("both non-positive -> false"), V::TradeConsumesWholeStack(0, 0));

    TestTrue(TEXT("index 0 in a ring of 3"), V::IsValidBuybackIndex(0, 3));
    TestTrue(TEXT("last valid index 2 in a ring of 3"), V::IsValidBuybackIndex(2, 3));
    TestFalse(TEXT("index == Num is out of range"), V::IsValidBuybackIndex(3, 3));
    TestFalse(TEXT("index > Num is out of range"), V::IsValidBuybackIndex(9, 3));
    TestFalse(TEXT("negative index is invalid"), V::IsValidBuybackIndex(-1, 3));
    TestFalse(TEXT("any index into an empty ring is invalid"), V::IsValidBuybackIndex(0, 0));

    TestEqual(TEXT("first push -> slot 0"), V::BuybackRingWriteSlot(0, 4), 0);
    TestEqual(TEXT("second push -> slot 1"), V::BuybackRingWriteSlot(1, 4), 1);
    TestEqual(TEXT("fills to the last slot (push 3 -> slot 3)"), V::BuybackRingWriteSlot(3, 4), 3);
    TestEqual(TEXT("push 4 wraps to slot 0 (evicts the oldest)"), V::BuybackRingWriteSlot(4, 4), 0);
    TestEqual(TEXT("push 5 -> slot 1"), V::BuybackRingWriteSlot(5, 4), 1);
    TestEqual(TEXT("push 8 -> slot 0 again (two full wraps)"), V::BuybackRingWriteSlot(8, 4), 0);
    TestEqual(TEXT("capacity 1 always writes slot 0"), V::BuybackRingWriteSlot(5, 1), 0);
    TestEqual(TEXT("degenerate capacity 0 -> slot 0 (guard)"), V::BuybackRingWriteSlot(3, 0), 0);
    TestEqual(TEXT("negative push counter still wraps non-negative"), V::BuybackRingWriteSlot(-1, 4), 3);

    {
        const int32 Capacity = 4;
        TArray<int32> WrittenSlots;
        for (int32 Push = 0; Push < Capacity + 2; ++Push) {
            WrittenSlots.Add(V::BuybackRingWriteSlot(Push, Capacity));
        }
        const TArray<int32> Expected = {0, 1, 2, 3, 0, 1};
        TestEqual(TEXT("6 pushes into a 4-slot ring: slots 0 and 1 are recycled (oldest evicted)"), WrittenSlots, Expected);
        TestEqual(TEXT("push 4 evicts the slot push 0 used"), WrittenSlots[4], WrittenSlots[0]);
        TestEqual(TEXT("push 5 evicts the slot push 1 used"), WrittenSlots[5], WrittenSlots[1]);
    }

    return true;
}

// Mythic — vendor pricing / economy unit tests.
// Covers the pure currency decisions the server-authoritative vendor transaction is built on (ComputeBuyPrice,
// ComputeSalePrice, CanAfford, ComputeBalanceAfterSpend) — including the "no money pump" economic-integrity check.
// Run via: Session Frontend → Automation → Mythic.Itemization.Vendor

#include "Misc/AutomationTest.h"
#include "Itemization/Inventory/MythicCurrency.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicVendorPricingTest,
    "Mythic.Itemization.Vendor.Pricing",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicVendorPricingTest::RunTest(const FString &Parameters) {
    using namespace MythicCurrency;

    // ── Buy price: per-unit ceil(Value * Multiplier), then × Quantity ─────────────────────────────────
    TestEqual(TEXT("buy at value, mult 1.0"), ComputeBuyPrice(10, 3, 1.0f), 30);
    // ceil(10 * 1.25) = ceil(12.5) = 13, × 2 = 26
    TestEqual(TEXT("buy margin rounds the per-unit price UP"), ComputeBuyPrice(10, 2, 1.25f), 26);
    // bulk must equal that many single buys (no buy-one-at-a-time rounding exploit)
    TestEqual(TEXT("bulk == repeated single buys"), ComputeBuyPrice(7, 5, 1.1f), ComputeBuyPrice(7, 1, 1.1f) * 5);
    TestEqual(TEXT("unpriced item (Value 0) is not for sale"), ComputeBuyPrice(0, 5, 1.25f), 0);
    TestEqual(TEXT("non-positive quantity -> 0"), ComputeBuyPrice(10, 0, 1.25f), 0);
    TestEqual(TEXT("non-positive quantity (negative) -> 0"), ComputeBuyPrice(10, -3, 1.25f), 0);
    TestEqual(TEXT("zero multiplier -> 0 (free; vendor rejects as not-for-sale)"), ComputeBuyPrice(10, 3, 0.0f), 0);
    TestEqual(TEXT("negative multiplier clamps to 0"), ComputeBuyPrice(10, 3, -2.0f), 0);

    // ── Sale price: floor(Value * Quantity * clamp(SellRate)) ─────────────────────────────────────────
    TestEqual(TEXT("sell half value, floored"), ComputeSalePrice(10, 3, 0.5f), 15);
    // 7 * 1 * 0.4 = 2.8 -> floor 2
    TestEqual(TEXT("sell floors the fractional coin"), ComputeSalePrice(7, 1, 0.4f), 2);
    TestEqual(TEXT("sell rate clamps to 1 (never above value)"), ComputeSalePrice(10, 1, 2.0f), 10);
    TestEqual(TEXT("sell rate clamps to 0 (negative)"), ComputeSalePrice(10, 1, -1.0f), 0);
    TestEqual(TEXT("worthless / unsellable item -> 0"), ComputeSalePrice(0, 5, 0.5f), 0);

    // ── Economic integrity: at sane knobs, buy price > sell price, so a player can't pump money by ─────
    //    buying from and reselling to the same vendor.
    const int32 Buy = ComputeBuyPrice(100, 1, 1.25f);  // 125
    const int32 Sell = ComputeSalePrice(100, 1, 0.4f); // 40
    TestTrue(TEXT("buy > sell at default margins (no money pump)"), Buy > Sell);

    // ── Affordability + spend ─────────────────────────────────────────────────────────────────────────
    TestTrue(TEXT("can afford the exact price"), CanAfford(125, 125));
    TestFalse(TEXT("cannot afford one short"), CanAfford(124, 125));
    TestEqual(TEXT("balance after an affordable spend"), ComputeBalanceAfterSpend(200, 125), 75);
    TestEqual(TEXT("an unaffordable spend is a no-op (never goes negative)"), ComputeBalanceAfterSpend(100, 125), 100);
    TestEqual(TEXT("a free spend (price 0) is a no-op"), ComputeBalanceAfterSpend(50, 0), 50);

    return true;
}

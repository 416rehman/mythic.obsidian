// Mythic — vendor pricing / economy unit tests.
// Covers the pure currency decisions the server-authoritative vendor transaction is built on (ComputeBuyPrice,
// ComputeSalePrice, CanAfford, ComputeBalanceAfterSpend) — including the "no money pump" economic-integrity check.
// Run via: Session Frontend → Automation → Mythic.Itemization.Vendor

#include "Misc/AutomationTest.h"
#include "Itemization/Inventory/MythicCurrency.h"
#include "Itemization/Inventory/MythicTrade.h"

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

// ─── Trade-decision outcomes (PlanBuy / PlanSell): the pure brain the server executes + the callout reads ───
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicVendorTradePlanTest,
    "Mythic.Itemization.Vendor.TradePlan",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicVendorTradePlanTest::RunTest(const FString &Parameters) {
    using namespace MythicTrade;

    // ── BUY: PlanBuy(RequestedQty, UnitValue, PriceMultiplier, AvailableStock, BuyerCurrency, bHasDeliveryTarget) ──
    {
        // Full success: want 3, priced 10@1.0 = 30 each-3, stock 5, wallet 100, has room.
        const FMythicTradePlan P = PlanBuy(3, 10, 1.0f, 5, 100, true);
        TestEqual(TEXT("buy full: result Success"), P.Result, EMythicTradeResult::Success);
        TestEqual(TEXT("buy full: qty 3"), P.Quantity, 3);
        TestEqual(TEXT("buy full: price 30"), P.TotalPrice, 30);
    }
    {
        // Stock-limited partial: want 10, only 4 in stock, wallet ample.
        const FMythicTradePlan P = PlanBuy(10, 10, 1.0f, 4, 1000, true);
        TestEqual(TEXT("buy partial stock: PartialStock"), P.Result, EMythicTradeResult::PartialStock);
        TestEqual(TEXT("buy partial stock: qty 4"), P.Quantity, 4);
    }
    {
        // Funds-limited partial: want 10, stock 10, wallet 55, unit 10 -> 5 affordable.
        const FMythicTradePlan P = PlanBuy(10, 10, 1.0f, 10, 55, true);
        TestEqual(TEXT("buy partial funds: PartialFunds"), P.Result, EMythicTradeResult::PartialFunds);
        TestEqual(TEXT("buy partial funds: qty 5"), P.Quantity, 5);
        TestEqual(TEXT("buy partial funds: price 50"), P.TotalPrice, 50);
    }
    {
        // Can't afford even one.
        const FMythicTradePlan P = PlanBuy(1, 100, 1.0f, 5, 50, true);
        TestEqual(TEXT("buy broke: InsufficientFunds"), P.Result, EMythicTradeResult::InsufficientFunds);
        TestEqual(TEXT("buy broke: qty 0"), P.Quantity, 0);
    }
    {
        const FMythicTradePlan P = PlanBuy(1, 10, 1.0f, 0, 1000, true);
        TestEqual(TEXT("buy no stock: OutOfStock"), P.Result, EMythicTradeResult::OutOfStock);
    }
    {
        const FMythicTradePlan P = PlanBuy(1, 10, 1.0f, 5, 1000, false);
        TestEqual(TEXT("buy no room: NoRoom"), P.Result, EMythicTradeResult::NoRoom);
    }
    {
        const FMythicTradePlan P = PlanBuy(1, 0, 1.25f, 5, 1000, true);
        TestEqual(TEXT("buy unpriced: NotForSale"), P.Result, EMythicTradeResult::NotForSale);
    }
    {
        const FMythicTradePlan P = PlanBuy(0, 10, 1.0f, 5, 1000, true);
        TestEqual(TEXT("buy zero qty: InvalidRequest"), P.Result, EMythicTradeResult::InvalidRequest);
    }

    // ── SELL: PlanSell(RequestedQty, AvailableStacks, UnitValue, SellRate, bHasCurrencyDef, bCanTake, bIsCurrencyItem) ──
    {
        // Full sale: 3 of value 10 at 0.5 = 15 proceeds.
        const FMythicTradePlan P = PlanSell(3, 5, 10, 0.5f, /*currencyDef*/ true, /*canTake*/ true, /*isCurrency*/ false);
        TestEqual(TEXT("sell full: Success"), P.Result, EMythicTradeResult::Success);
        TestEqual(TEXT("sell full: qty 3"), P.Quantity, 3);
        TestEqual(TEXT("sell full: proceeds 15"), P.TotalPrice, 15);
    }
    {
        // Stack-limited partial: want 10, only 2 held.
        const FMythicTradePlan P = PlanSell(10, 2, 10, 0.5f, true, true, false);
        TestEqual(TEXT("sell partial: PartialStock"), P.Result, EMythicTradeResult::PartialStock);
        TestEqual(TEXT("sell partial: qty 2"), P.Quantity, 2);
    }
    {
        const FMythicTradePlan P = PlanSell(1, 5, 10, 0.5f, /*currencyDef*/ false, true, false);
        TestEqual(TEXT("sell no currency def: VendorCannotPay"), P.Result, EMythicTradeResult::VendorCannotPay);
    }
    {
        const FMythicTradePlan P = PlanSell(1, 5, 10, 0.5f, true, /*canTake*/ false, false);
        TestEqual(TEXT("sell bound/equipped: NotSellable"), P.Result, EMythicTradeResult::NotSellable);
    }
    {
        const FMythicTradePlan P = PlanSell(1, 5, 10, 0.5f, true, true, /*isCurrency*/ true);
        TestEqual(TEXT("sell currency item: NotSellable"), P.Result, EMythicTradeResult::NotSellable);
    }
    {
        const FMythicTradePlan P = PlanSell(1, 5, 0, 0.5f, true, true, false);
        TestEqual(TEXT("sell worthless (value 0): NotSellable"), P.Result, EMythicTradeResult::NotSellable);
        TestEqual(TEXT("sell worthless: qty 0"), P.Quantity, 0);
    }

    // ── Failure classification + messages ──────────────────────────────────────────────────────────────
    TestTrue(TEXT("InsufficientFunds is a shown failure"), IsFailureWorthShowing(EMythicTradeResult::InsufficientFunds));
    TestTrue(TEXT("NotForSale is a shown failure"), IsFailureWorthShowing(EMythicTradeResult::NotForSale));
    TestFalse(TEXT("Success is not a failure callout"), IsFailureWorthShowing(EMythicTradeResult::Success));
    TestFalse(TEXT("PartialStock is not a failure callout"), IsFailureWorthShowing(EMythicTradeResult::PartialStock));
    TestFalse(TEXT("PartialFunds is not a failure callout"), IsFailureWorthShowing(EMythicTradeResult::PartialFunds));
    TestFalse(TEXT("InvalidRequest stays silent"), IsFailureWorthShowing(EMythicTradeResult::InvalidRequest));
    TestFalse(TEXT("a shown failure has a non-empty message"), DescribeResult(EMythicTradeResult::InsufficientFunds).IsEmpty());
    TestTrue(TEXT("Success has no message"), DescribeResult(EMythicTradeResult::Success).IsEmpty());

    return true;
}

// ─── Repair pricing + decision (ComputeRepairCost / PlanRepair): the blacksmith currency-repair brain ───
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicVendorRepairTest,
    "Mythic.Itemization.Vendor.Repair",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicVendorRepairTest::RunTest(const FString &Parameters) {
    using namespace MythicCurrency;
    using namespace MythicTrade;

    // ── ComputeRepairCost(Current, Max, Value, Fraction) = ceil(Value * (Missing/Max) * Fraction) ──
    TestEqual(TEXT("fully broken costs Fraction*Value"), ComputeRepairCost(0, 100, 100, 0.5f), 50);
    TestEqual(TEXT("half worn costs half of that"), ComputeRepairCost(50, 100, 100, 0.5f), 25);
    TestEqual(TEXT("repair cost rounds up the fractional coin"), ComputeRepairCost(90, 100, 33, 0.5f), 2); // 33*0.1*0.5=1.65 -> 2
    TestEqual(TEXT("already full -> 0"), ComputeRepairCost(100, 100, 100, 0.5f), 0);
    TestEqual(TEXT("valueless item -> 0 (free)"), ComputeRepairCost(0, 100, 0, 0.5f), 0);
    TestEqual(TEXT("no durability (max 0) -> 0"), ComputeRepairCost(0, 0, 100, 0.5f), 0);
    TestEqual(TEXT("zero fraction -> 0"), ComputeRepairCost(0, 100, 100, 0.0f), 0);

    // ── PlanRepair(Current, Max, Value, Fraction, PayerCurrency) ──
    {
        const FMythicTradePlan P = PlanRepair(0, 100, 100, 0.5f, 200);
        TestEqual(TEXT("repair success result"), P.Result, EMythicTradeResult::Success);
        TestEqual(TEXT("repair restores all missing points"), P.Quantity, 100);
        TestEqual(TEXT("repair charges the cost"), P.TotalPrice, 50);
    }
    {
        const FMythicTradePlan P = PlanRepair(75, 100, 100, 0.5f, 200);
        TestEqual(TEXT("partial-wear success"), P.Result, EMythicTradeResult::Success);
        TestEqual(TEXT("restores 25 missing points"), P.Quantity, 25);
        TestEqual(TEXT("costs ceil(100*0.25*0.5)=13"), P.TotalPrice, 13);
    }
    {
        const FMythicTradePlan P = PlanRepair(100, 100, 100, 0.5f, 200);
        TestEqual(TEXT("already full -> NothingToRepair"), P.Result, EMythicTradeResult::NothingToRepair);
        TestEqual(TEXT("nothing restored"), P.Quantity, 0);
    }
    {
        const FMythicTradePlan P = PlanRepair(0, 0, 100, 0.5f, 200);
        TestEqual(TEXT("no durability -> NothingToRepair"), P.Result, EMythicTradeResult::NothingToRepair);
    }
    {
        const FMythicTradePlan P = PlanRepair(0, 100, 100, 0.5f, 40); // cost 50, only 40 coins
        TestEqual(TEXT("can't afford repair -> InsufficientFunds"), P.Result, EMythicTradeResult::InsufficientFunds);
        TestEqual(TEXT("no repair on reject"), P.Quantity, 0);
    }
    {
        const FMythicTradePlan P = PlanRepair(50, 100, 0, 0.5f, 0); // valueless -> free repair even with 0 coins
        TestEqual(TEXT("free repair of a valueless item succeeds"), P.Result, EMythicTradeResult::Success);
        TestEqual(TEXT("free repair restores missing points"), P.Quantity, 50);
        TestEqual(TEXT("free repair costs 0"), P.TotalPrice, 0);
    }

    TestTrue(TEXT("NothingToRepair is shown to the player"), IsFailureWorthShowing(EMythicTradeResult::NothingToRepair));
    TestFalse(TEXT("NothingToRepair has a message"), DescribeResult(EMythicTradeResult::NothingToRepair).IsEmpty());

    return true;
}

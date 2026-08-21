
#include "Misc/AutomationTest.h"
#include "Itemization/Inventory/MythicCurrency.h"
#include "Itemization/Inventory/MythicTrade.h"
#include "Itemization/Vendor/MythicVendor.h"
#include "Player/MythicFactionStandingComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicVendorPricingTest,
    "Mythic.Itemization.Vendor.Pricing",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicVendorPricingTest::RunTest(const FString &Parameters) {
    using namespace MythicCurrency;

    TestEqual(TEXT("buy at value, mult 1.0"), ComputeBuyPrice(10, 3, 1.0f), 30);
    TestEqual(TEXT("buy margin rounds the per-unit price UP"), ComputeBuyPrice(10, 2, 1.25f), 26);
    TestEqual(TEXT("bulk == repeated single buys"), ComputeBuyPrice(7, 5, 1.1f), ComputeBuyPrice(7, 1, 1.1f) * 5);
    TestEqual(TEXT("unpriced item (Value 0) is not for sale"), ComputeBuyPrice(0, 5, 1.25f), 0);
    TestEqual(TEXT("non-positive quantity -> 0"), ComputeBuyPrice(10, 0, 1.25f), 0);
    TestEqual(TEXT("non-positive quantity (negative) -> 0"), ComputeBuyPrice(10, -3, 1.25f), 0);
    TestEqual(TEXT("zero multiplier -> 0 (free; vendor rejects as not-for-sale)"), ComputeBuyPrice(10, 3, 0.0f), 0);
    TestEqual(TEXT("negative multiplier clamps to 0"), ComputeBuyPrice(10, 3, -2.0f), 0);

    TestEqual(TEXT("sell half value, floored"), ComputeSalePrice(10, 3, 0.5f), 15);
    TestEqual(TEXT("sell floors the fractional coin"), ComputeSalePrice(7, 1, 0.4f), 2);
    TestEqual(TEXT("sell rate clamps to 1 (never above value)"), ComputeSalePrice(10, 1, 2.0f), 10);
    TestEqual(TEXT("sell rate clamps to 0 (negative)"), ComputeSalePrice(10, 1, -1.0f), 0);
    TestEqual(TEXT("worthless / unsellable item -> 0"), ComputeSalePrice(0, 5, 0.5f), 0);

    const int32 Buy = ComputeBuyPrice(100, 1, 1.25f);
    const int32 Sell = ComputeSalePrice(100, 1, 0.4f);
    TestTrue(TEXT("buy > sell at default margins (no money pump)"), Buy > Sell);

    TestTrue(TEXT("can afford the exact price"), CanAfford(125, 125));
    TestFalse(TEXT("cannot afford one short"), CanAfford(124, 125));
    TestEqual(TEXT("balance after an affordable spend"), ComputeBalanceAfterSpend(200, 125), 75);
    TestEqual(TEXT("an unaffordable spend is a no-op (never goes negative)"), ComputeBalanceAfterSpend(100, 125), 100);
    TestEqual(TEXT("a free spend (price 0) is a no-op"), ComputeBalanceAfterSpend(50, 0), 50);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicVendorReputationPricingTest,
    "Mythic.Itemization.Vendor.ReputationPricing",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicVendorReputationPricingTest::RunTest(const FString &Parameters) {
    using namespace MythicCurrency;
    using V = AMythicVendor;
    using ET = EMythicStandingTier;

    TestEqual(TEXT("neutral with all-1.0 mults leaves the base unchanged"), V::ComputeReputationAdjustedMultiplier(1.25f, ET::Neutral, 1.0f, 1.0f, 1.0f), 1.25f);
    TestEqual(TEXT("friendly discount scales the base down"), V::ComputeReputationAdjustedMultiplier(1.25f, ET::Friendly, 1.0f, 1.0f, 0.8f), 1.0f);
    TestEqual(TEXT("hostile surcharge scales the base up"), V::ComputeReputationAdjustedMultiplier(1.25f, ET::Hostile, 1.5f, 1.0f, 0.8f), 1.875f);
    TestEqual(TEXT("a negative tier mult clamps to 0"), V::ComputeReputationAdjustedMultiplier(1.25f, ET::Friendly, 1.0f, 1.0f, -2.0f), 0.0f);

    const float NeutralBuyMult = V::ComputeReputationAdjustedMultiplier(1.25f, ET::Neutral, 1.0f, 1.0f, 0.8f);
    const float FriendlyBuyMult = V::ComputeReputationAdjustedMultiplier(1.25f, ET::Friendly, 1.0f, 1.0f, 0.8f);
    TestTrue(TEXT("friendly buy price < neutral buy price"), ComputeBuyPrice(100, 1, FriendlyBuyMult) < ComputeBuyPrice(100, 1, NeutralBuyMult));

    const float PumpRate = V::ComputeReputationAdjustedMultiplier(0.4f, ET::Friendly, 1.0f, 1.0f, 5.0f);
    TestEqual(TEXT("a runaway friendly sell bonus caps at full value (no money pump)"), ComputeSalePrice(100, 1, PumpRate), 100);

    const float AggressiveBuyMult = V::ComputeReputationAdjustedMultiplier(1.25f, ET::Friendly, 1.0f, 1.0f, 0.3f);
    const float SameTierSellRate = V::ComputeReputationAdjustedMultiplier(0.4f, ET::Friendly, 1.0f, 1.0f, 1.0f);
    const float GuardedBuyMult = V::EnforceBuyAboveSellRate(AggressiveBuyMult, SameTierSellRate);
    const int32 GuardedBuy = ComputeBuyPrice(100, 1, GuardedBuyMult);
    const int32 SameTierSell = ComputeSalePrice(100, 1, SameTierSellRate);
    TestTrue(TEXT("guarded friendly buy price >= same-tier sell payout (no buy-low/resell pump)"), GuardedBuy >= SameTierSell);
    TestEqual(TEXT("the guard lifts the buy mult to exactly the sell rate (0.4), not the raw 0.375"), GuardedBuyMult, 0.4f);
    TestEqual(TEXT("a modest discount above the sell rate is left untouched"), V::EnforceBuyAboveSellRate(1.0f, 0.4f), 1.0f);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicVendorTradePlanTest,
    "Mythic.Itemization.Vendor.TradePlan",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicVendorTradePlanTest::RunTest(const FString &Parameters) {
    using namespace MythicTrade;

    {
        const FMythicTradePlan P = PlanBuy(3, 10, 1.0f, 5, 100, true);
        TestEqual(TEXT("buy full: result Success"), P.Result, EMythicTradeResult::Success);
        TestEqual(TEXT("buy full: qty 3"), P.Quantity, 3);
        TestEqual(TEXT("buy full: price 30"), P.TotalPrice, 30);
    }
    {
        const FMythicTradePlan P = PlanBuy(10, 10, 1.0f, 4, 1000, true);
        TestEqual(TEXT("buy partial stock: PartialStock"), P.Result, EMythicTradeResult::PartialStock);
        TestEqual(TEXT("buy partial stock: qty 4"), P.Quantity, 4);
    }
    {
        const FMythicTradePlan P = PlanBuy(10, 10, 1.0f, 10, 55, true);
        TestEqual(TEXT("buy partial funds: PartialFunds"), P.Result, EMythicTradeResult::PartialFunds);
        TestEqual(TEXT("buy partial funds: qty 5"), P.Quantity, 5);
        TestEqual(TEXT("buy partial funds: price 50"), P.TotalPrice, 50);
    }
    {
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

    {
        const FMythicTradePlan P = PlanSell(3, 5, 10, 0.5f, true, true, false);
        TestEqual(TEXT("sell full: Success"), P.Result, EMythicTradeResult::Success);
        TestEqual(TEXT("sell full: qty 3"), P.Quantity, 3);
        TestEqual(TEXT("sell full: proceeds 15"), P.TotalPrice, 15);
    }
    {
        const FMythicTradePlan P = PlanSell(10, 2, 10, 0.5f, true, true, false);
        TestEqual(TEXT("sell partial: PartialStock"), P.Result, EMythicTradeResult::PartialStock);
        TestEqual(TEXT("sell partial: qty 2"), P.Quantity, 2);
    }
    {
        const FMythicTradePlan P = PlanSell(1, 5, 10, 0.5f, false, true, false);
        TestEqual(TEXT("sell no currency def: VendorCannotPay"), P.Result, EMythicTradeResult::VendorCannotPay);
    }
    {
        const FMythicTradePlan P = PlanSell(1, 5, 10, 0.5f, true, false, false);
        TestEqual(TEXT("sell bound/equipped: NotSellable"), P.Result, EMythicTradeResult::NotSellable);
    }
    {
        const FMythicTradePlan P = PlanSell(1, 5, 10, 0.5f, true, true, true);
        TestEqual(TEXT("sell currency item: NotSellable"), P.Result, EMythicTradeResult::NotSellable);
    }
    {
        const FMythicTradePlan P = PlanSell(1, 5, 0, 0.5f, true, true, false);
        TestEqual(TEXT("sell worthless (value 0): NotSellable"), P.Result, EMythicTradeResult::NotSellable);
        TestEqual(TEXT("sell worthless: qty 0"), P.Quantity, 0);
    }

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicVendorRepairTest,
    "Mythic.Itemization.Vendor.Repair",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicVendorRepairTest::RunTest(const FString &Parameters) {
    using namespace MythicCurrency;
    using namespace MythicTrade;

    TestEqual(TEXT("fully broken costs Fraction*Value"), ComputeRepairCost(0, 100, 100, 0.5f), 50);
    TestEqual(TEXT("half worn costs half of that"), ComputeRepairCost(50, 100, 100, 0.5f), 25);
    TestEqual(TEXT("repair cost rounds up the fractional coin"), ComputeRepairCost(90, 100, 33, 0.5f), 2);
    TestEqual(TEXT("already full -> 0"), ComputeRepairCost(100, 100, 100, 0.5f), 0);
    TestEqual(TEXT("valueless item -> 0 (free)"), ComputeRepairCost(0, 100, 0, 0.5f), 0);
    TestEqual(TEXT("no durability (max 0) -> 0"), ComputeRepairCost(0, 0, 100, 0.5f), 0);
    TestEqual(TEXT("zero fraction -> 0"), ComputeRepairCost(0, 100, 100, 0.0f), 0);

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
        const FMythicTradePlan P = PlanRepair(0, 100, 100, 0.5f, 40);
        TestEqual(TEXT("can't afford repair -> InsufficientFunds"), P.Result, EMythicTradeResult::InsufficientFunds);
        TestEqual(TEXT("no repair on reject"), P.Quantity, 0);
    }
    {
        const FMythicTradePlan P = PlanRepair(50, 100, 0, 0.5f, 0);
        TestEqual(TEXT("free repair of a valueless item succeeds"), P.Result, EMythicTradeResult::Success);
        TestEqual(TEXT("free repair restores missing points"), P.Quantity, 50);
        TestEqual(TEXT("free repair costs 0"), P.TotalPrice, 0);
    }

    TestTrue(TEXT("NothingToRepair is shown to the player"), IsFailureWorthShowing(EMythicTradeResult::NothingToRepair));
    TestFalse(TEXT("NothingToRepair has a message"), DescribeResult(EMythicTradeResult::NothingToRepair).IsEmpty());

    {
        const FMythicTradePlan P = ComputeRepairAllPlan(TArray<int32>{}, 100);
        TestEqual(TEXT("repair-all: nothing damaged -> NothingToRepair"), P.Result, EMythicTradeResult::NothingToRepair);
        TestEqual(TEXT("repair-all: nothing repaired"), P.Quantity, 0);
    }
    {
        const FMythicTradePlan P = ComputeRepairAllPlan(TArray<int32>{10, 20, 30}, 100);
        TestEqual(TEXT("repair-all: affords everything -> Success"), P.Result, EMythicTradeResult::Success);
        TestEqual(TEXT("repair-all: all 3 repaired"), P.Quantity, 3);
        TestEqual(TEXT("repair-all: total 60"), P.TotalPrice, 60);
    }
    {
        const FMythicTradePlan P = ComputeRepairAllPlan(TArray<int32>{10, 20, 30}, 35);
        TestEqual(TEXT("repair-all: partial budget -> Success (subset)"), P.Result, EMythicTradeResult::Success);
        TestEqual(TEXT("repair-all: cheapest 2 repaired"), P.Quantity, 2);
        TestEqual(TEXT("repair-all: spent 30, not over budget"), P.TotalPrice, 30);
    }
    {
        const FMythicTradePlan P = ComputeRepairAllPlan(TArray<int32>{50, 60}, 40);
        TestEqual(TEXT("repair-all: can't afford the cheapest -> InsufficientFunds"), P.Result, EMythicTradeResult::InsufficientFunds);
        TestEqual(TEXT("repair-all: nothing repaired / charged"), P.Quantity, 0);
        TestEqual(TEXT("repair-all: no charge on reject"), P.TotalPrice, 0);
    }
    {
        const FMythicTradePlan P = ComputeRepairAllPlan(TArray<int32>{10, 20, 30}, 60);
        TestEqual(TEXT("repair-all: exact budget repairs all"), P.Quantity, 3);
        TestEqual(TEXT("repair-all: exact spend"), P.TotalPrice, 60);
    }
    {
        const FMythicTradePlan P = ComputeRepairAllPlan(TArray<int32>{MAX_int32, MAX_int32}, MAX_int32);
        TestEqual(TEXT("repair-all: huge costs don't overflow the budget compare"), P.Quantity, 1);
        TestEqual(TEXT("repair-all: charges exactly the one affordable cost"), P.TotalPrice, MAX_int32);
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRerollCostTest,
    "Mythic.Itemization.RerollCost",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRerollCostTest::RunTest(const FString &Parameters) {
    using namespace MythicCurrency;

    TestEqual(TEXT("base cost 0 -> free"), ComputeRerollCost(50, 3, 0, 0.10f, 0.75f), 0);
    TestEqual(TEXT("negative base cost -> free"), ComputeRerollCost(50, 3, -10, 0.10f, 0.75f), 0);

    TestEqual(TEXT("level 0 rarity 0 -> base cost"), ComputeRerollCost(0, 0, 100, 0.10f, 0.75f), 100);

    TestEqual(TEXT("level 10 @ +10%/lvl -> 2x base"), ComputeRerollCost(10, 0, 100, 0.10f, 0.0f), 200);

    TestEqual(TEXT("rarity 2 @ +50%/tier -> 2x base"), ComputeRerollCost(0, 2, 100, 0.0f, 0.5f), 200);

    TestEqual(TEXT("level + rarity multiply"), ComputeRerollCost(10, 1, 100, 0.10f, 0.75f), 350);

    TestEqual(TEXT("reroll cost rounds up"), ComputeRerollCost(1, 0, 33, 0.05f, 0.0f), 35);

    TestEqual(TEXT("negative level & rarity floored -> base"), ComputeRerollCost(-5, -2, 100, 0.10f, 0.75f), 100);

    TestEqual(TEXT("negative fractions clamped -> base"), ComputeRerollCost(10, 5, 100, -1.0f, -1.0f), 100);

    TestTrue(TEXT("higher rarity costs strictly more"),
             ComputeRerollCost(20, 3, 50, 0.10f, 0.75f) > ComputeRerollCost(20, 0, 50, 0.10f, 0.75f));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicVendorTradingXpTest,
    "Mythic.Itemization.Vendor.TradingXp",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicVendorTradingXpTest::RunTest(const FString &Parameters) {
    using V = AMythicVendor;

    TestEqual(TEXT("xp = per-coin * coins"), V::ComputeTradingXpReward(0.5f, 100, 3, 0), 50.0f);
    TestEqual(TEXT("zero per-coin -> 0 (inert default)"), V::ComputeTradingXpReward(0.0f, 100, 3, 0), 0.0f);
    TestEqual(TEXT("negative per-coin -> 0"), V::ComputeTradingXpReward(-1.0f, 100, 3, 0), 0.0f);
    TestEqual(TEXT("zero coins -> 0"), V::ComputeTradingXpReward(0.5f, 0, 3, 0), 0.0f);
    TestEqual(TEXT("negative coins -> 0"), V::ComputeTradingXpReward(0.5f, -10, 3, 0), 0.0f);
    TestEqual(TEXT("at the cap level -> 0"), V::ComputeTradingXpReward(0.5f, 100, 10, 10), 0.0f);
    TestEqual(TEXT("above the cap level -> 0"), V::ComputeTradingXpReward(0.5f, 100, 12, 10), 0.0f);
    TestEqual(TEXT("below the cap pays out"), V::ComputeTradingXpReward(0.5f, 100, 9, 10), 50.0f);
    TestEqual(TEXT("cap 0 = no cap"), V::ComputeTradingXpReward(0.5f, 100, 999, 0), 50.0f);

    return true;
}

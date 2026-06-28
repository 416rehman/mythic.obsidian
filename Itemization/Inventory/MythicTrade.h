// Mythic — pure vendor trade decisions (no engine state) so the buy/sell outcome logic is unit-testable headlessly and
// shared by the server execution + the player-facing failure callout. Mirrors the PlaceableFragment::PlanDeploy pattern:
// the server gathers live inputs (price, stock, wallet, space), this decides the outcome + quantity, the server executes.

#pragma once

#include "CoreMinimal.h"
#include "MythicTrade.generated.h"

/** The outcome of a buy/sell decision — drives both the server mutation and the client failure callout. */
UENUM(BlueprintType)
enum class EMythicTradeResult : uint8 {
    Success,           // traded everything requested
    PartialStock,      // traded some; the vendor ran out of stock
    PartialFunds,      // traded some; the buyer ran out of currency
    OutOfStock,        // traded nothing; the slot is empty / 0 stacks
    InsufficientFunds, // traded nothing; can't afford even one unit
    NoRoom,            // traded nothing; no buyer inventory accepts the item
    NotForSale,        // traded nothing; the item is unpriced (Value 0 / multiplier 0)
    NotSellable,       // traded nothing; worthless, equipped/bound, or a currency item
    VendorCannotPay,   // traded nothing; the vendor has no currency definition assigned
    NothingToRepair,   // repair: the item is already at full durability, or has no durability to repair
    InvalidRequest     // traded nothing; null / non-positive arguments
};

/** Plain (non-reflected) result of a trade decision: the reason + units actually traded + the coin total. */
struct FMythicTradePlan {
    EMythicTradeResult Result = EMythicTradeResult::InvalidRequest;
    int32 Quantity = 0;   // units actually bought/sold (0 on a hard reject)
    int32 TotalPrice = 0; // coins charged (buy) or paid out (sell)
};

namespace MythicTrade {
    // Pure BUY decision. UnitValue + PriceMultiplier are the item's price inputs (priced via MythicCurrency::ComputeBuyPrice
    // internally so a bulk buy == repeated single buys); the rest is gated live state. Clamps the quantity to stock and
    // affordability; reports the binding constraint on a partial fill.
    MYTHIC_API FMythicTradePlan PlanBuy(int32 RequestedQty, int32 UnitValue, float PriceMultiplier, int32 AvailableStock,
                                        int32 BuyerCurrency, bool bHasDeliveryTarget);

    // Pure SELL decision. Proceeds are priced via MythicCurrency::ComputeSalePrice over the clamped quantity. bCanTake is
    // the source slot's player-take rule; bIsCurrencyItem blocks selling currency for currency.
    MYTHIC_API FMythicTradePlan PlanSell(int32 RequestedQty, int32 AvailableStacks, int32 UnitValue, float SellRate,
                                         bool bHasCurrencyDef, bool bCanTake, bool bIsCurrencyItem);

    // Pure REPAIR decision. Cost is priced via MythicCurrency::ComputeRepairCost. On Success, Quantity = the durability
    // points to restore (Max − Current) and TotalPrice = the cost. NothingToRepair when already full / no durability;
    // InsufficientFunds when the payer can't afford it. A 0 cost (valueless item / free) still succeeds and restores.
    MYTHIC_API FMythicTradePlan PlanRepair(int32 CurrentDurability, int32 MaxDurability, int32 ItemValue,
                                           float RepairCostFraction, int32 PayerCurrency);

    // Pure REPAIR-ALL decision: given each damaged item's full-repair cost (POSITIVE, sorted ASCENDING) and the payer's
    // currency, repair the most items the budget allows (cheapest-first greedy prefix). Quantity = item count, TotalPrice =
    // total charged (64-bit accumulation so a huge cost list can't overflow the budget compare). NothingToRepair on an empty
    // list, InsufficientFunds when not even the cheapest is affordable, else Success (possibly a partial subset).
    MYTHIC_API FMythicTradePlan ComputeRepairAllPlan(const TArray<int32> &CostsAscending, int32 PayerCurrency);

    // True when a result is a hard reject (Quantity always 0) worth a player-facing failure callout. Successes and partial
    // fills are NOT failures (the "+N" pickup callout already covers them). InvalidRequest stays silent (a bad-input edge).
    MYTHIC_API bool IsFailureWorthShowing(EMythicTradeResult Result);

    // Player-facing message for a result (empty for successes/partials/InvalidRequest — nothing to show).
    MYTHIC_API FText DescribeResult(EMythicTradeResult Result);
}

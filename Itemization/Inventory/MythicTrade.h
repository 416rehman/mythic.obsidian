
#pragma once

#include "CoreMinimal.h"
#include "MythicTrade.generated.h"

UENUM(BlueprintType)
enum class EMythicTradeResult : uint8 {
    Success,
    PartialStock,
    PartialFunds,
    OutOfStock,
    InsufficientFunds,
    NoRoom,
    NotForSale,
    NotSellable,
    VendorCannotPay,
    NothingToRepair,
    InvalidRequest,
    RequiresStation
};

struct FMythicTradePlan {
    EMythicTradeResult Result = EMythicTradeResult::InvalidRequest;
    int32 Quantity = 0;
    int32 TotalPrice = 0;
};

namespace MythicTrade {
    MYTHIC_API FMythicTradePlan PlanBuy(int32 RequestedQty, int32 UnitValue, float PriceMultiplier, int32 AvailableStock,
                                        int32 BuyerCurrency, bool bHasDeliveryTarget);

    MYTHIC_API FMythicTradePlan PlanSell(int32 RequestedQty, int32 AvailableStacks, int32 UnitValue, float SellRate,
                                         bool bHasCurrencyDef, bool bCanTake, bool bIsCurrencyItem);

    MYTHIC_API FMythicTradePlan PlanRepair(int32 CurrentDurability, int32 MaxDurability, int32 ItemValue,
                                           float RepairCostFraction, int32 PayerCurrency);

    MYTHIC_API FMythicTradePlan ComputeRepairAllPlan(const TArray<int32> &CostsAscending, int32 PayerCurrency);

    MYTHIC_API bool IsFailureWorthShowing(EMythicTradeResult Result);

    MYTHIC_API FText DescribeResult(EMythicTradeResult Result);
}

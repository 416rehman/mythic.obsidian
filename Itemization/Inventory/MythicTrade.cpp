
#include "MythicTrade.h"

#include "MythicCurrency.h"

#define LOCTEXT_NAMESPACE "MythicTrade"

namespace MythicTrade {
    FMythicTradePlan PlanBuy(int32 RequestedQty, int32 UnitValue, float PriceMultiplier, int32 AvailableStock,
                             int32 BuyerCurrency, bool bHasDeliveryTarget) {
        FMythicTradePlan Plan;
        if (RequestedQty <= 0) {
            Plan.Result = EMythicTradeResult::InvalidRequest;
            return Plan;
        }
        const int32 UnitPrice = MythicCurrency::ComputeBuyPrice(UnitValue, 1, PriceMultiplier);
        if (UnitPrice <= 0) {
            Plan.Result = EMythicTradeResult::NotForSale;
            return Plan;
        }
        if (AvailableStock <= 0) {
            Plan.Result = EMythicTradeResult::OutOfStock;
            return Plan;
        }
        if (!bHasDeliveryTarget) {
            Plan.Result = EMythicTradeResult::NoRoom;
            return Plan;
        }
        const int32 AffordableQty = BuyerCurrency / UnitPrice;
        if (AffordableQty <= 0) {
            Plan.Result = EMythicTradeResult::InsufficientFunds;
            return Plan;
        }

        const int32 Qty = FMath::Min3(RequestedQty, AvailableStock, AffordableQty);
        Plan.Quantity = Qty;
        Plan.TotalPrice = UnitPrice * Qty;
        if (Qty < RequestedQty) {
            Plan.Result = (AffordableQty < AvailableStock) ? EMythicTradeResult::PartialFunds : EMythicTradeResult::PartialStock;
        }
        else {
            Plan.Result = EMythicTradeResult::Success;
        }
        return Plan;
    }

    FMythicTradePlan PlanSell(int32 RequestedQty, int32 AvailableStacks, int32 UnitValue, float SellRate, bool bHasCurrencyDef,
                              bool bCanTake, bool bIsCurrencyItem) {
        FMythicTradePlan Plan;
        if (RequestedQty <= 0) {
            Plan.Result = EMythicTradeResult::InvalidRequest;
            return Plan;
        }
        if (!bHasCurrencyDef) {
            Plan.Result = EMythicTradeResult::VendorCannotPay;
            return Plan;
        }
        if (bIsCurrencyItem || !bCanTake) {
            Plan.Result = EMythicTradeResult::NotSellable;
            return Plan;
        }
        if (AvailableStacks <= 0) {
            Plan.Result = EMythicTradeResult::OutOfStock;
            return Plan;
        }

        const int32 Qty = FMath::Min(RequestedQty, AvailableStacks);
        const int32 Proceeds = MythicCurrency::ComputeSalePrice(UnitValue, Qty, SellRate);
        if (Proceeds <= 0) {
            Plan.Result = EMythicTradeResult::NotSellable;
            return Plan;
        }

        Plan.Quantity = Qty;
        Plan.TotalPrice = Proceeds;
        Plan.Result = (Qty < RequestedQty) ? EMythicTradeResult::PartialStock : EMythicTradeResult::Success;
        return Plan;
    }

    FMythicTradePlan PlanRepair(int32 CurrentDurability, int32 MaxDurability, int32 ItemValue, float RepairCostFraction,
                                int32 PayerCurrency) {
        FMythicTradePlan Plan;
        if (MaxDurability <= 0) {
            Plan.Result = EMythicTradeResult::NothingToRepair;
            return Plan;
        }
        const int32 Missing = FMath::Clamp(MaxDurability - CurrentDurability, 0, MaxDurability);
        if (Missing <= 0) {
            Plan.Result = EMythicTradeResult::NothingToRepair;
            return Plan;
        }
        const int32 Cost = MythicCurrency::ComputeRepairCost(CurrentDurability, MaxDurability, ItemValue, RepairCostFraction);
        if (Cost > 0 && !MythicCurrency::CanAfford(PayerCurrency, Cost)) {
            Plan.Result = EMythicTradeResult::InsufficientFunds;
            return Plan;
        }
        Plan.Result = EMythicTradeResult::Success;
        Plan.Quantity = Missing;
        Plan.TotalPrice = Cost;
        return Plan;
    }

    FMythicTradePlan ComputeRepairAllPlan(const TArray<int32> &CostsAscending, int32 PayerCurrency) {
        FMythicTradePlan Plan;
        if (CostsAscending.Num() == 0) {
            Plan.Result = EMythicTradeResult::NothingToRepair;
            return Plan;
        }
        int64 Running = 0;
        int32 Count = 0;
        for (const int32 Cost : CostsAscending) {
            if (Running + static_cast<int64>(Cost) > static_cast<int64>(PayerCurrency)) {
                break;
            }
            Running += Cost;
            ++Count;
        }
        Plan.Quantity = Count;
        Plan.TotalPrice = static_cast<int32>(Running);
        Plan.Result = (Count > 0) ? EMythicTradeResult::Success : EMythicTradeResult::InsufficientFunds;
        return Plan;
    }

    bool IsFailureWorthShowing(EMythicTradeResult Result) {
        switch (Result) {
            case EMythicTradeResult::InsufficientFunds:
            case EMythicTradeResult::OutOfStock:
            case EMythicTradeResult::NoRoom:
            case EMythicTradeResult::NotForSale:
            case EMythicTradeResult::NotSellable:
            case EMythicTradeResult::VendorCannotPay:
            case EMythicTradeResult::NothingToRepair:
            case EMythicTradeResult::RequiresStation:
                return true;
            default:
                return false;
        }
    }

    FText DescribeResult(EMythicTradeResult Result) {
        switch (Result) {
            case EMythicTradeResult::InsufficientFunds:
                return LOCTEXT("InsufficientFunds", "Not enough gold");
            case EMythicTradeResult::OutOfStock:
                return LOCTEXT("OutOfStock", "Out of stock");
            case EMythicTradeResult::NoRoom:
                return LOCTEXT("NoRoom", "Inventory full");
            case EMythicTradeResult::NotForSale:
                return LOCTEXT("NotForSale", "Not for sale");
            case EMythicTradeResult::NotSellable:
                return LOCTEXT("NotSellable", "Can't sell that");
            case EMythicTradeResult::VendorCannotPay:
                return LOCTEXT("VendorCannotPay", "Merchant can't pay");
            case EMythicTradeResult::NothingToRepair:
                return LOCTEXT("NothingToRepair", "Nothing to repair");
            case EMythicTradeResult::RequiresStation:
                return LOCTEXT("RequiresStation", "Requires a nearby Forge");
            default:
                return FText::GetEmpty();
        }
    }
}

#undef LOCTEXT_NAMESPACE

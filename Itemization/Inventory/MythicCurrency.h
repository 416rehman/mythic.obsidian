#pragma once

#include "CoreMinimal.h"

/**
 * Pure currency-transaction decisions (no engine state) for the wallet/economy verb, so the money rules are
 * unit-testable headlessly. Currency is modelled as stackable Itemization.Type.Currency items in the inventory (the
 * existing model — loot already drops them), so a player's "balance" is the summed currency-item stacks
 * (UMythicInventoryComponent::GetTotalCurrency). These helpers operate on plain integer balances/prices; the actual
 * add/remove of currency items + vendor flow is the wiring built on top.
 */
namespace MythicCurrency {
    // Can a balance cover a price? A non-positive price is always affordable (free); a negative balance never is.
    MYTHIC_API bool CanAfford(int32 Balance, int32 Price);

    // The balance after spending Price: Balance - Price when affordable (and Price > 0), else the balance UNCHANGED
    // (an unaffordable or non-positive spend is a no-op — never goes negative, never silently "gives" money).
    MYTHIC_API int32 ComputeBalanceAfterSpend(int32 Balance, int32 Price);

    // The currency a vendor pays for Quantity units of an item worth UnitValue each, at a buy-back SellRate (e.g. 0.5 =
    // half value). Floored to whole coins, clamped non-negative. SellRate clamped to [0,1] so a vendor never pays above
    // value. Zero-value (unsellable) or non-positive quantity → 0.
    MYTHIC_API int32 ComputeSalePrice(int32 UnitValue, int32 Quantity, float SellRate);
}

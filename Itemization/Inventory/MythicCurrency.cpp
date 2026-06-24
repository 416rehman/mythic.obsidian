#include "MythicCurrency.h"

namespace MythicCurrency {
    bool CanAfford(int32 Balance, int32 Price) {
        if (Price <= 0) {
            return true; // free / refund — always affordable
        }
        return Balance >= Price;
    }

    int32 ComputeBalanceAfterSpend(int32 Balance, int32 Price) {
        if (Price <= 0 || !CanAfford(Balance, Price)) {
            return Balance; // no-op: nothing to pay, or can't afford it (caller must check CanAfford first)
        }
        return Balance - Price;
    }

    int32 ComputeSalePrice(int32 UnitValue, int32 Quantity, float SellRate) {
        if (UnitValue <= 0 || Quantity <= 0) {
            return 0;
        }
        const float Rate = FMath::Clamp(SellRate, 0.0f, 1.0f);
        return FMath::FloorToInt(static_cast<float>(UnitValue) * static_cast<float>(Quantity) * Rate);
    }
}

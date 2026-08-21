#include "MythicCurrency.h"

namespace MythicCurrency {
    bool CanAfford(int32 Balance, int32 Price) {
        if (Price <= 0) {
            return true;
        }
        return Balance >= Price;
    }

    int32 ComputeBalanceAfterSpend(int32 Balance, int32 Price) {
        if (Price <= 0 || !CanAfford(Balance, Price)) {
            return Balance;
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

    int32 ComputeBuyPrice(int32 UnitValue, int32 Quantity, float PriceMultiplier) {
        if (UnitValue <= 0 || Quantity <= 0) {
            return 0;
        }
        const float Mult = FMath::Max(0.0f, PriceMultiplier);
        const int32 UnitPrice = FMath::Max(0, FMath::CeilToInt(static_cast<float>(UnitValue) * Mult));
        return UnitPrice * Quantity;
    }

    int32 ComputeRepairCost(int32 CurrentDurability, int32 MaxDurability, int32 ItemValue, float RepairCostFraction) {
        if (MaxDurability <= 0 || ItemValue <= 0 || RepairCostFraction <= 0.0f) {
            return 0;
        }
        const int32 Missing = FMath::Clamp(MaxDurability - CurrentDurability, 0, MaxDurability);
        if (Missing <= 0) {
            return 0;
        }
        const float Frac = static_cast<float>(Missing) / static_cast<float>(MaxDurability);
        return FMath::Max(0, FMath::CeilToInt(static_cast<float>(ItemValue) * Frac * FMath::Max(0.0f, RepairCostFraction)));
    }

    int32 ComputeRerollCost(int32 ItemLevel, int32 RarityIndex, int32 BaseCost, float LevelFraction, float RarityFraction) {
        if (BaseCost <= 0) {
            return 0;
        }
        const int32 Lvl = FMath::Max(0, ItemLevel);
        const int32 Rarity = FMath::Max(0, RarityIndex);
        const float LevelScale = 1.0f + static_cast<float>(Lvl) * FMath::Max(0.0f, LevelFraction);
        const float RarityScale = 1.0f + static_cast<float>(Rarity) * FMath::Max(0.0f, RarityFraction);
        return FMath::Max(0, FMath::CeilToInt(static_cast<float>(BaseCost) * LevelScale * RarityScale));
    }
}

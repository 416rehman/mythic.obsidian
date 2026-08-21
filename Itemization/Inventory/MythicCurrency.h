#pragma once

#include "CoreMinimal.h"

namespace MythicCurrency {
    MYTHIC_API bool CanAfford(int32 Balance, int32 Price);

    MYTHIC_API int32 ComputeBalanceAfterSpend(int32 Balance, int32 Price);

    MYTHIC_API int32 ComputeSalePrice(int32 UnitValue, int32 Quantity, float SellRate);

    MYTHIC_API int32 ComputeBuyPrice(int32 UnitValue, int32 Quantity, float PriceMultiplier);

    MYTHIC_API int32 ComputeRepairCost(int32 CurrentDurability, int32 MaxDurability, int32 ItemValue, float RepairCostFraction);

    MYTHIC_API int32 ComputeRerollCost(int32 ItemLevel, int32 RarityIndex, int32 BaseCost, float LevelFraction, float RarityFraction);
}


#pragma once

#include "CoreMinimal.h"

namespace MythicLootFilter {
    inline constexpr int32 DefaultMaxJunkRarity = 0;

    MYTHIC_API bool QualifiesAsAutoJunk(int32 RarityValue, int32 MaxJunkRarityValue, int32 UnitValue,
                                        bool bIsCurrency, bool bIsEquipped, bool bCanTake);

    MYTHIC_API bool IsJunk(bool bManuallyMarked, int32 RarityValue, int32 MaxJunkRarityValue, int32 UnitValue,
                           bool bIsCurrency, bool bIsEquipped, bool bCanTake);
}

struct FMythicSellAllSlot {
    int32 SlotIndex = INDEX_NONE;
    bool bIsJunk = false;
    bool bSellable = false;
    int32 UnitValue = 0;
    int32 Quantity = 0;
};

struct FMythicSellAllPlan {
    TArray<int32> SlotsToSell;
    int32 TotalValue = 0;
};

namespace MythicLootFilter {
    MYTHIC_API FMythicSellAllPlan ComputeSellAllJunkPlan(const TArray<FMythicSellAllSlot> &Slots);
}

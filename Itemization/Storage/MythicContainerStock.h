
#pragma once

#include "CoreMinimal.h"
#include "Math/RandomStream.h"

namespace MythicContainerStock {
    struct FStockEntry {
        float OverrideDropChance = 0.0f;

        int32 StackMin = 1;
        int32 StackMax = 1;

        bool bStackable = false;
    };

    struct FStockRoll {
        int32 EntryIndex = INDEX_NONE;
        int32 Quantity = 1;
    };

    MYTHIC_API void RollStock(TConstArrayView<FStockEntry> Entries, float TableDropChance, int32 MaxItems,
                              float DefaultChance, FRandomStream &Stream, TArray<FStockRoll> &OutRolls);
}

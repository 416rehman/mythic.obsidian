
#include "MythicLootFilter.h"

namespace MythicLootFilter {
    bool QualifiesAsAutoJunk(int32 RarityValue, int32 MaxJunkRarityValue, int32 UnitValue,
                             bool bIsCurrency, bool bIsEquipped, bool bCanTake) {
        if (bIsCurrency || bIsEquipped) {
            return false;
        }
        if (!bCanTake || UnitValue <= 0) {
            return false;
        }
        return RarityValue <= MaxJunkRarityValue;
    }

    bool IsJunk(bool bManuallyMarked, int32 RarityValue, int32 MaxJunkRarityValue, int32 UnitValue,
                bool bIsCurrency, bool bIsEquipped, bool bCanTake) {
        return bManuallyMarked
                   || QualifiesAsAutoJunk(RarityValue, MaxJunkRarityValue, UnitValue, bIsCurrency, bIsEquipped, bCanTake);
    }

    FMythicSellAllPlan ComputeSellAllJunkPlan(const TArray<FMythicSellAllSlot> &Slots) {
        FMythicSellAllPlan Plan;
        int64 Running = 0;
        for (const FMythicSellAllSlot &Slot : Slots) {
            if (!Slot.bIsJunk || !Slot.bSellable) {
                continue;
            }
            Plan.SlotsToSell.Add(Slot.SlotIndex);
            const int64 UnitValue = FMath::Max(0, Slot.UnitValue);
            const int64 Quantity = FMath::Max(0, Slot.Quantity);
            Running += UnitValue * Quantity;
        }
        Plan.TotalValue = static_cast<int32>(FMath::Clamp<int64>(Running, 0, MAX_int32));
        return Plan;
    }
}

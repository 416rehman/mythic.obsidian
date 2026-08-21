
#include "MythicContainerStock.h"

namespace MythicContainerStock {
    void RollStock(TConstArrayView<FStockEntry> Entries, float TableDropChance, int32 MaxItems,
                   float DefaultChance, FRandomStream &Stream, TArray<FStockRoll> &OutRolls) {
        OutRolls.Reset();

        if (Entries.Num() == 0 || MaxItems <= 0) {
            return;
        }

        if (Stream.FRand() > TableDropChance) {
            return;
        }

        TArray<int32> Winners;
        Winners.Reserve(Entries.Num());
        for (int32 i = 0; i < Entries.Num(); ++i) {
            const FStockEntry &Entry = Entries[i];
            const float Chance = Entry.OverrideDropChance > 0.0f ? Entry.OverrideDropChance : DefaultChance;
            if (Stream.FRand() <= Chance) {
                Winners.Add(i);
            }
        }

        if (Winners.Num() == 0) {
            return;
        }

        const int32 Count = Stream.RandRange(1, FMath::Min(MaxItems, Winners.Num()));
        OutRolls.Reserve(Count);

        for (int32 Picked = 0; Picked < Count; ++Picked) {
            const int32 Swap = Stream.RandRange(Picked, Winners.Num() - 1);
            Winners.Swap(Picked, Swap);

            const FStockEntry &Entry = Entries[Winners[Picked]];

            FStockRoll Roll;
            Roll.EntryIndex = Winners[Picked];
            Roll.Quantity = 1;
            if (Entry.bStackable) {
                const int32 Min = FMath::Max(1, FMath::Min(Entry.StackMin, Entry.StackMax));
                const int32 Max = FMath::Max(Min, FMath::Max(Entry.StackMin, Entry.StackMax));
                Roll.Quantity = Stream.RandRange(Min, Max);
            }
            OutRolls.Add(Roll);
        }
    }
}

#pragma once

#include "CoreMinimal.h"
#include "Mass/EntityHandle.h"
#include "World/LivingWorld/LivingWorldTypes.h"

struct FMythicCellSpatialIndex {
    void Reset() {
        for (TPair<FMythicCellCoord, TArray<FMassEntityHandle>> &Pair : CellBuckets) {
            Pair.Value.Reset();
        }
        TotalEntities = 0;
    }

    void Insert(const FMythicCellCoord &Cell, FMassEntityHandle Entity) {
        CellBuckets.FindOrAdd(Cell).Add(Entity);
        ++TotalEntities;
    }

    void QueryRange(const FMythicCellCoord &Center, int32 RadiusCells, TArray<FMassEntityHandle> &OutEntities) const {
        if (RadiusCells < 0) {
            return;
        }
        for (int32 DY = -RadiusCells; DY <= RadiusCells; ++DY) {
            for (int32 DX = -RadiusCells; DX <= RadiusCells; ++DX) {
                if (const TArray<FMassEntityHandle> *Bucket = CellBuckets.Find(FMythicCellCoord(Center.X + DX, Center.Y + DY))) {
                    OutEntities.Append(*Bucket);
                }
            }
        }
    }

    int32 GetCellCount(const FMythicCellCoord &Cell) const {
        const TArray<FMassEntityHandle> *Bucket = CellBuckets.Find(Cell);
        return Bucket ? Bucket->Num() : 0;
    }

    int32 Num() const { return TotalEntities; }

private:
    TMap<FMythicCellCoord, TArray<FMassEntityHandle>> CellBuckets;
    int32 TotalEntities = 0;
};

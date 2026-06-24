#pragma once

#include "CoreMinimal.h"
#include "Mass/EntityHandle.h"
#include "World/LivingWorld/LivingWorldTypes.h" // FMythicCellCoord (hashable: used as TMap/TSet key elsewhere)

/**
 * Broad-phase cell -> entity spatial index for the living-world MASS entity set.
 *
 * PURPOSE: several living-world systems currently sweep EVERY entity to answer a spatial question — the witness-
 * perception scan walks all entities per crime event to find those near the event cell (O(events x N)), and the
 * population spawner walks all entities to count per-cell density (O(N)). This index replaces those full sweeps with
 * a single O(N) rebuild per tick (Reset + Insert per entity) plus O((2r+1)^2) range queries / O(1) per-cell counts.
 *
 * ARCHITECTURE: pure data (no UObject, no world, no replication, no MASS processing) — owned by a manager/subsystem
 * that rebuilds it each tick from a MASS query and handed to the consumers. Server-side only (the living-world sim is
 * server-authoritative; nothing here replicates). Stores FMassEntityHandle payloads; the caller resolves fragments
 * from the handle exactly as it does today.
 *
 * QueryRange is BROAD-PHASE: it returns every entity whose cell is within a Chebyshev (square) radius of the center.
 * The caller then applies the precise distance test it already performs (Manhattan hearing range, circular spawn
 * radius, etc.) on the much smaller candidate set. This mirrors the standard broad/narrow-phase split and keeps the
 * exact gameplay semantics the callers already have.
 *
 * NOTE: this is the data-structure prerequisite. Wiring the witness processor + population spawner to consume it is
 * the next iteration (see BACKLOG). Verified here by FLivingWorldSpatialIndexQueryTest (Mythic.LivingWorld.SpatialIndex).
 */
struct FMythicCellSpatialIndex {
    // Clear all buckets for a fresh rebuild, RETAINING the map keys + per-bucket array allocations (TArray::Reset keeps
    // slack) so a per-tick rebuild does not churn allocations. Stale empty buckets are harmless (GetCellCount/QueryRange
    // treat them as empty); the key set is bounded by the number of distinct cells ever occupied.
    void Reset() {
        for (TPair<FMythicCellCoord, TArray<FMassEntityHandle>> &Pair : CellBuckets) {
            Pair.Value.Reset();
        }
        TotalEntities = 0;
    }

    // Index one entity at its cell. O(1) amortized.
    void Insert(const FMythicCellCoord &Cell, FMassEntityHandle Entity) {
        CellBuckets.FindOrAdd(Cell).Add(Entity);
        ++TotalEntities;
    }

    // Append every indexed entity within Chebyshev radius RadiusCells of Center to OutEntities (broad phase — the
    // caller refines with its own precise distance test). RadiusCells < 0 is a no-op. Does NOT clear OutEntities first.
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

    // Number of entities indexed in exactly this cell (for the per-cell population census). 0 if the cell is empty.
    int32 GetCellCount(const FMythicCellCoord &Cell) const {
        const TArray<FMassEntityHandle> *Bucket = CellBuckets.Find(Cell);
        return Bucket ? Bucket->Num() : 0;
    }

    // Total entities indexed across all cells since the last Reset.
    int32 Num() const { return TotalEntities; }

private:
    TMap<FMythicCellCoord, TArray<FMassEntityHandle>> CellBuckets;
    int32 TotalEntities = 0;
};

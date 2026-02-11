// Mythic Living World System — Territory Grid
// 2D grid tracking faction influence per cell. Background thread writes, game thread reads cache.

#pragma once

#include "CoreMinimal.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "TerritoryGrid.generated.h"

// ─────────────────────────────────────────────────────────────
// Cell Data — Per-cell territory state
// ─────────────────────────────────────────────────────────────

/** Per-cell data in the territory grid. ~6 bytes per cell. */
USTRUCT()
struct MYTHIC_API FMythicTerritoryCell {
    GENERATED_BODY()

    /** Dominant faction controlling this cell */
    FMythicFactionId DominantFaction;

    /** Influence strength of the dominant faction (0.0 - 1.0) */
    float Influence = 0.0f;

    /** Is this cell a player-owned property? */
    uint8 bPlayerOwned : 1;

    /** Player index who owns this cell (only valid if bPlayerOwned) */
    uint8 OwningPlayerIndex = 0;

    FMythicTerritoryCell() : bPlayerOwned(false) {}
};

// ─────────────────────────────────────────────────────────────
// Territory Grid Settings — Data asset for configuration
// ─────────────────────────────────────────────────────────────

UCLASS(BlueprintType, Const)
class MYTHIC_API UMythicTerritoryGridSettings : public UDataAsset {
    GENERATED_BODY()

public:
    /** Grid width (number of cells along X axis) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "16", ClampMax = "1024"))
    int32 GridWidth = 128;

    /** Grid height (number of cells along Y axis) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "16", ClampMax = "1024"))
    int32 GridHeight = 128;

    /** World-space size of each cell in centimeters */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "100.0"))
    float CellWorldSize = 5000.0f;

    /** World-space origin offset (bottom-left corner of grid in world coordinates) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FVector2D WorldOrigin = FVector2D::ZeroVector;

    /** Influence bleed rate per sim tick (how fast influence spreads to neighbor cells) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float InfluenceBleedRate = 0.05f;

    /** Minimum influence threshold for a faction to maintain control */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MinControlThreshold = 0.1f;
};

// ─────────────────────────────────────────────────────────────
// Territory Grid — Spatial faction control
// ─────────────────────────────────────────────────────────────

/**
 * 2D grid of territory cells tracking which faction controls each area.
 *
 * Threading model:
 * - Background thread writes (influence propagation, faction claims)
 * - Game thread reads from a committed snapshot (lock-free)
 * - Double-buffered like the Causal Fabric
 *
 * Memory: ~6 bytes per cell × 128×128 = ~98KB per buffer × 2 = ~196KB total
 */
UCLASS()
class MYTHIC_API UMythicTerritoryGrid : public UObject {
    GENERATED_BODY()

public:
    /** Initialize the grid from settings. Must be called before use. */
    void Initialize(const UMythicTerritoryGridSettings *Settings);

    // ─── Write Interface (Background Thread Only) ─────────

    /** Set faction influence on a specific cell */
    void SetCellInfluence(const FMythicCellCoord &Coord, FMythicFactionId Faction, float Influence);

    /** Set player ownership of a cell */
    void SetCellPlayerOwned(const FMythicCellCoord &Coord, bool bOwned, uint8 PlayerIndex = 0);

    /**
     * Run one tick of influence propagation.
     * Each cell bleeds influence to its 4-neighbors at the configured rate.
     * Called by the world sim background thread.
     */
    void PropagateInfluence();

    /** Commit the write buffer — swaps the read snapshot for game thread. */
    void CommitWrites();

    // ─── Read Interface (Game Thread — Lock-Free) ─────────

    /** Get the territory cell data at a grid coordinate. Returns default cell if out of bounds. */
    FMythicTerritoryCell GetCell(const FMythicCellCoord &Coord) const;

    /** Get the dominant faction at a grid coordinate */
    FMythicFactionId GetDominantFaction(const FMythicCellCoord &Coord) const;

    /** Convert a world-space position to a cell coordinate */
    FMythicCellCoord WorldToCell(const FVector &WorldPosition) const;

    /** Convert a cell coordinate to world-space center position */
    FVector CellToWorld(const FMythicCellCoord &Coord) const;

    /** Check if a cell coordinate is valid (within grid bounds) */
    bool IsValidCoord(const FMythicCellCoord &Coord) const;

    /** Get grid dimensions */
    int32 GetWidth() const { return Width; }
    int32 GetHeight() const { return Height; }

    /**
     * Get all cells controlled by a specific faction.
     * Budget-capped — returns up to MaxResults.
     */
    void GetFactionCells(FMythicFactionId Faction, int32 MaxResults, TArray<FMythicCellCoord> &OutCells) const;

    /**
     * Get cells that changed dominant faction since last commit.
     * Used for delta-compressed network replication.
     */
    void GetChangedCells(TArray<FMythicCellCoord> &OutChangedCells) const;

private:
    int32 Width = 0;
    int32 Height = 0;
    float CellWorldSize = 5000.0f;
    FVector2D WorldOrigin = FVector2D::ZeroVector;
    float InfluenceBleedRate = 0.05f;
    float MinControlThreshold = 0.1f;

    /** Write buffer — background thread only */
    TArray<FMythicTerritoryCell> WriteBuffer;

    /** Read buffer — game thread snapshot */
    TArray<FMythicTerritoryCell> ReadBuffer;

    /** Track which cells changed since last commit (for delta replication) */
    TBitArray<> DirtyCells;

    /** Flatten 2D coord to 1D index */
    int32 CoordToIndex(const FMythicCellCoord &Coord) const {
        return static_cast<int32>(Coord.Y) * Width + static_cast<int32>(Coord.X);
    }
};

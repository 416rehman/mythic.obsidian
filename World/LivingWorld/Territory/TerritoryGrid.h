// Mythic Living World System — Territory Grid
// 2D grid tracking faction influence per cell. Background thread writes, game thread reads cache.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/LivingWorld/Territory/MythicBiome.h"
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

    // ─── Biome (procedural, immutable) ────────────────────
    // Biomes are a PURE deterministic function of (CellX, CellY, BiomeWorldSeed) — never simulated, never mutated.
    // They classify wilderness for wildlife selection. Change the seed to reshuffle the whole world's terrain.

    /** Seed for the procedural biome value-noise. Same seed => same biome map across runs/saves. */
    UPROPERTY(EditDefaultsOnly)
    uint32 BiomeWorldSeed = 1337;

    /** Spatial frequency of the biome noise lattice (cells^-1). Lower = larger, smoother biome regions. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.001", ClampMax = "1.0"))
    float BiomeNoiseFrequency = 0.08f;

    /** Designer-tunable elevation/moisture bands that map the two noise channels to a biome. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FMythicBiomeThresholds BiomeThresholds;
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

    /**
     * Tally cells owned per faction from the (sim-thread-private) write buffer, indexed by FMythicFactionId.Index.
     * Lets the sim reconcile each faction's authoritative ControlledCellCount with the emergent influence-driven
     * ownership flips after PropagateInfluence(). Caller must already hold the external SimulationLock (no SnapshotLock
     * is taken — this reads only WriteBuffer, exactly like CommitWrites' rebuild does). OutCounts is sized to 256.
     */
    void GetWriteCellCounts(TArray<int32> &OutCountsByFactionIndex) const;

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

    // ─── Biome Read (Game Thread — Lock-Free, pure) ───────

    /**
     * Biome of a cell. PURE function of (X, Y, cached BiomeWorldSeed/Frequency/Thresholds) — takes NO lock, allocates
     * nothing, never reads sim-mutated state, so it is safe to call from any game-thread hot loop. Returns Plains for
     * out-of-bounds coords (well-defined fallback).
     */
    UFUNCTION(BlueprintCallable, Category = "Living World|Biome")
    EMythicBiome GetBiomeAtCell(const FMythicCellCoord &Coord) const;

    /** Convenience: biome at a world position (= GetBiomeAtCell(WorldToCell(WorldPos))). */
    EMythicBiome GetBiomeAtWorld(const FVector &WorldPos) const;

    /** Get grid dimensions */
    int32 GetWidth() const { return Width; }
    int32 GetHeight() const { return Height; }
    float GetCellSize() const { return CellWorldSize; }

    /**
     * Get all cells controlled by a specific faction.
     * Budget-capped — returns up to MaxResults.
     */
    void GetFactionCells(FMythicFactionId Faction, int32 MaxResults, TArray<FMythicCellCoord> &OutCells) const;

    /**
     * Get cells flagged dirty (ANY influence change) since the last commit — NOT only dominant-faction flips. Influence
     * propagation marks a cell dirty whenever its influence shifts, even when the dominant faction is unchanged, so this
     * is a SUPERSET of dominant-faction changes. Callers replicating per-DOMINANT-FACTION proxy state must therefore
     * change-detect downstream (see AMythicLivingWorldReplicator::TerritoryProxyNeedsUpdate) or they re-send unchanged
     * proxies. Drains the committed delta (consumed once per call). Used for delta-compressed network replication.
     */
    void GetChangedCells(TArray<FMythicCellCoord> &OutChangedCells) const;

    // ─── Serialization ───────────────────────────────────

    /** Serialize the entire grid state for save/load. */
    virtual void Serialize(FArchive &Ar) override;

private:
    int32 Width = 0;
    int32 Height = 0;
    float CellWorldSize = 5000.0f;
    FVector2D WorldOrigin = FVector2D::ZeroVector;
    float InfluenceBleedRate = 0.05f;
    float MinControlThreshold = 0.1f;

    // ─── Biome cache (immutable after Initialize) ─────────
    uint32 BiomeWorldSeed = 0;
    float BiomeNoiseFrequency = 0.08f;
    FMythicBiomeThresholds BiomeThresholds;

    /**
     * Classify from two already-sampled noise channels (Elevation, Moisture each in [0,1]) via the threshold bands.
     * Pure + static + frequency-agnostic — the single source of truth for the Elev/Moist -> biome mapping, so it's
     * trivially unit-testable independent of the noise sampler.
     */
    static EMythicBiome ComputeBiome(float Elevation, float Moisture, const FMythicBiomeThresholds &T);

    /** 2D integer hash for the noise lattice. Wang-mix identical to FMythicNPCGenerator::HashStep (inlined, no include). */
    static uint32 BiomeHash2D(int32 X, int32 Y, uint32 Seed);

    /** Smoothstep-interpolated lattice value noise in [0,1] at (X*Frequency, Y*Frequency). Pure + static. */
    static float BiomeValueNoise(int32 X, int32 Y, uint32 Seed, float Frequency);

    /** Write buffer — background thread only */
    TArray<FMythicTerritoryCell> WriteBuffer;

    /** Read buffer — GameThread access (snapshot) */
    TArray<FMythicTerritoryCell> ReadBuffer;

    /** Write buffer cache of cells owned by each faction (Index = FactionId) */
    TArray<TArray<FMythicCellCoord>> WriteFactionCells;

    /** Read buffer cache of cells owned by each faction */
    TArray<TArray<FMythicCellCoord>> ReadFactionCells;

    /** Lock protecting ReadBuffer during CommitWrites */
    mutable FCriticalSection SnapshotLock;

    /** Dirty flags — working set of cells changed during the current tick (set by SetCellInfluence/PropagateInfluence).
     *  Folded into ReadDirtyCells + cleared at CommitWrites. Guarded by the EXTERNAL SimulationLock: EVERY writer holds
     *  it — the sim thread (SimTick -> CommitAllSnapshots -> CommitWrites) AND the game-thread settlement seed/transfer
     *  paths (RegisterSettlement / TransferSettlement / SeedTerritoryFromSettlements). NOT sim-thread-only — a future
     *  refactor must keep SimulationLock on any DirtyCells writer. (ReadDirtyCells is the SnapshotLock-guarded snapshot.) */
    TBitArray<> DirtyCells;

    /** Committed changed-cell deltas pending replication, guarded by SnapshotLock. CommitWrites (sim thread) ORs the
     *  tick's DirtyCells in; GetChangedCells (game thread) drains it. Mutable so the const drain can clear it. */
    mutable TBitArray<> ReadDirtyCells;

    /** Flatten 2D coord to 1D index */
    int32 CoordToIndex(const FMythicCellCoord &Coord) const {
        return static_cast<int32>(Coord.Y) * Width + static_cast<int32>(Coord.X);
    }
};

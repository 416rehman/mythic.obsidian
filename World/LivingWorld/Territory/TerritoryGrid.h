
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/LivingWorld/Territory/MythicBiome.h"
#include "World/LivingWorld/Territory/MythicDanger.h"
#include "TerritoryGrid.generated.h"


USTRUCT()
struct MYTHIC_API FMythicTerritoryCell {
    GENERATED_BODY()

    FMythicFactionId DominantFaction;

    float Influence = 0.0f;

    uint8 bPlayerOwned : 1;

    uint8 OwningPlayerIndex = 0;

    FMythicTerritoryCell() : bPlayerOwned(false) {}
};


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


UCLASS()
class MYTHIC_API UMythicTerritoryGrid : public UObject {
    GENERATED_BODY()

public:
    void Initialize(const UMythicTerritoryGridSettings *Settings);


    void SetCellInfluence(const FMythicCellCoord &Coord, FMythicFactionId Faction, float Influence);

    void SetCellPlayerOwned(const FMythicCellCoord &Coord, bool bOwned, uint8 PlayerIndex = 0);

    void PropagateInfluence();

    void CommitWrites();

    void GetWriteCellCounts(TArray<int32> &OutCountsByFactionIndex) const;


    FMythicTerritoryCell GetCell(const FMythicCellCoord &Coord) const;

    FMythicFactionId GetDominantFaction(const FMythicCellCoord &Coord) const;

    FMythicCellCoord WorldToCell(const FVector &WorldPosition) const;

    FVector CellToWorld(const FMythicCellCoord &Coord) const;

    bool IsValidCoord(const FMythicCellCoord &Coord) const;


    /**
     * Biome of a cell. PURE function of (X, Y, cached BiomeWorldSeed/Frequency/Thresholds) — takes NO lock, allocates
     * nothing, never reads sim-mutated state, so it is safe to call from any game-thread hot loop. Returns Plains for
     * out-of-bounds coords (well-defined fallback).
     */
    UFUNCTION(BlueprintCallable, Category = "Living World|Biome")
    EMythicBiome GetBiomeAtCell(const FMythicCellCoord &Coord) const;

    EMythicBiome GetBiomeAtWorld(const FVector &WorldPos) const;


    /**
     * Danger tier of a cell (Safe..Extreme), derived from LIVE grid state via FMythicDanger::ComputeDangerTier:
     *   - Anchor: the grid CENTER is treated as the safe "civilization core"; danger rises with Chebyshev distance
     *     toward the frontier edges. (A future slice can feed the real player/faction capital cell instead — the pure
     *     ComputeDangerTier already takes an explicit distance, so only this glue changes.)
     *   - Strength: the cell's controlling INFLUENCE [0,1] is the contested-strength signal (a strongly-held frontier
     *     cell is more dangerous than a lightly-held one).
     *   - Safe zones: the core cell itself and any player-owned cell read Safe.
     * Takes the snapshot lock once (via GetCell). Out-of-bounds coords read Safe (no threat off-grid).
     * Exposed to Blueprint so a war-map danger overlay / debug HUD can tint cells by tier.
     */
    UFUNCTION(BlueprintCallable, Category = "Living World|Danger")
    EMythicDangerTier GetCellDangerTier(const FMythicCellCoord &Cell) const;

    int32 GetWidth() const { return Width; }
    int32 GetHeight() const { return Height; }
    float GetCellSize() const { return CellWorldSize; }

    void GetFactionCells(FMythicFactionId Faction, int32 MaxResults, TArray<FMythicCellCoord> &OutCells) const;

    void GetChangedCells(TArray<FMythicCellCoord> &OutChangedCells) const;


    virtual void Serialize(FArchive &Ar) override;

private:
    int32 Width = 0;
    int32 Height = 0;
    float CellWorldSize = 5000.0f;
    FVector2D WorldOrigin = FVector2D::ZeroVector;
    float InfluenceBleedRate = 0.05f;
    float MinControlThreshold = 0.1f;

    uint32 BiomeWorldSeed = 0;
    float BiomeNoiseFrequency = 0.08f;
    FMythicBiomeThresholds BiomeThresholds;

    static EMythicBiome ComputeBiome(float Elevation, float Moisture, const FMythicBiomeThresholds &T);

    static uint32 BiomeHash2D(int32 X, int32 Y, uint32 Seed);

    static float BiomeValueNoise(int32 X, int32 Y, uint32 Seed, float Frequency);

    TArray<FMythicTerritoryCell> WriteBuffer;

    TArray<FMythicTerritoryCell> ReadBuffer;

    TArray<TArray<FMythicCellCoord>> WriteFactionCells;

    TArray<TArray<FMythicCellCoord>> ReadFactionCells;

    mutable FCriticalSection SnapshotLock;

    TBitArray<> DirtyCells;

    mutable TBitArray<> ReadDirtyCells;

    int32 CoordToIndex(const FMythicCellCoord &Coord) const {
        return static_cast<int32>(Coord.Y) * Width + static_cast<int32>(Coord.X);
    }
};

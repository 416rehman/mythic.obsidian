// Mythic Living World — Settlement System
// Spline-based settlement actors that seed territory and drive population spawning.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "MythicSettlement.generated.h"

class USplineComponent;
class UMythicTerritoryGrid;
class UInstancedStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;

// ─────────────────────────────────────────────────────────────
// Settlement Data — Runtime state for a single settlement
// ─────────────────────────────────────────────────────────────

/**
 * Tracks a persistent point-of-interest role within a settlement (e.g., Blacksmith, Innkeeper).
 * When the NPC holding this role dies, the slot is vacated and a succession timer begins.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicShopSlot {
    GENERATED_BODY()

    /** Name of the shop ("The Rusty Anvil") */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FString ShopName;

    /** Role required to run this shop (e.g., "NPC.Role.Merchant.Blacksmith") */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FGameplayTag RequiredRole;

    /** Entity ID of the current owner (0 = vacated) */
    UPROPERTY(BlueprintReadOnly)
    int32 OwnerEntityId = 0;

    /** If vacant, when did the previous owner die? (World Time) */
    UPROPERTY(BlueprintReadOnly)
    double VacatedTime = 0.0;

    /** Is this shop currently owned by a player? (Overrides NPC succession) */
    UPROPERTY(BlueprintReadOnly)
    bool bPlayerOwned = false;

    /** Player index if bPlayerOwned is true */
    UPROPERTY(BlueprintReadOnly)
    uint8 OwningPlayerIndex = 0;
};

// ─────────────────────────────────────────────────────────────
// Settlement Economy — the settlement's economic TYPE
// ─────────────────────────────────────────────────────────────

/**
 * The economic character of a settlement. Drives which civilian ROLES its ambient population derives (farmers in a
 * Farming town, merchants in a Trade hub, miners/laborers in a Mining outpost, etc.). Generic = derive from the
 * governing faction's base production. Authored per-settlement in the editor; falls back to faction production when
 * left Generic. The single authoritative definition (no other slice redefines this enum).
 */
UENUM(BlueprintType)
enum class EMythicSettlementEconomy : uint8 {
    Generic = 0 UMETA(DisplayName = "Generic"),
    Farming UMETA(DisplayName = "Farming"),
    Trade UMETA(DisplayName = "Trade"),
    Military UMETA(DisplayName = "Military"),
    Mining UMETA(DisplayName = "Mining"),
    Fishing UMETA(DisplayName = "Fishing"),
    COUNT UMETA(Hidden)
};

// ─────────────────────────────────────────────────────────────
// Spawn-Point Purpose — what KIND of NPC a generated spawn point hosts
// ─────────────────────────────────────────────────────────────

/**
 * Classifies a settlement spawn point so the population spawner can place the right KIND of NPC at it (peaceful
 * townsfolk at Civilian points, town watch at Guard points, hostile occupants at Enemy points in a bandit camp). Single
 * authoritative definition (no other slice redefines this enum). Any matches a desired purpose as a fallback.
 */
UENUM(BlueprintType)
enum class EMythicSpawnPointPurpose : uint8 {
    Civilian = 0 UMETA(DisplayName = "Civilian"),
    Guard UMETA(DisplayName = "Guard"),
    Enemy UMETA(DisplayName = "Enemy"),
    Any UMETA(DisplayName = "Any"),
    COUNT UMETA(Hidden)
};

// ─────────────────────────────────────────────────────────────
// Spawn Point — a precomputed, navmesh-valid tagged anchor
// ─────────────────────────────────────────────────────────────

/**
 * A single navmesh-validated spawn anchor inside a settlement, computed once at BeginPlay via MythicPlacement.
 * The population spawner anchors an NPC's body here (the embodiment fast-path: ActorSpawnProcessor skips the full
 * project+scatter pipeline and only re-tests occupancy). NOT serialized — these are runtime placement anchors derived
 * from the live navmesh; entity identity is the NameHash, never the position, so a save/load that regenerates them at
 * different exact positions does not break any NPC's identity.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicSpawnPoint {
    GENERATED_BODY()

    /** Precomputed-valid foot position (from MythicPlacement::FindValidSpawn at generation time). */
    UPROPERTY(BlueprintReadOnly, Category = "Settlement")
    FVector WorldLocation = FVector::ZeroVector;

    /** Owning cell — equals the embodied entity's Identity.Cell, so per-cell density bookkeeping is unchanged. */
    UPROPERTY(BlueprintReadOnly, Category = "Settlement")
    FMythicCellCoord Cell;

    /** What kind of NPC this point hosts (drives the spawner's point selection per derived role). */
    UPROPERTY(BlueprintReadOnly, Category = "Settlement")
    EMythicSpawnPointPurpose Purpose = EMythicSpawnPointPurpose::Civilian;
};

/**
 * Runtime data for a settlement. Populated from the settlement actor's
 * editor-configured properties + spline rasterization results.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicSettlementData {
    GENERATED_BODY()

    /** Human-readable settlement name (e.g., "City of Avalon") */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settlement")
    FText DisplayName;

    /** Faction currently governing this settlement */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settlement")
    FMythicFactionId GoverningFaction;

    /** Grid cells that fall inside the settlement boundary (computed from spline at init) */
    UPROPERTY(BlueprintReadOnly, Category = "Settlement")
    TArray<FMythicCellCoord> RasterizedCells;

    /** Max MASS entities per cell within this settlement (designer-set ceiling; actual driven by faction sim) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Population", meta = (ClampMin = "1", ClampMax = "100"))
    int32 MaxPopulationDensity = 20;

    /** Gameplay tag for quest/script references (e.g., "Settlement.Avalon") */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settlement")
    FGameplayTag SettlementTag;

    /** Level instance or sub-level name */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settlement")
    FName LevelName;

    /** Center cell coordinate */
    UPROPERTY(BlueprintReadOnly, Category = "Settlement")
    FMythicCellCoord CenterCell;

    /** List of all persistent shops/roles in this settlement */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settlement")
    TArray<FMythicShopSlot> Shops;

    /** Economic type — drives the civilian role mix of this settlement's ambient population (farmers/merchants/etc.). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settlement")
    EMythicSettlementEconomy Economy = EMythicSettlementEconomy::Generic;

    /** If true, this settlement is the faction's capital — receives significance boost */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settlement")
    bool bIsCapital = false;

    /** Unique runtime ID assigned by the settlement registry */
    UPROPERTY(BlueprintReadOnly, Category = "Settlement")
    int32 SettlementId = INDEX_NONE;

    /** Number of MASS entities currently spawned in this settlement's cells */
    UPROPERTY(BlueprintReadOnly, Category = "Population")
    int32 CurrentEntityCount = 0;

    /** If true, this is a HOSTILE camp — its spawn points host enemies (bandits/raiders), not townsfolk. */
    UPROPERTY(BlueprintReadOnly, Category = "Population")
    bool bIsHostileCamp = false;

    /**
     * Precomputed navmesh-valid tagged spawn anchors, generated once at BeginPlay (NOT serialized — runtime placement
     * anchors derived from the live navmesh). Empty when navmesh wasn't ready / generation produced nothing → the
     * population spawner transparently falls back to the cell-center placement path (coexistence).
     */
    UPROPERTY(BlueprintReadOnly, Category = "Population", Transient)
    TArray<FMythicSpawnPoint> SpawnPoints;

    /** Get the number of territory cells this settlement covers */
    int32 GetCellCount() const { return RasterizedCells.Num(); }

    /** Get the number of generated spawn points (0 ⇒ cell-center placement fallback). */
    int32 GetSpawnPointCount() const { return SpawnPoints.Num(); }
};

// ─────────────────────────────────────────────────────────────
// Settlement Settings — Global defaults
// ─────────────────────────────────────────────────────────────

/**
 * Global settlement defaults. Referenced by the Living World Settings data asset.
 */
UCLASS(BlueprintType, Const)
class MYTHIC_API UMythicSettlementSettings : public UDataAsset {
    GENERATED_BODY()

public:
    /** Default max population density for settlements that don't override it */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "1", ClampMax = "100"))
    int32 DefaultMaxPopulationDensity = 20;

    /** Significance score boost multiplier for faction capitals */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "1.0", ClampMax = "5.0"))
    float CapitalSignificanceMultiplier = 2.0f;

    /** Minimum number of cells a settlement must rasterize to be valid (prevents degenerate splines) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "1", ClampMax = "100"))
    int32 MinSettlementCells = 1;
};

// ─────────────────────────────────────────────────────────────
// Settlement Actor — World-placed settlement with spline boundary
// ─────────────────────────────────────────────────────────────

/**
 * A settlement placed in the world by level designers.
 *
 * The actor uses a USplineComponent to define the settlement boundary.
 * Designers draw the spline around the settlement area in the editor.
 * On BeginPlay, the spline polygon is rasterized to territory grid cells,
 * and the settlement registers with the Living World Subsystem.
 *
 * Settlements seed territory, drive population spawning, and provide
 * zone-entry detection for "Welcome to..." notifications.
 */
UCLASS(Blueprintable, meta = (DisplayName = "Settlement"))
class MYTHIC_API AMythicSettlement : public AActor {
    GENERATED_BODY()

public:
    AMythicSettlement();

    // ─── Editor-Configured Properties ─────────────────────

    /** Settlement display name shown to players */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Settlement")
    FText SettlementName;

    /** Faction tag that governs this settlement at world start (resolved to FMythicFactionId at runtime) */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Settlement", meta = (Categories = "Faction"))
    FGameplayTag InitialFactionTag;

    /** Max MASS entities per cell. Actual count = MaxDensity × (FactionPop / FactionCapacity). */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Population", meta = (ClampMin = "1", ClampMax = "100"))
    int32 MaxPopulationDensity = 20;

    /** Gameplay tag for referencing this settlement from quests/scripts */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Settlement")
    FGameplayTag SettlementTag;

    /** Is this the faction's capital settlement? */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Settlement")
    bool bIsCapital = false;

    /** Economic type of this settlement — derives its civilian role mix (Generic = derive from faction production). */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Settlement")
    EMythicSettlementEconomy Economy = EMythicSettlementEconomy::Generic;

    /**
     * Server-authoritative hostility flag. When true, this settlement is a hostile camp: its generated spawn points are
     * tagged Enemy and the population spawner fills it with bandits/raiders instead of peaceful townsfolk. This is NOT
     * player-standing-based — the player is not a faction and standing is per-player & non-deterministic; hostility here
     * is a fixed property of the place (a bandit camp is a bandit camp for everyone).
     */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Population")
    bool bIsHostileCamp = false;

    /** Number of navmesh-validated spawn points to generate PER covered cell at BeginPlay (one-time placement anchors). */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Population", meta = (ClampMin = "1"))
    int32 SpawnPointsPerCell = 4;

    // ─── Editor Spawn-Cell Preview ────────────────────────

    /**
     * When true (default), the editor viewport shows one translucent cube per territory cell this settlement's spline
     * covers — i.e. the cells the PopulationSpawnerProcessor will fill with MASS NPCs. Editor-only: the instanced-mesh
     * component is hidden in the shipped game. Rebuilt in OnConstruction (so it tracks spline edits / actor moves).
     */
    UPROPERTY(EditAnywhere, Category = "Spawn Preview")
    bool bShowSpawnCellsInEditor = true;

    // ─── Runtime Access ───────────────────────────────────

    /** Get the runtime settlement data (valid after BeginPlay) */
    const FMythicSettlementData &GetSettlementData() const { return SettlementData; }

    /** Get the spline component defining the settlement boundary */
    USplineComponent *GetBoundarySpline() const { return BoundarySpline; }

    /**
     * Rasterize the spline boundary into territory grid cells.
     * Called during initialization. Populates SettlementData.RasterizedCells.
     * @param TerritoryGrid The grid to use for coordinate conversion
     */
    void RasterizeSplineToCells(const UMythicTerritoryGrid *TerritoryGrid);

    /**
     * Pure-math rasterizer shared by the runtime path (RasterizeSplineToCells) AND the edit-time spawn preview
     * (OnConstruction), so the editor footprint can NEVER drift from the cells PopulationSpawnerProcessor actually
     * spawns into. Replicates UMythicTerritoryGrid::WorldToCell / CellToWorld / IsValidCoord verbatim (float casts,
     * cell CENTER point-in-polygon, bounds-clamped AABB) using the four raw grid params instead of a grid object.
     * Samples the closed spline every 100cm into a polygon and tests each candidate cell center. Out is reset first;
     * a spline with <3 points yields no cells.
     */
    static void RasterizeSplineCells(const USplineComponent *Spline, float CellWorldSize, FVector2D WorldOrigin,
                                     int32 GridWidth, int32 GridHeight, TArray<FMythicCellCoord> &OutCells);

    /**
     * Compute a settlement's center cell: the rasterized cell nearest the cells' centroid (guaranteed to be an ACTUAL
     * settlement cell, so it's valid even for concave/L-shaped boundaries where the raw centroid could fall outside).
     * Empty input → (0,0). Pure + static so it's unit-testable. CenterCell drives the Socialize gather-point
     * (AIController — "converge where people are") — it was previously never assigned (always (0,0) → socializing NPCs
     * converged on the grid's origin corner instead of the town center). (It was also the v1 save key, since replaced
     * by SettlementTag.) RasterizeSplineToCells sets it from the rasterized cells.
     */
    static FMythicCellCoord ComputeCenterCell(const TArray<FMythicCellCoord> &Cells);

    /**
     * Derive the purpose of the Index-th spawn point in a cell. Pure + static so it's unit-testable and deterministic.
     * A hostile camp's points are all Enemy. Otherwise a deterministic per-(cell,index) roll picks Guard with a fixed
     * minority share (~25%), else Civilian — so a town gets a sprinkling of watch posts among its civilian anchors
     * without any RNG/wall-clock. Salt: (HashCombine(GetTypeHash(Cell), 0x53504E54 ^ Index) & 0xFFFFFF)/16777216.
     * @param Cell     The cell the point lives in.
     * @param Index    The point's index within the cell (0..PerCell-1).
     * @param bHostile The settlement's hostility flag.
     */
    static EMythicSpawnPointPurpose DerivePurpose(const FMythicCellCoord &Cell, int32 Index, bool bHostile);

    /**
     * Transfer this settlement to a new governing faction.
     * Updates settlement data, does NOT re-seed territory (caller responsibility).
     */
    void TransferToFaction(FMythicFactionId NewFaction);

protected:
    virtual void BeginPlay() override;

    /** Edit-time hook: re-runs whenever the actor is placed, moved, or its spline/properties change. Rebuilds the
     *  editor spawn-cell preview so it always reflects the current spline + grid settings. */
    virtual void OnConstruction(const FTransform &Transform) override;

#if WITH_EDITOR
    /** Rebuild the preview when bShowSpawnCellsInEditor or any other property is tweaked in the details panel. */
    virtual void PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent) override;
#endif

private:
    /** Spline component that defines the settlement boundary shape */
    UPROPERTY(VisibleAnywhere, Category = "Settlement")
    TObjectPtr<USplineComponent> BoundarySpline;

    /** Editor-only translucent cube-per-spawn-cell footprint. Hidden in the shipped game (visualization component). */
    UPROPERTY(VisibleAnywhere, Category = "Spawn Preview")
    TObjectPtr<UInstancedStaticMeshComponent> SpawnCellsISM;

    /** Unit cube + its base material, resolved in the constructor (same assets as the runtime debug actor). */
    UPROPERTY()
    TObjectPtr<UStaticMesh> PreviewCellMesh;

    UPROPERTY()
    TObjectPtr<UMaterialInterface> PreviewCellMaterial;

    /** Runtime settlement data (populated on BeginPlay from editor properties + rasterization) */
    FMythicSettlementData SettlementData;

    /** Edit-time: load the territory grid settings, rasterize this settlement's spline, and place one translucent cube
     *  instance per covered cell at the cell's world center (sized to the cell). Clears the instances and bails on any
     *  missing settings / no spline / 0 cells — never crashes the editor. Drawn at the CELL footprint granularity (the
     *  spawner targets each covered cell; population spawn position is the deterministic cell center, see .cpp note). */
    void RebuildSpawnCellPreview();

    /**
     * Point-in-polygon test for the closed spline (ray casting / even-odd rule). Static + pure so it's reusable by the
     * shared static rasterizer (no `this` needed).
     */
    static bool IsPointInsideSpline(const FVector2D &TestPoint, const TArray<FVector2D> &SplinePolygon);

    /**
     * Generate this settlement's navmesh-validated tagged spawn points (server, BeginPlay, game thread, navmesh-ready).
     * For each rasterized cell × SpawnPointsPerCell, calls MythicPlacement::FindValidSpawn near the cell center and, on
     * success, stores the validated foot position + cell + a DerivePurpose-tagged purpose into SettlementData.SpawnPoints.
     * Skips points that fail placement (navmesh not ready / fully built-over) — a sparse or empty result is fine: the
     * population spawner falls back to the cell-center path for any cell with no matching point. Called from BeginPlay
     * AFTER RasterizeSplineToCells (needs the rasterized cells) and BEFORE RegisterSettlement (so the registry's copy
     * carries the points). No-op without a valid grid/world.
     * @param Grid The territory grid (cell→world conversion + cell size for scatter radius).
     */
    void GenerateSpawnPoints(const UMythicTerritoryGrid *Grid);
};

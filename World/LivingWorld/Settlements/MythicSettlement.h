
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/Entity/MythicEntityId.h"
#include "MythicSettlement.generated.h"

class USplineComponent;
class UMythicTerritoryGrid;
class UInstancedStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;


USTRUCT(BlueprintType)
struct MYTHIC_API FMythicShopSlot {
    GENERATED_BODY()

    /** Name of the shop ("The Rusty Anvil") */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FString ShopName;

    /** Role required to run this shop (e.g., "NPC.Role.Merchant.Blacksmith") */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FGameplayTag RequiredRole;

    /** Authority/private canonical identity of the current NPC owner; invalid means vacated. */
    FMythicEntityId OwnerEntityId;

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


UENUM(BlueprintType)
enum class EMythicSpawnPointPurpose : uint8 {
    Civilian = 0 UMETA(DisplayName = "Civilian"),
    Guard UMETA(DisplayName = "Guard"),
    Enemy UMETA(DisplayName = "Enemy"),
    Any UMETA(DisplayName = "Any"),
    COUNT UMETA(Hidden)
};


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

    int32 GetCellCount() const { return RasterizedCells.Num(); }

    int32 GetSpawnPointCount() const { return SpawnPoints.Num(); }
};


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


UCLASS(Blueprintable, meta = (DisplayName = "Settlement"))
class MYTHIC_API AMythicSettlement : public AActor {
    GENERATED_BODY()

public:
    AMythicSettlement();


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


    /**
     * When true (default), the editor viewport shows one translucent cube per territory cell this settlement's spline
     * covers — i.e. the cells the PopulationSpawnerProcessor will fill with MASS NPCs. Editor-only: the instanced-mesh
     * component is hidden in the shipped game. Rebuilt in OnConstruction (so it tracks spline edits / actor moves).
     */
    UPROPERTY(EditAnywhere, Category = "Spawn Preview")
    bool bShowSpawnCellsInEditor = true;


    const FMythicSettlementData &GetSettlementData() const { return SettlementData; }

    USplineComponent *GetBoundarySpline() const { return BoundarySpline; }

    void RasterizeSplineToCells(const UMythicTerritoryGrid *TerritoryGrid);

    static void RasterizeSplineCells(const USplineComponent *Spline, float CellWorldSize, FVector2D WorldOrigin,
                                     int32 GridWidth, int32 GridHeight, TArray<FMythicCellCoord> &OutCells);

    static FMythicCellCoord ComputeCenterCell(const TArray<FMythicCellCoord> &Cells);

    static EMythicSpawnPointPurpose DerivePurpose(const FMythicCellCoord &Cell, int32 Index, bool bHostile);

    void TransferToFaction(FMythicFactionId NewFaction);

protected:
    virtual void BeginPlay() override;

    virtual void OnConstruction(const FTransform &Transform) override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent) override;
#endif

private:
    /** Spline component that defines the settlement boundary shape */
    UPROPERTY(VisibleAnywhere, Category = "Settlement")
    TObjectPtr<USplineComponent> BoundarySpline;

    /** Editor-only translucent cube-per-spawn-cell footprint. Hidden in the shipped game (visualization component). */
    UPROPERTY(VisibleAnywhere, Category = "Spawn Preview")
    TObjectPtr<UInstancedStaticMeshComponent> SpawnCellsISM;

    UPROPERTY()
    TObjectPtr<UStaticMesh> PreviewCellMesh;

    UPROPERTY()
    TObjectPtr<UMaterialInterface> PreviewCellMaterial;

    FMythicSettlementData SettlementData;

    void RebuildSpawnCellPreview();

    static bool IsPointInsideSpline(const FVector2D &TestPoint, const TArray<FVector2D> &SplinePolygon);

    void GenerateSpawnPoints(const UMythicTerritoryGrid *Grid);
};

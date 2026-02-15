// Mythic Living World — Settlement System
// Spline-based settlement actors that seed territory and drive population spawning.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "MythicSettlement.generated.h"

class USplineComponent;
class UMythicTerritoryGrid;

// ─────────────────────────────────────────────────────────────
// Settlement Data — Runtime state for a single settlement
// ─────────────────────────────────────────────────────────────

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

    /** If true, this settlement is the faction's capital — receives significance boost */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settlement")
    bool bIsCapital = false;

    /** Unique runtime ID assigned by the settlement registry */
    UPROPERTY(BlueprintReadOnly, Category = "Settlement")
    int32 SettlementId = INDEX_NONE;

    /** Number of MASS entities currently spawned in this settlement's cells */
    UPROPERTY(BlueprintReadOnly, Category = "Population")
    int32 CurrentEntityCount = 0;

    /** Get the number of territory cells this settlement covers */
    int32 GetCellCount() const { return RasterizedCells.Num(); }
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
     * Transfer this settlement to a new governing faction.
     * Updates settlement data, does NOT re-seed territory (caller responsibility).
     */
    void TransferToFaction(FMythicFactionId NewFaction);

protected:
    virtual void BeginPlay() override;

private:
    /** Spline component that defines the settlement boundary shape */
    UPROPERTY(VisibleAnywhere, Category = "Settlement")
    TObjectPtr<USplineComponent> BoundarySpline;

    /** Runtime settlement data (populated on BeginPlay from editor properties + rasterization) */
    FMythicSettlementData SettlementData;

    /**
     * Point-in-polygon test for the closed spline.
     * Uses ray casting algorithm against the spline's sampled polygon.
     */
    bool IsPointInsideSpline(const FVector2D &TestPoint, const TArray<FVector2D> &SplinePolygon) const;
};

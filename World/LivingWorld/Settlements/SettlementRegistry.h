// Mythic Living World — Settlement Registry
// Central registry tracking all active settlements. Owned by the Living World Subsystem.

#pragma once

#include "CoreMinimal.h"
#include "World/LivingWorld/Settlements/MythicSettlement.h"
#include "SettlementRegistry.generated.h"

class UMythicCausalFabric;
class UMythicTerritoryGrid;
class UMythicFactionDatabase;



/**
 * Central registry for all active settlements in the world.
 *
 * Provides O(1) cell→settlement lookups, per-faction settlement lists,
 * and handles settlement transfer (faction hand-off + territory re-seed + event logging).
 *
 * Owned by UMythicLivingWorldSubsystem. Settlement actors register/unregister
 * themselves on BeginPlay/EndPlay.
 */
UCLASS()
class MYTHIC_API UMythicSettlementRegistry : public UObject {
    GENERATED_BODY()

public:
    /**
     * Register a settlement actor with the registry.
     * Assigns a unique settlement ID and builds cell→settlement index.
     * @return The assigned settlement ID
     */
    int32 RegisterSettlement(AMythicSettlement *Settlement);

    /** Unregister a settlement (e.g., when its actor is destroyed or level unloads) */
    void UnregisterSettlement(AMythicSettlement *Settlement);

    // ─── Queries ──────────────────────────────────────────

    /** Get settlement data by ID. Returns nullptr if not found. */
    const FMythicSettlementData *GetSettlementData(int32 SettlementId) const;
    FMythicSettlementData *GetMutableSettlementData(int32 SettlementId);

    /** Get the settlement that contains a given cell. Returns nullptr if no settlement covers that cell. */
    const FMythicSettlementData *GetSettlementAtCell(const FMythicCellCoord &Cell) const;

    /** Get the settlement actor by ID. Returns nullptr if not found or actor destroyed. */
    AMythicSettlement *GetSettlementActor(int32 SettlementId) const;

    /** Get all settlement IDs belonging to a faction */
    void GetSettlementsForFaction(FMythicFactionId Faction, TArray<int32> &OutSettlementIds) const;

    /** Get total number of registered settlements */
    int32 GetSettlementCount() const { return Settlements.Num(); }

    /** Get all registered settlement IDs */
    void GetAllSettlementIds(TArray<int32> &OutIds) const;

    // ─── Shop Succession (Phase 7) ───────────────────────

    /**
     * Called when a persistent NPC dies.
     * Checks if the NPC owned any shops, vacates them, and starts succession timer.
     * @param DeadEntityId  ID of the dead NPC
     * @param DeathTime     Current world time
     */
    void HandleNPCDeath(uint32 DeadEntityId, double DeathTime);

    /**
     * Ticked by WorldSimThread to handle shop succession timers.
     * Scans vacated shops; if VacatedTime > threshold, creates a new NPC
     * template and assigns them to the shop.
     * @param CurrentWorldTime  Current game time
     * @param SuccessionDelay   How long before a shop is replaced
     */
    void TickShopSuccession(double CurrentWorldTime, double SuccessionDelay);

    // ─── Settlement Transfer ─────────────────────────────

    /**
     * Transfer a settlement to a new faction.
     * 1. Updates settlement data
     * 2. Re-seeds territory grid cells with new faction
     * 3. Updates faction ControlledCellCount in faction DB
     * 4. Logs a Causal Fabric event (Territory type)
     */
    void TransferSettlement(
        int32 SettlementId,
        FMythicFactionId NewFaction,
        UMythicTerritoryGrid *TerritoryGrid,
        UMythicFactionDatabase *FactionDB,
        UMythicCausalFabric *CausalFabric
        );

    // ─── Territory Seeding ───────────────────────────────

    /**
     * Seed territory grid from all registered settlements.
     * Sets influence to 1.0 for each settlement's cells under its governing faction.
     * Called once during initialization after all settlements are registered.
     */
    void SeedTerritoryFromSettlements(UMythicTerritoryGrid *TerritoryGrid, UMythicFactionDatabase *FactionDB);

private:
    /** Map of settlement ID → settlement data */
    TMap<int32, FMythicSettlementData> Settlements;

    /** Map of settlement ID → weak pointer to settlement actor */
    TMap<int32, TWeakObjectPtr<AMythicSettlement>> SettlementActors;

    /** Cell → settlement ID lookup for O(1) spatial queries */
    TMap<FMythicCellCoord, int32> CellToSettlement;

    /** Per-faction settlement ID lists (cached for fast lookup) */
    TMap<FMythicFactionId, TArray<int32>> FactionSettlements;

    /** Next settlement ID to assign */
    int32 NextSettlementId = 0;

    /** Rebuild the cell→settlement and faction→settlements indices from scratch */
    void RebuildIndices();
};


#pragma once

#include "CoreMinimal.h"
#include "World/LivingWorld/Settlements/MythicSettlement.h"
#include "SettlementRegistry.generated.h"

class UMythicCausalFabric;
class UMythicTerritoryGrid;
class UMythicFactionDatabase;


UCLASS()
class MYTHIC_API UMythicSettlementRegistry : public UObject {
    GENERATED_BODY()

public:
    int32 RegisterSettlement(AMythicSettlement *Settlement);

    void UnregisterSettlement(AMythicSettlement *Settlement);


    const FMythicSettlementData *GetSettlementData(int32 SettlementId) const;
    FMythicSettlementData *GetMutableSettlementData(int32 SettlementId);

    const FMythicSettlementData *GetSettlementAtCell(const FMythicCellCoord &Cell) const;

    AMythicSettlement *GetSettlementActor(int32 SettlementId) const;

    void GetSettlementsForFaction(FMythicFactionId Faction, TArray<int32> &OutSettlementIds) const;

    int32 GetSettlementCount() const { return Settlements.Num(); }

    void GetAllSettlementIds(TArray<int32> &OutIds) const;


    void HandleNPCDeath(const FMythicEntityId &DeadEntityId, double DeathTime);

    void TickShopSuccession(double CurrentWorldTime, double SuccessionDelay);

    int32 ClaimVacantShop(int32 SettlementId, const FMythicEntityId &ClaimantEntityId,
                         const FGameplayTag &ClaimantRole);

    /** Returns true when any durable NPC shop-ownership slot still refers to the exact canonical entity. */
    bool ReferencesEntityIdentity(const FMythicEntityId &EntityId) const;

    /**
     * Clears NPC shop-owner assignments whose logical person cannot be reconstructed after a LivingWorld restore.
     * Cleared shops become immediately eligible for ordinary role-gated succession.
     */
    int32 ClearUnrestorableShopOwners(
        const TSet<FMythicEntityId> &RestorableEntityIds);

    /** Clears one failed-to-rehydrate NPC shop owner without misreporting a death event. */
    bool ClearUnrestorableShopOwner(const FMythicEntityId &EntityId);

    static bool CanClaimShop(const FMythicShopSlot &Shop, const FGameplayTag &ClaimantRole);


    void TransferSettlement(
        int32 SettlementId,
        FMythicFactionId NewFaction,
        UMythicTerritoryGrid *TerritoryGrid,
        UMythicFactionDatabase *FactionDB,
        UMythicCausalFabric *CausalFabric
        );

    void TickConquest(
        UMythicTerritoryGrid *TerritoryGrid,
        UMythicFactionDatabase *FactionDB,
        UMythicCausalFabric *CausalFabric,
        float ConquestThreshold
        );


    void SeedTerritoryFromSettlements(UMythicTerritoryGrid *TerritoryGrid, UMythicFactionDatabase *FactionDB);

    virtual void Serialize(FArchive &Ar) override;

private:
    TMap<int32, FMythicSettlementData> Settlements;

    TMap<int32, TWeakObjectPtr<AMythicSettlement>> SettlementActors;

    TMap<FMythicCellCoord, int32> CellToSettlement;

    TMap<FMythicFactionId, TArray<int32>> FactionSettlements;

    int32 NextSettlementId = 0;

    TMap<FName, uint8> LoadedGoverningFactionOverrides;

    void RebuildIndices();
};

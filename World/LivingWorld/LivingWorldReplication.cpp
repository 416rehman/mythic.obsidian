#include "World/LivingWorld/LivingWorldReplication.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "Net/UnrealNetwork.h"
#include "World/LivingWorld/LivingWorldSettings.h"

AMythicLivingWorldReplicator::AMythicLivingWorldReplicator() {
    bReplicates = true;
    bAlwaysRelevant = true;
    PrimaryActorTick.bCanEverTick = false;
}

void AMythicLivingWorldReplicator::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AMythicLivingWorldReplicator, FactionProxies);
    DOREPLIFETIME(AMythicLivingWorldReplicator, TerritoryProxies);
}

void AMythicLivingWorldReplicator::SyncProxies(UMythicLivingWorldSubsystem* Subsystem) {
    if (!HasAuthority() || !Subsystem) return;

    // ─── Sync Factions ───
    UMythicFactionDatabase* FactionDB = Subsystem->GetFactionDatabase();
    if (FactionDB) {
        bool bFactionsChanged = false;
        
        // Loop through all valid factions. Only sync active ones.
        for (uint8 i = 0; i < FactionDB->GetMaxFactions(); ++i) {
            FMythicFactionId FactionId;
            FactionId.Index = i;

            FMythicFactionData FData;
            if (FactionDB->GetFaction(FactionId, FData)) {
                if (FData.Status == EMythicFactionStatus::Dormant || FData.Status == EMythicFactionStatus::Annihilated) {
                    continue;
                }
                // Find or add proxy via O(1) map
                FMythicFactionProxyItem* FoundProxy = nullptr;
                if (int32* ProxyIdxPtr = FactionProxyIndex.Find(FactionId)) {
                    FoundProxy = &FactionProxies.Items[*ProxyIdxPtr];
                }

                if (!FoundProxy) {
                    FMythicFactionProxyItem NewProxy;
                    NewProxy.FactionId = FactionId;
                    int32 NewIdx = FactionProxies.Items.Add(NewProxy);
                    FactionProxyIndex.Add(FactionId, NewIdx);
                    FoundProxy = &FactionProxies.Items[NewIdx];
                    FactionProxies.MarkItemDirty(*FoundProxy);
                    bFactionsChanged = true;
                }

                // Update data if changed
                if (FoundProxy->Status != FData.Status || 
                    FoundProxy->Population != FData.Population ||
                    FoundProxy->ControlledCellCount != FData.ControlledCellCount) {
                    
                    FoundProxy->Status = FData.Status;
                    FoundProxy->Population = FData.Population;
                    FoundProxy->ControlledCellCount = FData.ControlledCellCount;
                    
                    // Wealth mapping (0 to proxy's 255)
                    FoundProxy->WealthLevel = FMath::Clamp(FMath::RoundToInt(FData.Reserves.Wealth), 0, 255);
                    FoundProxy->MilitaryStrength = FMath::Clamp(FMath::RoundToInt(FData.MilitaryStrength * 255.0f), 0, 255);
                    
                    FactionProxies.MarkItemDirty(*FoundProxy);
                    bFactionsChanged = true;
                }
            }
        }

        if (bFactionsChanged) {
            FactionProxies.MarkArrayDirty();
        }
    }

    // ─── Sync Territory (Simplified Deltas) ───
    UMythicTerritoryGrid* Grid = Subsystem->GetTerritoryGrid();
    if (Grid) {
        TArray<FMythicCellCoord> ChangedCells;
        Grid->GetChangedCells(ChangedCells);
        
        bool bTerritoryChanged = false;
        for (const FMythicCellCoord& Cell : ChangedCells) {
            FMythicFactionId ControllingFaction = Grid->GetDominantFaction(Cell);
            
            FMythicTerritoryProxyItem* FoundProxy = nullptr;
            if (int32* ProxyIdxPtr = TerritoryProxyIndex.Find(Cell)) {
                FoundProxy = &TerritoryProxies.Items[*ProxyIdxPtr];
            }
            
            if (!FoundProxy) {
                FMythicTerritoryProxyItem NewProxy;
                NewProxy.Cell = Cell;
                int32 NewIdx = TerritoryProxies.Items.Add(NewProxy);
                TerritoryProxyIndex.Add(Cell, NewIdx);
                FoundProxy = &TerritoryProxies.Items[NewIdx];
            }
            
            FoundProxy->ControllingFaction = ControllingFaction;
            FoundProxy->ContestedLevel = 0;
            
            TerritoryProxies.MarkItemDirty(*FoundProxy);
            bTerritoryChanged = true;
        }
        
        if (bTerritoryChanged) {
            TerritoryProxies.MarkArrayDirty();
        }
    }
}


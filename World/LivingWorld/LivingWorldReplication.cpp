#include "LivingWorldReplication.h"
#include "World/LivingWorld/LivingWorldReplication.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/Encounters/EncounterDirector.h"
#include "World/LivingWorld/Settlements/MythicSettlement.h" // FMythicSettlementData (complete type for settlement sync)
#include "Net/UnrealNetwork.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

AMythicLivingWorldReplicator::AMythicLivingWorldReplicator() {
    bReplicates = true;
    bAlwaysRelevant = true;
    PrimaryActorTick.bCanEverTick = false;

    // Link the FastArrays back to this actor so their client-side item callbacks can reach the subsystem. Set in
    // the ctor (runs on both server + client) so the link exists before any replication delta arrives.
    FactionProxies.OwnerReplicator = this;
    TerritoryProxies.OwnerReplicator = this;
    EncounterProxies.OwnerReplicator = this;
    SettlementProxies.OwnerReplicator = this;
}

void AMythicLivingWorldReplicator::BeginPlay() {
    Super::BeginPlay();

    // CLIENT: the subsystem never learns about this server-spawned, replicated-in actor on its own (it only sets
    // its Replicator pointer on the server, at SpawnActor). Register so the subsystem's proxy accessors + change
    // delegate work client-side.
    if (!HasAuthority()) {
        if (const UWorld *World = GetWorld()) {
            if (UGameInstance *GI = World->GetGameInstance()) {
                if (UMythicLivingWorldSubsystem *Sub = GI->GetSubsystem<UMythicLivingWorldSubsystem>()) {
                    ClientSubsystem = Sub;
                    Sub->RegisterClientReplicator(this);
                }
            }
        }
    }
}

void AMythicLivingWorldReplicator::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    if (!HasAuthority()) {
        if (UMythicLivingWorldSubsystem *Sub = ClientSubsystem.Get()) {
            Sub->RegisterClientReplicator(nullptr);
        }
    }
    ClientSubsystem.Reset();
    Super::EndPlay(EndPlayReason);
}

void AMythicLivingWorldReplicator::NotifyClientProxiesChanged() {
    if (UMythicLivingWorldSubsystem *Sub = ClientSubsystem.Get()) {
        Sub->OnLivingWorldProxiesChanged.Broadcast();
    }
}

const FMythicFactionProxyItem *AMythicLivingWorldReplicator::GetFactionProxy(FMythicFactionId FactionId) const {
    for (const FMythicFactionProxyItem &Item : FactionProxies.Items) {
        if (Item.FactionId == FactionId) {
            return &Item;
        }
    }
    return nullptr;
}

bool AMythicLivingWorldReplicator::GetTerritoryProxy(FMythicCellCoord Cell, FMythicTerritoryProxyItem &OutProxy) const {
    for (const FMythicTerritoryProxyItem &Item : TerritoryProxies.Items) {
        if (Item.Cell == Cell) {
            OutProxy = Item;
            return true;
        }
    }
    return false;
}

// ─── FastArray item callbacks (client ingest) ───
// All route to the owner so the client subsystem broadcasts its change delegate (UI/gameplay react). On the
// server these don't fire (it is the source of truth). Defined here so the array + replicator types are complete.
void FMythicFactionProxyItem::PostReplicatedAdd(const FMythicFactionProxyArray &InArraySerializer) {
    if (InArraySerializer.OwnerReplicator) { InArraySerializer.OwnerReplicator->NotifyClientProxiesChanged(); }
}

void FMythicFactionProxyItem::PostReplicatedChange(const FMythicFactionProxyArray &InArraySerializer) {
    if (InArraySerializer.OwnerReplicator) { InArraySerializer.OwnerReplicator->NotifyClientProxiesChanged(); }
}

void FMythicFactionProxyItem::PreReplicatedRemove(const FMythicFactionProxyArray &InArraySerializer) {
    if (InArraySerializer.OwnerReplicator) { InArraySerializer.OwnerReplicator->NotifyClientProxiesChanged(); }
}

void FMythicTerritoryProxyItem::PostReplicatedAdd(const FMythicTerritoryProxyArray &InArraySerializer) {
    if (InArraySerializer.OwnerReplicator) { InArraySerializer.OwnerReplicator->NotifyClientProxiesChanged(); }
}

void FMythicTerritoryProxyItem::PostReplicatedChange(const FMythicTerritoryProxyArray &InArraySerializer) {
    if (InArraySerializer.OwnerReplicator) { InArraySerializer.OwnerReplicator->NotifyClientProxiesChanged(); }
}

void FMythicTerritoryProxyItem::PreReplicatedRemove(const FMythicTerritoryProxyArray &InArraySerializer) {
    if (InArraySerializer.OwnerReplicator) { InArraySerializer.OwnerReplicator->NotifyClientProxiesChanged(); }
}

void FMythicEncounterProxyItem::PostReplicatedAdd(const FMythicEncounterProxyArray &InArraySerializer) {
    if (InArraySerializer.OwnerReplicator) { InArraySerializer.OwnerReplicator->NotifyClientProxiesChanged(); }
}

void FMythicEncounterProxyItem::PostReplicatedChange(const FMythicEncounterProxyArray &InArraySerializer) {
    if (InArraySerializer.OwnerReplicator) { InArraySerializer.OwnerReplicator->NotifyClientProxiesChanged(); }
}

void FMythicEncounterProxyItem::PreReplicatedRemove(const FMythicEncounterProxyArray &InArraySerializer) {
    if (InArraySerializer.OwnerReplicator) { InArraySerializer.OwnerReplicator->NotifyClientProxiesChanged(); }
}

void FMythicSettlementProxyItem::PostReplicatedAdd(const FMythicSettlementProxyArray &InArraySerializer) {
    if (InArraySerializer.OwnerReplicator) { InArraySerializer.OwnerReplicator->NotifyClientProxiesChanged(); }
}

void FMythicSettlementProxyItem::PostReplicatedChange(const FMythicSettlementProxyArray &InArraySerializer) {
    if (InArraySerializer.OwnerReplicator) { InArraySerializer.OwnerReplicator->NotifyClientProxiesChanged(); }
}

void FMythicSettlementProxyItem::PreReplicatedRemove(const FMythicSettlementProxyArray &InArraySerializer) {
    if (InArraySerializer.OwnerReplicator) { InArraySerializer.OwnerReplicator->NotifyClientProxiesChanged(); }
}

void AMythicLivingWorldReplicator::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AMythicLivingWorldReplicator, FactionProxies);
    DOREPLIFETIME(AMythicLivingWorldReplicator, TerritoryProxies);
    DOREPLIFETIME(AMythicLivingWorldReplicator, EncounterProxies);
    DOREPLIFETIME(AMythicLivingWorldReplicator, SettlementProxies);
}

bool AMythicLivingWorldReplicator::TerritoryProxyNeedsUpdate(const FMythicTerritoryProxyItem &Existing, FMythicFactionId NewFaction, uint8 NewContestedLevel) {
    // Compare only the client-visible fields. (FactionId identity is its Index.)
    return Existing.ControllingFaction.Index != NewFaction.Index || Existing.ContestedLevel != NewContestedLevel;
}

void AMythicLivingWorldReplicator::SyncProxies(UMythicLivingWorldSubsystem *Subsystem) {
    if (!HasAuthority() || !Subsystem) {
        return;
    }

    // ─── Sync Factions ───
    UMythicFactionDatabase *FactionDB = Subsystem->GetFactionDatabase();
    if (FactionDB) {
        bool bFactionsChanged = false;

        // WealthLevel is a [0..255]-mapped-to-[0..MaxReserve] field (per its replication doc-comment), mirroring
        // MilitaryStrength's [0..255]-mapped-to-[0..1]. Normalize against the sim's reserve clamp so the byte uses
        // its full range instead of shipping the raw reserve (which only spanned 0..~MaxReserve and collapsed all
        // debt to 0). Computed once — MaxReserve is global.
        const UMythicLivingWorldSettings *LWSettings = Subsystem->GetSettings();
        const float MaxReserve = (LWSettings ? FMath::Max(LWSettings->MaxReserve, 1.0f) : 100.0f);

        // Loop through all valid factions. Only sync active ones.
        for (uint8 i = 0; i < FactionDB->GetMaxFactions(); ++i) {
            FMythicFactionId FactionId;
            FactionId.Index = i;

            FMythicFactionData FData;
            if (FactionDB->GetFaction(FactionId, FData)) {
                if (FData.Status == EMythicFactionStatus::Dormant || FData.Status == EMythicFactionStatus::Annihilated) {
                    // Dead/dormant: REMOVE any existing proxy so clients stop seeing the faction as alive. The old
                    // `continue` simply skipped it, so a faction that died AFTER its proxy was created left a stale
                    // "still alive" zombie proxy on every client. RemoveAtSwap + patch the swapped item's cached index
                    // (O(1)); MarkArrayDirty replicates the removal (clients drop it via PostReplicatedRemove).
                    if (int32 *DeadIdxPtr = FactionProxyIndex.Find(FactionId)) {
                        const int32 RemovedIdx = *DeadIdxPtr;
                        FactionProxyIndex.Remove(FactionId);
                        FactionProxies.Items.RemoveAtSwap(RemovedIdx);
                        if (RemovedIdx < FactionProxies.Items.Num()) {
                            // RemoveAtSwap moved the former-last proxy into RemovedIdx — fix its cached index.
                            FactionProxyIndex[FactionProxies.Items[RemovedIdx].FactionId] = RemovedIdx;
                        }
                        FactionProxies.MarkArrayDirty();
                        bFactionsChanged = true;
                    }
                    continue;
                }
                // Find or add proxy via O(1) map
                FMythicFactionProxyItem *FoundProxy = nullptr;
                if (int32 *ProxyIdxPtr = FactionProxyIndex.Find(FactionId)) {
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

                // Update data if changed. Compute the mapped wealth/military up front and include them in the
                // change test — otherwise a wealth- or military-ONLY shift (Status/Population/Cells unchanged) never
                // dirties the proxy, so those values silently never replicate to clients (stale faction UI).
                const uint8 NewWealth = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(FMath::Max(FData.Reserves.Wealth, 0.0f) / MaxReserve * 255.0f), 0,
                                                                        255));
                const uint8 NewMilitary = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(FData.MilitaryStrength * 255.0f), 0, 255));
                if (FoundProxy->Status != FData.Status ||
                    FoundProxy->Population != FData.Population ||
                    FoundProxy->ControlledCellCount != FData.ControlledCellCount ||
                    FoundProxy->WealthLevel != NewWealth ||
                    FoundProxy->MilitaryStrength != NewMilitary) {

                    FoundProxy->Status = FData.Status;
                    FoundProxy->Population = FData.Population;
                    FoundProxy->ControlledCellCount = FData.ControlledCellCount;
                    FoundProxy->WealthLevel = NewWealth;
                    FoundProxy->MilitaryStrength = NewMilitary;

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
    UMythicTerritoryGrid *Grid = Subsystem->GetTerritoryGrid();
    if (Grid) {
        TArray<FMythicCellCoord> ChangedCells;
        Grid->GetChangedCells(ChangedCells);

        bool bTerritoryChanged = false;
        for (const FMythicCellCoord &Cell : ChangedCells) {
            FMythicFactionId ControllingFaction = Grid->GetDominantFaction(Cell);

            // ContestedLevel is not yet computed from the grid — always 0 for now. (The proxy field replicates, but the
            // contested-cell signal never reaches clients; logged as a half-wired field in backlog.)
            constexpr uint8 NewContested = 0;

            // Change-detect like the faction sync above. GetChangedCells returns every INFLUENCE-changed cell, but a
            // cell's influence often shifts WITHOUT flipping its dominant faction; re-marking those proxies dirty would
            // re-replicate an unchanged ControllingFaction to every client each territory tick (bandwidth waste that
            // defeats the delta). Only dirty when the client-visible fields actually change. New cells always dirty.
            if (int32 *ProxyIdxPtr = TerritoryProxyIndex.Find(Cell)) {
                FMythicTerritoryProxyItem &Proxy = TerritoryProxies.Items[*ProxyIdxPtr];
                if (TerritoryProxyNeedsUpdate(Proxy, ControllingFaction, NewContested)) {
                    Proxy.ControllingFaction = ControllingFaction;
                    Proxy.ContestedLevel = NewContested;
                    TerritoryProxies.MarkItemDirty(Proxy);
                    bTerritoryChanged = true;
                }
            }
            else {
                FMythicTerritoryProxyItem NewProxy;
                NewProxy.Cell = Cell;
                NewProxy.ControllingFaction = ControllingFaction;
                NewProxy.ContestedLevel = NewContested;
                const int32 NewIdx = TerritoryProxies.Items.Add(NewProxy);
                TerritoryProxyIndex.Add(Cell, NewIdx);
                TerritoryProxies.MarkItemDirty(TerritoryProxies.Items[NewIdx]);
                bTerritoryChanged = true;
            }
        }

        if (bTerritoryChanged) {
            TerritoryProxies.MarkArrayDirty();
        }
    }

    // ─── Sync Encounters (added on spawn, REMOVED on completion — unlike factions/territory which only grow) ───
    if (UMythicEncounterDirector *EncounterDir = GetWorld() ? GetWorld()->GetSubsystem<UMythicEncounterDirector>() : nullptr) {
        const TArray<FMythicActiveEncounter> &Active = EncounterDir->GetActiveEncounters();
        bool bEncountersChanged = false;

        // Remove proxies whose encounter is no longer active (completed/cleaned up). Counts are tiny (<= ~10), so
        // the linear scans are trivial — no index map to maintain across removals.
        for (int32 i = EncounterProxies.Items.Num() - 1; i >= 0; --i) {
            const uint32 ProxyId = EncounterProxies.Items[i].EncounterId;
            if (!Active.ContainsByPredicate([ProxyId](const FMythicActiveEncounter &E) { return E.EncounterId == ProxyId; })) {
                EncounterProxies.Items.RemoveAt(i);
                bEncountersChanged = true;
            }
        }

        // Add new encounters + update the (mutable) state of existing ones.
        for (const FMythicActiveEncounter &Enc : Active) {
            FMythicEncounterProxyItem *Proxy = EncounterProxies.Items.FindByPredicate(
                [&Enc](const FMythicEncounterProxyItem &P) { return P.EncounterId == Enc.EncounterId; });
            if (!Proxy) {
                FMythicEncounterProxyItem NewProxy;
                NewProxy.EncounterId = Enc.EncounterId;
                NewProxy.TemplateTag = Enc.TemplateTag;
                NewProxy.State = Enc.State;
                NewProxy.Cell = Enc.Cell;
                NewProxy.OriginFaction = Enc.OriginFaction;
                const int32 NewIdx = EncounterProxies.Items.Add(NewProxy);
                EncounterProxies.MarkItemDirty(EncounterProxies.Items[NewIdx]);
                bEncountersChanged = true;
            }
            else if (Proxy->State != Enc.State) {
                Proxy->State = Enc.State; // type/cell/faction are fixed for an encounter's life; only state evolves
                EncounterProxies.MarkItemDirty(*Proxy);
                bEncountersChanged = true;
            }
        }

        if (bEncountersChanged) {
            EncounterProxies.MarkArrayDirty();
        }
    }

    // ─── Sync Settlements (added on registration; GoverningFaction / capital can change on conquest) ───
    // Walk the registry through the subsystem's thread-safe copy-out helpers (the sim thread mutates Settlements under
    // SimulationLock — CopyAllSettlementIds / CopySettlementById snapshot by value under that lock). Settlement counts
    // are tiny, so the linear scans / by-value copies are trivial. Settlements persist for the level's life (placed
    // actors), so unlike encounters they are not removed here; only adds + field updates are synced.
    {
        TArray<int32> SettlementIds;
        Subsystem->CopyAllSettlementIds(SettlementIds);

        bool bSettlementsChanged = false;
        for (const int32 SettlementId : SettlementIds) {
            FMythicSettlementData SData;
            if (!Subsystem->CopySettlementById(SettlementId, SData)) {
                continue;
            }

            FMythicSettlementProxyItem *Proxy = SettlementProxies.Items.FindByPredicate(
                [SettlementId](const FMythicSettlementProxyItem &P) { return P.SettlementId == SettlementId; });

            if (!Proxy) {
                FMythicSettlementProxyItem NewProxy;
                NewProxy.SettlementId = SettlementId;
                NewProxy.CenterCell = SData.CenterCell;
                NewProxy.GoverningFaction = SData.GoverningFaction;
                NewProxy.DisplayName = SData.DisplayName;
                NewProxy.bIsCapital = SData.bIsCapital;
                const int32 NewIdx = SettlementProxies.Items.Add(NewProxy);
                SettlementProxies.MarkItemDirty(SettlementProxies.Items[NewIdx]);
                bSettlementsChanged = true;
            }
            else if (Proxy->GoverningFaction.Index != SData.GoverningFaction.Index ||
                     Proxy->bIsCapital != SData.bIsCapital ||
                     Proxy->CenterCell != SData.CenterCell) {
                // Conquest flips GoverningFaction; CenterCell/capital can change after a (re)rasterize. Name is static.
                Proxy->CenterCell = SData.CenterCell;
                Proxy->GoverningFaction = SData.GoverningFaction;
                Proxy->bIsCapital = SData.bIsCapital;
                SettlementProxies.MarkItemDirty(*Proxy);
                bSettlementsChanged = true;
            }
        }

        if (bSettlementsChanged) {
            SettlementProxies.MarkArrayDirty();
        }
    }
}

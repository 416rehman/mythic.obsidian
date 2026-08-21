#include "LivingWorldReplication.h"
#include "World/LivingWorld/LivingWorldReplication.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/Encounters/EncounterDirector.h"
#include "World/LivingWorld/Settlements/MythicSettlement.h"
#include "Net/UnrealNetwork.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

AMythicLivingWorldReplicator::AMythicLivingWorldReplicator() {
    bReplicates = true;
    bAlwaysRelevant = true;
    PrimaryActorTick.bCanEverTick = false;

    FactionProxies.OwnerReplicator = this;
    TerritoryProxies.OwnerReplicator = this;
    EncounterProxies.OwnerReplicator = this;
    SettlementProxies.OwnerReplicator = this;
}

void AMythicLivingWorldReplicator::BeginPlay() {
    Super::BeginPlay();

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
    return Existing.ControllingFaction.Index != NewFaction.Index || Existing.ContestedLevel != NewContestedLevel;
}

void AMythicLivingWorldReplicator::SyncProxies(UMythicLivingWorldSubsystem *Subsystem) {
    if (!HasAuthority() || !Subsystem) {
        return;
    }

    UMythicFactionDatabase *FactionDB = Subsystem->GetFactionDatabase();
    if (FactionDB) {
        bool bFactionsChanged = false;

        const UMythicLivingWorldSettings *LWSettings = Subsystem->GetSettings();
        const float MaxReserve = (LWSettings ? FMath::Max(LWSettings->MaxReserve, 1.0f) : 100.0f);

        for (uint8 i = 0; i < FactionDB->GetMaxFactions(); ++i) {
            FMythicFactionId FactionId;
            FactionId.Index = i;

            FMythicFactionData FData;
            if (FactionDB->GetFaction(FactionId, FData)) {
                if (FData.Status == EMythicFactionStatus::Dormant || FData.Status == EMythicFactionStatus::Annihilated) {
                    if (int32 *DeadIdxPtr = FactionProxyIndex.Find(FactionId)) {
                        const int32 RemovedIdx = *DeadIdxPtr;
                        FactionProxyIndex.Remove(FactionId);
                        FactionProxies.Items.RemoveAtSwap(RemovedIdx);
                        if (RemovedIdx < FactionProxies.Items.Num()) {
                            FactionProxyIndex[FactionProxies.Items[RemovedIdx].FactionId] = RemovedIdx;
                        }
                        FactionProxies.MarkArrayDirty();
                        bFactionsChanged = true;
                    }
                    continue;
                }
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

    UMythicTerritoryGrid *Grid = Subsystem->GetTerritoryGrid();
    if (Grid) {
        TArray<FMythicCellCoord> ChangedCells;
        Grid->GetChangedCells(ChangedCells);

        bool bTerritoryChanged = false;
        for (const FMythicCellCoord &Cell : ChangedCells) {
            FMythicFactionId ControllingFaction = Grid->GetDominantFaction(Cell);

            constexpr uint8 NewContested = 0;

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

    if (UMythicEncounterDirector *EncounterDir = GetWorld() ? GetWorld()->GetSubsystem<UMythicEncounterDirector>() : nullptr) {
        const TArray<FMythicActiveEncounter> &Active = EncounterDir->GetActiveEncounters();
        bool bEncountersChanged = false;

        for (int32 i = EncounterProxies.Items.Num() - 1; i >= 0; --i) {
            const uint32 ProxyId = EncounterProxies.Items[i].EncounterId;
            if (!Active.ContainsByPredicate([ProxyId](const FMythicActiveEncounter &E) { return E.EncounterId == ProxyId; })) {
                EncounterProxies.Items.RemoveAt(i);
                bEncountersChanged = true;
            }
        }

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
                Proxy->State = Enc.State;
                EncounterProxies.MarkItemDirty(*Proxy);
                bEncountersChanged = true;
            }
        }

        if (bEncountersChanged) {
            EncounterProxies.MarkArrayDirty();
        }
    }

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

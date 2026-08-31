
#include "World/LivingWorld/Settlements/SettlementRegistry.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/CausalFabric/CausalFabric.h"
#include "World/LivingWorld/MythicTags_LivingWorld.h"

int32 UMythicSettlementRegistry::RegisterSettlement(AMythicSettlement *Settlement) {
    if (!Settlement) {
        UE_LOG(LogMythSettlement, Warning, TEXT("Attempted to register null settlement."));
        return INDEX_NONE;
    }

    const int32 AssignedId = NextSettlementId++;

    FMythicSettlementData Data = Settlement->GetSettlementData();
    Data.SettlementId = AssignedId;

    const FName SettlementKey = Data.SettlementTag.GetTagName();
    if (SettlementKey.IsNone()) {
        UE_LOG(LogMythSettlement, Warning,
               TEXT("Settlement '%s' (ID=%d) has no SettlementTag — its conquered governance will NOT persist across save/load."),
               *Data.DisplayName.ToString(), AssignedId);
    }
    else if (const uint8 *SavedFaction = LoadedGoverningFactionOverrides.Find(SettlementKey)) {
        Data.GoverningFaction.Index = *SavedFaction;
        LoadedGoverningFactionOverrides.Remove(SettlementKey);
    }

    Settlements.Add(AssignedId, MoveTemp(Data));
    SettlementActors.Add(AssignedId, Settlement);

    const FMythicSettlementData &StoredData = Settlements[AssignedId];
    for (const FMythicCellCoord &Cell : StoredData.RasterizedCells) {
        if (!CellToSettlement.Contains(Cell)) {
            CellToSettlement.Add(Cell, AssignedId);
        }
    }

    FactionSettlements.FindOrAdd(StoredData.GoverningFaction).Add(AssignedId);

    UE_LOG(LogMythSettlement, Log, TEXT("Registered settlement '%s' (ID=%d, Faction=%d, Cells=%d)"),
           *StoredData.DisplayName.ToString(), AssignedId, StoredData.GoverningFaction.Index, StoredData.RasterizedCells.Num());

    return AssignedId;
}

void UMythicSettlementRegistry::UnregisterSettlement(AMythicSettlement *Settlement) {
    if (!Settlement) {
        return;
    }

    int32 FoundId = INDEX_NONE;
    for (const auto &Pair : SettlementActors) {
        if (Pair.Value.Get() == Settlement) {
            FoundId = Pair.Key;
            break;
        }
    }

    if (FoundId == INDEX_NONE) {
        return;
    }

    const FMythicSettlementData *Data = Settlements.Find(FoundId);
    if (Data) {
        for (const FMythicCellCoord &Cell : Data->RasterizedCells) {
            if (CellToSettlement.FindRef(Cell) == FoundId) {
                CellToSettlement.Remove(Cell);
            }
        }

        TArray<int32> *FactionList = FactionSettlements.Find(Data->GoverningFaction);
        if (FactionList) {
            FactionList->Remove(FoundId);
        }

        UE_LOG(LogMythSettlement, Log, TEXT("Unregistered settlement '%s' (ID=%d)"),
               *Data->DisplayName.ToString(), FoundId);
    }

    Settlements.Remove(FoundId);
    SettlementActors.Remove(FoundId);
}

const FMythicSettlementData *UMythicSettlementRegistry::GetSettlementData(int32 SettlementId) const {
    return Settlements.Find(SettlementId);
}

FMythicSettlementData *UMythicSettlementRegistry::GetMutableSettlementData(int32 SettlementId) {
    return Settlements.Find(SettlementId);
}

const FMythicSettlementData *UMythicSettlementRegistry::GetSettlementAtCell(const FMythicCellCoord &Cell) const {
    const int32 *SettlementId = CellToSettlement.Find(Cell);
    if (SettlementId) {
        return Settlements.Find(*SettlementId);
    }
    return nullptr;
}

AMythicSettlement *UMythicSettlementRegistry::GetSettlementActor(int32 SettlementId) const {
    const TWeakObjectPtr<AMythicSettlement> *WeakPtr = SettlementActors.Find(SettlementId);
    if (WeakPtr && WeakPtr->IsValid()) {
        return WeakPtr->Get();
    }
    return nullptr;
}

void UMythicSettlementRegistry::GetSettlementsForFaction(FMythicFactionId Faction, TArray<int32> &OutSettlementIds) const {
    const TArray<int32> *FactionList = FactionSettlements.Find(Faction);
    if (FactionList) {
        OutSettlementIds = *FactionList;
    }
    else {
        OutSettlementIds.Reset();
    }
}

void UMythicSettlementRegistry::GetAllSettlementIds(TArray<int32> &OutIds) const {
    Settlements.GetKeys(OutIds);
}

void UMythicSettlementRegistry::HandleNPCDeath(
    const FMythicEntityId &DeadEntityId, double DeathTime) {
    if (!DeadEntityId.IsValid()) {
        return;
    }

    for (auto &Pair : Settlements) {
        FMythicSettlementData &Data = Pair.Value;

        for (FMythicShopSlot &Shop : Data.Shops) {
            if (Shop.OwnerEntityId == DeadEntityId && !Shop.bPlayerOwned) {
                Shop.OwnerEntityId.Reset();
                Shop.VacatedTime = DeathTime;

                UE_LOG(LogMythSettlement, Log,
                       TEXT("Shop '%s' in '%s' vacated due to %s death."),
                       *Shop.ShopName, *Data.DisplayName.ToString(),
                       *DeadEntityId.ToDebugString());
            }
        }
    }
}

void UMythicSettlementRegistry::TickShopSuccession(double CurrentWorldTime, double SuccessionDelay) {
    for (auto &Pair : Settlements) {
        FMythicSettlementData &Data = Pair.Value;

        for (FMythicShopSlot &Shop : Data.Shops) {
            if (!Shop.OwnerEntityId.IsValid() && !Shop.bPlayerOwned && Shop.VacatedTime > 0.0) {
                if (CurrentWorldTime - Shop.VacatedTime >= SuccessionDelay) {
                    Shop.VacatedTime = 0.0;

                    UE_LOG(LogMythSettlement, Log, TEXT("Shop '%s' in '%s' ready for succession (delay %.1fs elapsed)."),
                           *Shop.ShopName, *Data.DisplayName.ToString(), SuccessionDelay);
                }
            }
        }
    }
}

bool UMythicSettlementRegistry::CanClaimShop(const FMythicShopSlot &Shop, const FGameplayTag &ClaimantRole) {
    if (Shop.OwnerEntityId.IsValid() || Shop.bPlayerOwned || Shop.VacatedTime != 0.0) {
        return false;
    }
    return Shop.RequiredRole.IsValid() && ClaimantRole.IsValid() && ClaimantRole.MatchesTag(Shop.RequiredRole);
}

int32 UMythicSettlementRegistry::ClaimVacantShop(
    int32 SettlementId, const FMythicEntityId &ClaimantEntityId,
    const FGameplayTag &ClaimantRole) {
    if (!ClaimantEntityId.IsValid()
        || ClaimantEntityId.GetDomain()
               != EMythicEntityDomain::LivingWorld) {
        return INDEX_NONE;
    }
    FMythicSettlementData *Data = Settlements.Find(SettlementId);
    if (!Data) {
        return INDEX_NONE;
    }
    for (int32 i = 0; i < Data->Shops.Num(); ++i) {
        FMythicShopSlot &Shop = Data->Shops[i];
        if (CanClaimShop(Shop, ClaimantRole)) {
            Shop.OwnerEntityId = ClaimantEntityId;
            UE_LOG(LogMythSettlement, Log,
                   TEXT("Shop '%s' in '%s' claimed by %s (role %s)."),
                   *Shop.ShopName, *Data->DisplayName.ToString(),
                   *ClaimantEntityId.ToDebugString(), *ClaimantRole.ToString());
            return i;
        }
    }
    return INDEX_NONE;
}

bool UMythicSettlementRegistry::ReferencesEntityIdentity(
    const FMythicEntityId &EntityId) const {
    if (!EntityId.IsValid()) {
        return false;
    }
    for (const TPair<int32, FMythicSettlementData> &Pair : Settlements) {
        for (const FMythicShopSlot &Shop : Pair.Value.Shops) {
            if (!Shop.bPlayerOwned && Shop.OwnerEntityId == EntityId) {
                return true;
            }
        }
    }
    return false;
}

int32 UMythicSettlementRegistry::ClearUnrestorableShopOwners(
    const TSet<FMythicEntityId> &RestorableEntityIds) {
    int32 ClearedCount = 0;
    for (TPair<int32, FMythicSettlementData> &Pair : Settlements) {
        FMythicSettlementData &Settlement = Pair.Value;
        for (FMythicShopSlot &Shop : Settlement.Shops) {
            if (Shop.bPlayerOwned || !Shop.OwnerEntityId.IsValid()
                || RestorableEntityIds.Contains(Shop.OwnerEntityId)) {
                continue;
            }

            UE_LOG(LogMythSettlement, Warning,
                   TEXT("Shop '%s' in '%s': cleared unrestorable owner %s during LivingWorld restore."),
                   *Shop.ShopName, *Settlement.DisplayName.ToString(),
                   *Shop.OwnerEntityId.ToDebugString());
            Shop.OwnerEntityId.Reset();
            Shop.VacatedTime = 0.0;
            ++ClearedCount;
        }
    }
    return ClearedCount;
}

bool UMythicSettlementRegistry::ClearUnrestorableShopOwner(
    const FMythicEntityId &EntityId) {
    if (!EntityId.IsValid()) {
        return false;
    }

    bool bCleared = false;
    for (TPair<int32, FMythicSettlementData> &Pair : Settlements) {
        FMythicSettlementData &Settlement = Pair.Value;
        for (FMythicShopSlot &Shop : Settlement.Shops) {
            if (!Shop.bPlayerOwned && Shop.OwnerEntityId == EntityId) {
                UE_LOG(LogMythSettlement, Warning,
                       TEXT("Shop '%s' in '%s': cleared owner %s after logical rehydration failed."),
                       *Shop.ShopName, *Settlement.DisplayName.ToString(),
                       *EntityId.ToDebugString());
                Shop.OwnerEntityId.Reset();
                Shop.VacatedTime = 0.0;
                bCleared = true;
            }
        }
    }
    return bCleared;
}

void UMythicSettlementRegistry::TransferSettlement(
    int32 SettlementId,
    FMythicFactionId NewFaction,
    UMythicTerritoryGrid *TerritoryGrid,
    UMythicFactionDatabase *FactionDB,
    UMythicCausalFabric *CausalFabric) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicSettlementRegistry_TransferSettlement);

    FMythicSettlementData *Data = Settlements.Find(SettlementId);
    if (!Data) {
        UE_LOG(LogMythSettlement, Warning, TEXT("TransferSettlement: settlement ID %d not found."), SettlementId);
        return;
    }

    const FMythicFactionId OldFaction = Data->GoverningFaction;
    if (OldFaction == NewFaction) {
        return;
    }

    TArray<int32> *OldFactionList = FactionSettlements.Find(OldFaction);
    if (OldFactionList) {
        OldFactionList->Remove(SettlementId);
    }
    FactionSettlements.FindOrAdd(NewFaction).Add(SettlementId);

    Data->GoverningFaction = NewFaction;

    if (TerritoryGrid) {
        for (const FMythicCellCoord &Cell : Data->RasterizedCells) {
            TerritoryGrid->SetCellInfluence(Cell, NewFaction, 1.0f);
        }
    }

    if (FactionDB) {
        const int32 CellDelta = Data->RasterizedCells.Num();

        FMythicFactionData *OldFactionData = FactionDB->GetFactionMutable(OldFaction);
        if (OldFactionData) {
            OldFactionData->ControlledCellCount = FMath::Max(0, OldFactionData->ControlledCellCount - CellDelta);
        }

        FMythicFactionData *NewFactionData = FactionDB->GetFactionMutable(NewFaction);
        if (NewFactionData) {
            NewFactionData->ControlledCellCount += CellDelta;
        }
    }

    if (AMythicSettlement *Actor = GetSettlementActor(SettlementId)) {
        Actor->TransferToFaction(NewFaction);
    }

    if (CausalFabric && Data->RasterizedCells.Num() > 0) {
        FMythicWorldEvent Event;
        Event.EventTag = TAG_LIVINGWORLD_EVENT_TERRITORY_SETTLEMENT_TRANSFER;
        Event.PrimaryFaction = NewFaction;
        Event.SecondaryFaction = OldFaction;
        Event.Cell = Data->RasterizedCells[0];
        Event.Significance = Data->bIsCapital ? 1.0f : 0.7f;
        Event.CategoryFlags = EMythicEventCategory::Territory;
        CausalFabric->AppendEvent(Event);
    }

    UE_LOG(LogMythSettlement, Log, TEXT("Settlement '%s' (ID=%d) transferred: faction %d → %d (%d cells)"),
           *Data->DisplayName.ToString(), SettlementId, OldFaction.Index, NewFaction.Index, Data->RasterizedCells.Num());
}

void UMythicSettlementRegistry::TickConquest(UMythicTerritoryGrid *TerritoryGrid, UMythicFactionDatabase *FactionDB,
                                             UMythicCausalFabric *CausalFabric, float ConquestThreshold) {
    if (!TerritoryGrid) {
        return;
    }

    TArray<TPair<int32, FMythicFactionId>> Conquests;
    for (const TPair<int32, FMythicSettlementData> &Pair : Settlements) {
        const FMythicSettlementData &Data = Pair.Value;
        const int32 CellCount = Data.RasterizedCells.Num();
        if (CellCount == 0) {
            continue;
        }

        TMap<FMythicFactionId, int32> FactionCellCounts;
        for (const FMythicCellCoord &Cell : Data.RasterizedCells) {
            const FMythicFactionId Dom = TerritoryGrid->GetDominantFaction(Cell);
            if (Dom.IsValid()) {
                FactionCellCounts.FindOrAdd(Dom)++;
            }
        }

        FMythicFactionId TopFaction;
        int32 TopCount = 0;
        for (const TPair<FMythicFactionId, int32> &FC : FactionCellCounts) {
            if (FC.Value > TopCount) {
                TopCount = FC.Value;
                TopFaction = FC.Key;
            }
        }

        if (TopFaction.IsValid() && TopFaction != Data.GoverningFaction
            && static_cast<float>(TopCount) / static_cast<float>(CellCount) > ConquestThreshold) {
            Conquests.Add(TPair<int32, FMythicFactionId>(Pair.Key, TopFaction));
        }
    }

    for (const TPair<int32, FMythicFactionId> &C : Conquests) {
        TransferSettlement(C.Key, C.Value, TerritoryGrid, FactionDB, CausalFabric);
    }
}

void UMythicSettlementRegistry::SeedTerritoryFromSettlements(UMythicTerritoryGrid *TerritoryGrid, UMythicFactionDatabase *FactionDB) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicSettlementRegistry_SeedTerritory);

    if (!TerritoryGrid || !FactionDB) {
        UE_LOG(LogMythSettlement, Error, TEXT("SeedTerritory: missing TerritoryGrid or FactionDB."));
        return;
    }

    TMap<FMythicFactionId, int32> FactionCellCounts;
    int32 TotalCellsSeeded = 0;

    for (const auto &Pair : Settlements) {
        const FMythicSettlementData &Data = Pair.Value;

        if (!Data.GoverningFaction.IsValid()) {
            continue;
        }

        for (const FMythicCellCoord &Cell : Data.RasterizedCells) {
            TerritoryGrid->SetCellInfluence(Cell, Data.GoverningFaction, 1.0f);
            FactionCellCounts.FindOrAdd(Data.GoverningFaction)++;
            ++TotalCellsSeeded;
        }
    }

    for (const auto &Pair : FactionCellCounts) {
        FMythicFactionData *FactionData = FactionDB->GetFactionMutable(Pair.Key);
        if (FactionData) {
            FactionData->ControlledCellCount = Pair.Value;
        }
    }

    UE_LOG(LogMythSettlement, Log, TEXT("Seeded %d settlements across %d factions (%d total cells)"),
           Settlements.Num(), FactionCellCounts.Num(), TotalCellsSeeded);
}

void UMythicSettlementRegistry::Serialize(FArchive &Ar) {
    int32 Version = 2;
    Ar << Version;

    if (Ar.IsSaving()) {
        int32 Count = Settlements.Num();
        Ar << Count;
        for (TPair<int32, FMythicSettlementData> &Pair : Settlements) {
            FName Tag = Pair.Value.SettlementTag.GetTagName();
            uint8 GovFaction = Pair.Value.GoverningFaction.Index;
            Ar << Tag;
            Ar << GovFaction;
        }
    }
    else {
        LoadedGoverningFactionOverrides.Reset();
        int32 Count = 0;
        Ar << Count;
        if (Count < 0 || Count > 1000000) {
            Ar.SetError();
            return;
        }
        for (int32 i = 0; i < Count; ++i) {
            FName Tag;
            uint8 GovFaction = 0;
            Ar << Tag;
            Ar << GovFaction;
            if (!Tag.IsNone()) {
                LoadedGoverningFactionOverrides.Add(Tag, GovFaction);
            }
        }

        bool bPatchedAny = false;
        for (TPair<int32, FMythicSettlementData> &Pair : Settlements) {
            const FName Key = Pair.Value.SettlementTag.GetTagName();
            if (const uint8 *SavedFaction = LoadedGoverningFactionOverrides.Find(Key)) {
                Pair.Value.GoverningFaction.Index = *SavedFaction;
                LoadedGoverningFactionOverrides.Remove(Key);
                bPatchedAny = true;
            }
        }
        if (bPatchedAny) {
            RebuildIndices();
        }
    }
}

void UMythicSettlementRegistry::RebuildIndices() {
    CellToSettlement.Reset();
    FactionSettlements.Reset();

    TArray<int32> OrderedIds;
    Settlements.GetKeys(OrderedIds);
    OrderedIds.Sort();

    for (const int32 SettlementId : OrderedIds) {
        const FMythicSettlementData &Data = Settlements[SettlementId];

        for (const FMythicCellCoord &Cell : Data.RasterizedCells) {
            if (!CellToSettlement.Contains(Cell)) {
                CellToSettlement.Add(Cell, SettlementId);
            }
        }

        FactionSettlements.FindOrAdd(Data.GoverningFaction).Add(SettlementId);
    }
}

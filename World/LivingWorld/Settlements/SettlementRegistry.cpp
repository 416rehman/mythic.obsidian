// Mythic Living World — Settlement Registry Implementation

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

    Settlements.Add(AssignedId, MoveTemp(Data));
    SettlementActors.Add(AssignedId, Settlement);

    // Update indices
    const FMythicSettlementData &StoredData = Settlements[AssignedId];
    for (const FMythicCellCoord &Cell : StoredData.RasterizedCells) {
        CellToSettlement.Add(Cell, AssignedId);
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

    // Find the settlement ID for this actor
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
        // Remove cell indices
        for (const FMythicCellCoord &Cell : Data->RasterizedCells) {
            CellToSettlement.Remove(Cell);
        }

        // Remove from faction list
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

    // Update faction settlement lists
    TArray<int32> *OldFactionList = FactionSettlements.Find(OldFaction);
    if (OldFactionList) {
        OldFactionList->Remove(SettlementId);
    }
    FactionSettlements.FindOrAdd(NewFaction).Add(SettlementId);

    // Update settlement data
    Data->GoverningFaction = NewFaction;

    // Re-seed territory cells to new faction
    if (TerritoryGrid) {
        for (const FMythicCellCoord &Cell : Data->RasterizedCells) {
            TerritoryGrid->SetCellInfluence(Cell, NewFaction, 1.0f);
        }
    }

    // Update faction ControlledCellCount
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

    // Update settlement actor if it still exists
    if (AMythicSettlement *Actor = GetSettlementActor(SettlementId)) {
        Actor->TransferToFaction(NewFaction);
    }

    // Log a causal fabric event
    if (CausalFabric && Data->RasterizedCells.Num() > 0) {
        FMythicWorldEvent Event;
        Event.EventTag = TAG_EVENT_TERRITORY_SETTLEMENT_TRANSFER;
        Event.PrimaryFaction = NewFaction;
        Event.SecondaryFaction = OldFaction;
        Event.Cell = Data->RasterizedCells[0]; // Settlement center (first cell is a reasonable pick)
        Event.Significance = Data->bIsCapital ? 1.0f : 0.7f;
        Event.CategoryFlags = EMythicEventCategory::Territory;
        CausalFabric->AppendEvent(Event);
    }

    UE_LOG(LogMythSettlement, Log, TEXT("Settlement '%s' (ID=%d) transferred: faction %d → %d (%d cells)"),
           *Data->DisplayName.ToString(), SettlementId, OldFaction.Index, NewFaction.Index, Data->RasterizedCells.Num());
}

void UMythicSettlementRegistry::SeedTerritoryFromSettlements(UMythicTerritoryGrid *TerritoryGrid, UMythicFactionDatabase *FactionDB) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicSettlementRegistry_SeedTerritory);

    if (!TerritoryGrid || !FactionDB) {
        UE_LOG(LogMythSettlement, Error, TEXT("SeedTerritory: missing TerritoryGrid or FactionDB."));
        return;
    }

    // Track cells seeded per faction for ControlledCellCount update
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

    // Update faction ControlledCellCount from seeding results
    for (const auto &Pair : FactionCellCounts) {
        FMythicFactionData *FactionData = FactionDB->GetFactionMutable(Pair.Key);
        if (FactionData) {
            FactionData->ControlledCellCount = Pair.Value;
        }
    }

    UE_LOG(LogMythSettlement, Log, TEXT("Seeded %d settlements across %d factions (%d total cells)"),
           Settlements.Num(), FactionCellCounts.Num(), TotalCellsSeeded);
}

void UMythicSettlementRegistry::RebuildIndices() {
    CellToSettlement.Reset();
    FactionSettlements.Reset();

    for (const auto &Pair : Settlements) {
        const int32 SettlementId = Pair.Key;
        const FMythicSettlementData &Data = Pair.Value;

        for (const FMythicCellCoord &Cell : Data.RasterizedCells) {
            CellToSettlement.Add(Cell, SettlementId);
        }

        FactionSettlements.FindOrAdd(Data.GoverningFaction).Add(SettlementId);
    }
}

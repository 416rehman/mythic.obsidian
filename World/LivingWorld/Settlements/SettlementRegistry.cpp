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

    // Restore a persisted governance override (territory conquest survives reload), keyed by the stable designer-set
    // SettlementTag. One-shot — consumed so a later re-registration can't revert a post-load conquest. Applied BEFORE
    // the faction index + territory seed below read GoverningFaction. An UNSET SettlementTag can't persist its
    // governance (and collapses with any other untagged settlement on save) — warn so it's caught in content.
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

    // Update indices
    const FMythicSettlementData &StoredData = Settlements[AssignedId];
    for (const FMythicCellCoord &Cell : StoredData.RasterizedCells) {
        // First-claim wins: if two settlements' rasterized cells OVERLAP, don't clobber the existing owner (TMap::Add
        // overwrites). Combined with the ownership-guarded Remove in UnregisterSettlement, this gives each shared cell a
        // stable, deterministic owner instead of last-writer-wins + survivor-wipe corruption of the reverse lookup.
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
        // Remove cell indices — but ONLY the cells THIS settlement actually owns. With overlapping settlements an
        // unconditional Remove would delete a still-valid survivor's mapping (the cell still lies inside another
        // settlement), leaving GetSettlementAtCell returning null for a cell that is genuinely covered.
        for (const FMythicCellCoord &Cell : Data->RasterizedCells) {
            if (CellToSettlement.FindRef(Cell) == FoundId) {
                CellToSettlement.Remove(Cell);
            }
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

void UMythicSettlementRegistry::HandleNPCDeath(uint32 DeadEntityId, double DeathTime) {
    if (DeadEntityId == 0) {
        return;
    }

    for (auto &Pair : Settlements) {
        FMythicSettlementData &Data = Pair.Value;

        for (FMythicShopSlot &Shop : Data.Shops) {
            if (Shop.OwnerEntityId == DeadEntityId && !Shop.bPlayerOwned) {
                // Vacate the shop
                Shop.OwnerEntityId = 0;
                Shop.VacatedTime = DeathTime;

                UE_LOG(LogMythSettlement, Log, TEXT("Shop '%s' in '%s' vacated due to NPC %u death."),
                       *Shop.ShopName, *Data.DisplayName.ToString(), DeadEntityId);
            }
        }
    }
}

void UMythicSettlementRegistry::TickShopSuccession(double CurrentWorldTime, double SuccessionDelay) {
    for (auto &Pair : Settlements) {
        FMythicSettlementData &Data = Pair.Value;

        for (FMythicShopSlot &Shop : Data.Shops) {
            // Check if shop is vacant, not player-owned, and succession timer has elapsed
            if (Shop.OwnerEntityId == 0 && !Shop.bPlayerOwned && Shop.VacatedTime > 0.0) {
                if (CurrentWorldTime - Shop.VacatedTime >= SuccessionDelay) {

                    // The actual generation of a new NPC is handled by the population spawner,
                    // but we clear the vacated time to signal it's ready for a new owner to claim.
                    // When a new NPC with matching role tag spawns in this cell, they claim the slot.
                    Shop.VacatedTime = 0.0;

                    UE_LOG(LogMythSettlement, Log, TEXT("Shop '%s' in '%s' ready for succession (delay %.1fs elapsed)."),
                           *Shop.ShopName, *Data.DisplayName.ToString(), SuccessionDelay);
                }
            }
        }
    }
}

bool UMythicSettlementRegistry::CanClaimShop(const FMythicShopSlot &Shop, const FGameplayTag &ClaimantRole) {
    // Unowned, not player-held, and not waiting out a succession delay (VacatedTime cleared to 0 == ready / never owned).
    if (Shop.OwnerEntityId != 0 || Shop.bPlayerOwned || Shop.VacatedTime != 0.0) {
        return false;
    }
    // The slot must specify a role and the claimant must satisfy it (its tag is the required role or a more-specific child).
    return Shop.RequiredRole.IsValid() && ClaimantRole.IsValid() && ClaimantRole.MatchesTag(Shop.RequiredRole);
}

int32 UMythicSettlementRegistry::ClaimVacantShop(int32 SettlementId, int32 ClaimantEntityId, const FGameplayTag &ClaimantRole) {
    if (ClaimantEntityId == 0) {
        return INDEX_NONE; // 0 is the "vacant" sentinel — a real claimant must have a non-zero entity id
    }
    FMythicSettlementData *Data = Settlements.Find(SettlementId);
    if (!Data) {
        return INDEX_NONE;
    }
    for (int32 i = 0; i < Data->Shops.Num(); ++i) {
        FMythicShopSlot &Shop = Data->Shops[i];
        if (CanClaimShop(Shop, ClaimantRole)) {
            Shop.OwnerEntityId = ClaimantEntityId;
            UE_LOG(LogMythSettlement, Log, TEXT("Shop '%s' in '%s' claimed by NPC %d (role %s)."),
                   *Shop.ShopName, *Data->DisplayName.ToString(), ClaimantEntityId, *ClaimantRole.ToString());
            return i;
        }
    }
    return INDEX_NONE;
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
        Event.EventTag = TAG_LIVINGWORLD_EVENT_TERRITORY_SETTLEMENT_TRANSFER;
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

void UMythicSettlementRegistry::TickConquest(UMythicTerritoryGrid *TerritoryGrid, UMythicFactionDatabase *FactionDB,
                                             UMythicCausalFabric *CausalFabric, float ConquestThreshold) {
    if (!TerritoryGrid) {
        return;
    }

    // Collect conquests FIRST — TransferSettlement mutates Settlements + the grid, so we must not call it while
    // iterating Settlements + reading GetDominantFaction.
    TArray<TPair<int32, FMythicFactionId>> Conquests;
    for (const TPair<int32, FMythicSettlementData> &Pair : Settlements) {
        const FMythicSettlementData &Data = Pair.Value;
        const int32 CellCount = Data.RasterizedCells.Num();
        if (CellCount == 0) {
            continue;
        }

        // Tally which faction dominates each of this settlement's cells (emergent territory-influence state).
        TMap<FMythicFactionId, int32> FactionCellCounts;
        for (const FMythicCellCoord &Cell : Data.RasterizedCells) {
            const FMythicFactionId Dom = TerritoryGrid->GetDominantFaction(Cell);
            if (Dom.IsValid()) {
                FactionCellCounts.FindOrAdd(Dom)++;
            }
        }

        // The single faction controlling the most cells.
        FMythicFactionId TopFaction;
        int32 TopCount = 0;
        for (const TPair<FMythicFactionId, int32> &FC : FactionCellCounts) {
            if (FC.Value > TopCount) {
                TopCount = FC.Value;
                TopFaction = FC.Key;
            }
        }

        // Conquered if a DIFFERENT valid faction holds a clear majority (> threshold) of the settlement's cells.
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

void UMythicSettlementRegistry::Serialize(FArchive &Ar) {
    int32 Version = 2; // v2: keyed by SettlementTag FName (v1's CenterCell key was always (0,0) — never assigned)
    Ar << Version;

    if (Ar.IsSaving()) {
        // Persist the mutable GoverningFaction of every registered settlement, keyed by the stable SettlementTag.
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
        // Load the overrides; RegisterSettlement applies them one-shot as settlements register (handles the case
        // where settlement actors register AFTER LoadLivingWorld). Settlements already registered are patched here.
        LoadedGoverningFactionOverrides.Reset();
        int32 Count = 0;
        Ar << Count;
        // Bound the stream-controlled Count before the loop: a garbage Count would otherwise spin a multi-billion-
        // iteration loop (reading past the stream + growing the override map) → hang/OOM. 1,000,000 is far above any
        // real settlement count; mirrors the other serialize guards.
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
                LoadedGoverningFactionOverrides.Remove(Key); // consume one-shot
                bPatchedAny = true;
            }
        }
        if (bPatchedAny) {
            RebuildIndices(); // re-derive FactionSettlements after GoverningFaction changes
        }
    }
}

void UMythicSettlementRegistry::RebuildIndices() {
    CellToSettlement.Reset();
    FactionSettlements.Reset();

    // Iterate in SettlementId (registration) order so the CellToSettlement first-claim-wins below resolves a cell
    // contested by two settlements the SAME way RegisterSettlement did at registration (SettlementRegistry.cpp:44).
    // A raw last-writer-wins Add (TMap iteration order is unspecified) would silently flip a contested cell's owner
    // on every rebuild, desyncing CellToSettlement from how the cell was originally claimed.
    TArray<int32> OrderedIds;
    Settlements.GetKeys(OrderedIds);
    OrderedIds.Sort();

    for (const int32 SettlementId : OrderedIds) {
        const FMythicSettlementData &Data = Settlements[SettlementId];

        for (const FMythicCellCoord &Cell : Data.RasterizedCells) {
            if (!CellToSettlement.Contains(Cell)) { // first-claim-wins, matching RegisterSettlement
                CellToSettlement.Add(Cell, SettlementId);
            }
        }

        FactionSettlements.FindOrAdd(Data.GoverningFaction).Add(SettlementId);
    }
}

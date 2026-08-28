#include "SavedInventory.h"

#include "Mythic/Itemization/Inventory/MythicInventoryComponent.h"
#include "Mythic/Itemization/Inventory/MythicItemInstance.h"
#include "Mythic/Mythic.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "UObject/StrongObjectPtr.h"

namespace {
constexpr int32 CurrentInventorySaveFormatVersion = 2;
constexpr int32 MaxInventorySlots = 1024;
constexpr int32 MaxStableContainerIdCharacters = 1024;
constexpr int32 MaxSerializedItemBytes = 16 * 1024 * 1024;
constexpr int64 MaxSerializedInventoryBytes = 64 * 1024 * 1024;

struct FStagedInventorySlotRestore {
    int32 SaveIndex = INDEX_NONE;
    int32 TargetIndex = INDEX_NONE;
    bool bHasItem = false;
    TStrongObjectPtr<UMythicItemInstance> Item;
};

struct FStagedInventoryRestore {
    TWeakObjectPtr<UMythicInventoryComponent> Component;
    TArray<FStagedInventorySlotRestore> Slots;
    int32 MismatchCount = 0;
    int32 RestoredItemCount = 0;
};

bool ValidateInventoryEnvelope(const FSerializedInventoryData &Data) {
    if (Data.SaveFormatVersion != CurrentInventorySaveFormatVersion
        || !Data.SaveGameGuid.IsValid() || Data.StableContainerId.IsEmpty()
        || Data.StableContainerId.Len() > MaxStableContainerIdCharacters
        || Data.Slots.Num() > MaxInventorySlots) {
        return false;
    }

    int64 TotalBytes = 0;
    for (const FSerializedSlotData &Slot : Data.Slots) {
        if (Slot.ItemData.ByteData.Num() > MaxSerializedItemBytes) return false;
        TotalBytes += Slot.ItemData.ByteData.Num();
        if (TotalBytes > MaxSerializedInventoryBytes) return false;

        const bool bHasClass = Slot.ItemData.ItemClass.IsValid();
        const bool bHasPayload = !Slot.ItemData.ByteData.IsEmpty();
        if (bHasClass != bHasPayload || Slot.bHasItem != (bHasClass && bHasPayload)) return false;
    }
    return true;
}

bool StageItem(UMythicInventoryComponent *Component, const int32 SaveIndex,
               const FSerializedSlotData &SavedSlot,
               TStrongObjectPtr<UMythicItemInstance> &OutItem) {
    if (!SavedSlot.bHasItem || SavedSlot.ItemData.ByteData.IsEmpty()
        || SavedSlot.ItemData.ByteData.Num() > MaxSerializedItemBytes
        || !SavedSlot.ItemData.ItemClass.IsValid()) {
        UE_LOG(MythSaveLoad, Error, TEXT("  Slot[%d]: empty or oversized item payload rejected"), SaveIndex);
        return false;
    }

    UClass *ItemClass = SavedSlot.ItemData.ItemClass.ResolveClass();
    if (!ItemClass || !ItemClass->IsChildOf(UMythicItemInstance::StaticClass())
        || ItemClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists)
        || FSoftClassPath(ItemClass).ToString() != SavedSlot.ItemData.ItemClass.ToString()) {
        UE_LOG(MythSaveLoad, Error, TEXT("  Slot[%d]: item class is not preloaded or allowed: %s"),
               SaveIndex, *SavedSlot.ItemData.ItemClass.ToString());
        return false;
    }

    UMythicItemInstance *NewItem = NewObject<UMythicItemInstance>(Component, ItemClass);
    if (!NewItem) return false;
    TStrongObjectPtr<UMythicItemInstance> StagedItem(NewItem);

    FMemoryReader MemReader(SavedSlot.ItemData.ByteData, true);
    MemReader.ArMaxSerializeSize = MaxSerializedItemBytes;
    MemReader.ArIsNetArchive = true;
    FObjectAndNameAsStringProxyArchive Ar(MemReader, false);
    Ar.ArIsSaveGame = true;
    Ar.ArMaxSerializeSize = MaxSerializedItemBytes;
    Ar.ArIsNetArchive = true;
    NewItem->Serialize(Ar);
    if (Ar.IsError() || !MemReader.AtEnd() || !NewItem->GetItemDefinition()
        || !NewItem->GetItemInstanceGuid().IsValid()) {
        UE_LOG(MythSaveLoad, Error,
               TEXT("  Slot[%d]: invalid current item payload (archive=%d, consumed=%lld/%d, definition=%s, guid=%s)"),
               SaveIndex, Ar.IsError() ? 1 : 0, MemReader.Tell(), SavedSlot.ItemData.ByteData.Num(),
               NewItem->GetItemDefinition() ? TEXT("valid") : TEXT("missing"),
               NewItem->GetItemInstanceGuid().IsValid() ? TEXT("valid") : TEXT("missing"));
        return false;
    }

    OutItem = MoveTemp(StagedItem);
    return true;
}

bool BuildStagedInventoryRestore(UMythicInventoryComponent *Component,
                                 const FSerializedInventoryData &InData,
                                 FStagedInventoryRestore &OutState) {
    if (!Component || !ValidateInventoryEnvelope(InData)) return false;

    const TArray<FMythicInventorySlotEntry> &TargetSlots = Component->GetAllSlots();
    TArray<FSoftObjectPath> SavedDefs;
    SavedDefs.Reserve(InData.Slots.Num());
    for (const FSerializedSlotData &Saved : InData.Slots) SavedDefs.Add(Saved.SlotDefinition);

    TArray<FSoftObjectPath> TargetDefs;
    TargetDefs.Reserve(TargetSlots.Num());
    for (const FMythicInventorySlotEntry &Target : TargetSlots) {
        TargetDefs.Add(FSoftObjectPath(Target.SlotDefinition));
    }
    const TArray<int32> Mapping = FSerializedInventoryData::ComputeSlotRestoreMapping(
        SavedDefs, TargetDefs);

    OutState.Component = Component;
    OutState.Slots.Reserve(Mapping.Num());
    for (int32 Index = 0; Index < Mapping.Num(); ++Index) {
        if (Mapping[Index] == INDEX_NONE) {
            ++OutState.MismatchCount;
            UE_LOG(MythSaveLoad, Warning,
                   TEXT("  Slot[%d]: no matching unprocessed slot found for definition %s"),
                   Index, *InData.Slots[Index].SlotDefinition.ToString());
            continue;
        }

        FStagedInventorySlotRestore Row;
        Row.SaveIndex = Index;
        Row.TargetIndex = Mapping[Index];
        Row.bHasItem = InData.Slots[Index].bHasItem;
        if (Row.bHasItem && !StageItem(Component, Index, InData.Slots[Index], Row.Item)) {
            UE_LOG(MythSaveLoad, Error,
                   TEXT("Inventory Deserialize: staging failed at slot %d; entire restore quarantined"),
                   Index);
            return false;
        }
        OutState.RestoredItemCount += Row.bHasItem ? 1 : 0;
        OutState.Slots.Add(MoveTemp(Row));
    }
    return true;
}

bool CommitStagedInventoryRestore(FStagedInventoryRestore &State) {
    UMythicInventoryComponent *Component = State.Component.Get();
    if (!Component) return false;

    struct FPreviousSlotState {
        int32 TargetIndex = INDEX_NONE;
        TStrongObjectPtr<UMythicItemInstance> Item;
    };
    TArray<FPreviousSlotState> PreviousStates;
    PreviousStates.Reserve(State.Slots.Num());
    TArray<FMythicInventorySlotEntry> &TargetSlots = Component->GetAllSlotsMutable();

    bool bCommitted = true;
    for (FStagedInventorySlotRestore &Row : State.Slots) {
        if (!TargetSlots.IsValidIndex(Row.TargetIndex)) {
            bCommitted = false;
            break;
        }
        FMythicInventorySlotEntry &Target = TargetSlots[Row.TargetIndex];
        FPreviousSlotState Previous;
        Previous.TargetIndex = Row.TargetIndex;
        if (Target.SlottedItemInstance) {
            Previous.Item = TStrongObjectPtr<UMythicItemInstance>(Target.SlottedItemInstance);
        }
        PreviousStates.Add(MoveTemp(Previous));

        if (!Component->SetItemInSlotInternal(Row.TargetIndex, nullptr)) {
            bCommitted = false;
            break;
        }
        if (Row.bHasItem
            && (!Row.Item.IsValid()
                || !Component->SetItemInSlotInternal(Row.TargetIndex, Row.Item.Get()))) {
            bCommitted = false;
            break;
        }
    }

    if (!bCommitted) {
        bool bRollbackSucceeded = true;
        for (int32 Index = PreviousStates.Num() - 1; Index >= 0; --Index) {
            FPreviousSlotState &Previous = PreviousStates[Index];
            bRollbackSucceeded &= Component->SetItemInSlotInternal(Previous.TargetIndex, nullptr);
            if (Previous.Item.IsValid()) {
                bRollbackSucceeded &= Component->SetItemInSlotInternal(
                    Previous.TargetIndex, Previous.Item.Get());
            }
        }
        UE_LOG(MythSaveLoad, Error, TEXT("Inventory Deserialize: commit failed; rollback %s"),
               bRollbackSucceeded ? TEXT("succeeded") : TEXT("FAILED"));
        return false;
    }

    UE_LOG(MythSaveLoad, Log,
           TEXT("Inventory Deserialize: transactionally restored %d items, %d slot mismatches"),
           State.RestoredItemCount, State.MismatchCount);
    return true;
}
}

bool FSerializedInventoryData::Serialize(
    UMythicInventoryComponent *Component,
    FSerializedInventoryData &OutData,
    const FMythicInventoryRestoreContext &Context) {
    if (!Component) {
        UE_LOG(MythSaveLoad, Error, TEXT("Inventory Serialize: Component is null"));
        return false;
    }

    const TArray<FMythicInventorySlotEntry> &SourceSlots = Component->GetAllSlots();
    if (SourceSlots.Num() > MaxInventorySlots) {
        UE_LOG(MythSaveLoad, Error, TEXT("Inventory Serialize: %d slots exceeds limit %d"),
               SourceSlots.Num(), MaxInventorySlots);
        return false;
    }

    FMythicInventoryRestoreContext EffectiveContext = Context;
    if (!EffectiveContext.IsValid()) {
        if (Context.SaveGameGuid.IsValid() || !Context.StableContainerId.IsEmpty()) {
            UE_LOG(MythSaveLoad, Error,
                   TEXT("Inventory Serialize: incomplete explicit save identity metadata"));
            return false;
        }
        EffectiveContext.SaveGameGuid = FGuid::NewGuid();
        EffectiveContext.StableContainerId = TEXT("inventory/root");
    }
    if (EffectiveContext.StableContainerId.Len() > MaxStableContainerIdCharacters) {
        UE_LOG(MythSaveLoad, Error, TEXT("Inventory Serialize: stable container identity exceeds limit"));
        return false;
    }

    TArray<FSerializedSlotData> StagedSlots;
    StagedSlots.Reserve(SourceSlots.Num());
    int64 TotalSerializedBytes = 0;
    for (int32 Index = 0; Index < SourceSlots.Num(); ++Index) {
        const FMythicInventorySlotEntry &SlotEntry = SourceSlots[Index];
        FSerializedSlotData SavedSlot;
        if (SlotEntry.SlotDefinition) {
            SavedSlot.SlotDefinition = FSoftObjectPath(SlotEntry.SlotDefinition);
        }
        SavedSlot.bHasItem = SlotEntry.SlottedItemInstance != nullptr;

        if (UMythicItemInstance *Item = SlotEntry.SlottedItemInstance) {
            if (!Item->GetItemInstanceGuid().IsValid()) {
                UE_LOG(MythSaveLoad, Error,
                       TEXT("  Slot[%d]: current-format items require a stable item GUID"), Index);
                return false;
            }
            SavedSlot.ItemData.ItemClass = FSoftClassPath(Item->GetClass());
            FMemoryWriter MemWriter(SavedSlot.ItemData.ByteData);
            FObjectAndNameAsStringProxyArchive Ar(MemWriter, false);
            Ar.ArIsSaveGame = true;
            Item->Serialize(Ar);
            if (Ar.IsError() || SavedSlot.ItemData.ByteData.IsEmpty()
                || SavedSlot.ItemData.ByteData.Num() > MaxSerializedItemBytes) {
                UE_LOG(MythSaveLoad, Error,
                       TEXT("  Slot[%d]: item serialization failed or exceeded %d bytes"),
                       Index, MaxSerializedItemBytes);
                return false;
            }
            TotalSerializedBytes += SavedSlot.ItemData.ByteData.Num();
            if (TotalSerializedBytes > MaxSerializedInventoryBytes) {
                UE_LOG(MythSaveLoad, Error,
                       TEXT("Inventory Serialize: aggregate item payload exceeded %lld bytes"),
                       MaxSerializedInventoryBytes);
                return false;
            }
        }
        StagedSlots.Add(MoveTemp(SavedSlot));
    }

    OutData.SaveFormatVersion = CurrentInventorySaveFormatVersion;
    OutData.SaveGameGuid = EffectiveContext.SaveGameGuid;
    OutData.StableContainerId = EffectiveContext.StableContainerId;
    OutData.Slots = MoveTemp(StagedSlots);
    return true;
}

TArray<int32> FSerializedInventoryData::ComputeSlotRestoreMapping(
    const TArray<FSoftObjectPath> &SavedSlotDefs,
    const TArray<FSoftObjectPath> &TargetSlotDefs) {
    TArray<int32> SaveToTarget;
    SaveToTarget.Init(INDEX_NONE, SavedSlotDefs.Num());
    TArray<bool> TargetClaimed;
    TargetClaimed.Init(false, TargetSlotDefs.Num());

    for (int32 Index = 0; Index < SavedSlotDefs.Num(); ++Index) {
        if (TargetSlotDefs.IsValidIndex(Index) && TargetSlotDefs[Index] == SavedSlotDefs[Index]) {
            SaveToTarget[Index] = Index;
            TargetClaimed[Index] = true;
        }
    }
    for (int32 Index = 0; Index < SavedSlotDefs.Num(); ++Index) {
        if (SaveToTarget[Index] != INDEX_NONE) continue;
        for (int32 TargetIndex = 0; TargetIndex < TargetSlotDefs.Num(); ++TargetIndex) {
            if (!TargetClaimed[TargetIndex]
                && TargetSlotDefs[TargetIndex] == SavedSlotDefs[Index]) {
                SaveToTarget[Index] = TargetIndex;
                TargetClaimed[TargetIndex] = true;
                break;
            }
        }
    }
    return SaveToTarget;
}

bool FSerializedInventoryData::Deserialize(
    UMythicInventoryComponent *Component,
    const FSerializedInventoryData &InData,
    const FMythicInventoryRestoreContext &Context) {
    if (!Component || !ValidateInventoryEnvelope(InData)) {
        UE_LOG(MythSaveLoad, Error,
               TEXT("Inventory Deserialize: invalid current-format envelope or excessive payload"));
        return false;
    }
    if ((Context.SaveGameGuid.IsValid() || !Context.StableContainerId.IsEmpty())
        && (!Context.IsValid() || Context.SaveGameGuid != InData.SaveGameGuid
            || Context.StableContainerId != InData.StableContainerId)) {
        UE_LOG(MythSaveLoad, Error,
               TEXT("Inventory Deserialize: explicit save identity does not match the payload"));
        return false;
    }

    FStagedInventoryRestore State;
    return BuildStagedInventoryRestore(Component, InData, State)
        && CommitStagedInventoryRestore(State);
}

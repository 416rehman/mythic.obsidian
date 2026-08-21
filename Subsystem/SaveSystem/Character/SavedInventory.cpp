#include "SavedInventory.h"
#include "Mythic/Itemization/Inventory/MythicInventoryComponent.h"
#include "Mythic/Itemization/Inventory/MythicItemInstance.h"
#include "Mythic/Mythic.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

void FSerializedInventoryData::Serialize(UMythicInventoryComponent *Component, FSerializedInventoryData &OutData) {
    if (!Component) {
        UE_LOG(MythSaveLoad, Error, TEXT("Inventory Serialize: Component is null"));
        return;
    }

    OutData.Slots.Empty();
    const TArray<FMythicInventorySlotEntry> &SrcSlots = Component->GetAllSlots();

    UE_LOG(MythSaveLoad, Log, TEXT("Inventory Serialize: Found %d slots"), SrcSlots.Num());

    for (int32 i = 0; i < SrcSlots.Num(); ++i) {
        const FMythicInventorySlotEntry &SlotEntry = SrcSlots[i];
        FSerializedSlotData SavedSlot;

        if (SlotEntry.SlotDefinition) {
            SavedSlot.SlotDefinition = FSoftObjectPath(SlotEntry.SlotDefinition);
        }

        SavedSlot.bIsActive = SlotEntry.bEquipmentSlot;
        SavedSlot.bHasItem = (SlotEntry.SlottedItemInstance != nullptr);

        if (SlotEntry.SlottedItemInstance) {
            UMythicItemInstance *Item = SlotEntry.SlottedItemInstance;
            SavedSlot.ItemData.ItemClass = FSoftClassPath(Item->GetClass());

            FMemoryWriter MemWriter(SavedSlot.ItemData.ByteData);
            FObjectAndNameAsStringProxyArchive Ar(MemWriter, false);
            Ar.ArIsSaveGame = true;
            Item->Serialize(Ar);

            UE_LOG(MythSaveLoad, Log, TEXT("  Slot[%d]: Saved item %s (%s) - %d bytes"),
                   i,
                   *Item->GetItemDefinition()->Name.ToString(),
                   *Item->GetClass()->GetName(),
                   SavedSlot.ItemData.ByteData.Num());
        }
        else {
            UE_LOG(MythSaveLoad, Log, TEXT("  Slot[%d]: Empty"), i);
        }

        OutData.Slots.Add(SavedSlot);
    }

    UE_LOG(MythSaveLoad, Log, TEXT("Inventory Serialize: Saved %d slots"), OutData.Slots.Num());
}

static void ProcessSlot(UMythicInventoryComponent *Component, int32 SaveIndex, int32 TargetIndex, const FSerializedSlotData &SavedSlot, int32 &RestoredCount) {
    TArray<FMythicInventorySlotEntry> &TargetSlots = Component->GetAllSlotsMutable();
    FMythicInventorySlotEntry &FoundSlot = TargetSlots[TargetIndex];

    FoundSlot.bEquipmentSlot = SavedSlot.bIsActive;

    if (SavedSlot.bHasItem) {
        UClass *ItemClass = SavedSlot.ItemData.ItemClass.TryLoadClass<UMythicItemInstance>();
        if (ItemClass) {
            UMythicItemInstance *NewItem = NewObject<UMythicItemInstance>(Component, ItemClass);

            FMemoryReader MemReader(SavedSlot.ItemData.ByteData);
            FObjectAndNameAsStringProxyArchive Ar(MemReader, false);
            Ar.ArIsSaveGame = true;
            NewItem->Serialize(Ar);

            UE_LOG(MythSaveLoad, Log, TEXT("  Slot[%d] -> Target[%d]: Deserialized item. Def: %s, Qty: %d, Level: %d"),
                   SaveIndex, TargetIndex,
                   NewItem->GetItemDefinition() ? *NewItem->GetItemDefinition()->GetName() : TEXT("NULL"),
                   NewItem->GetStacks(),
                   NewItem->GetItemLevel());

            if (FoundSlot.SlottedItemInstance != nullptr) {
                Component->SetItemInSlotInternal(TargetIndex, nullptr);
            }

            if (Component->SetItemInSlotInternal(TargetIndex, NewItem)) {
                RestoredCount++;
                UE_LOG(MythSaveLoad, Log, TEXT("  Slot[%d] -> Target[%d]: Restored item %s (Class: %s)"),
                       SaveIndex, TargetIndex,
                       NewItem->GetItemDefinition() ? *NewItem->GetItemDefinition()->Name.ToString() : TEXT("UNKNOWN"),
                       *ItemClass->GetName());
            }
            else {
                UE_LOG(MythSaveLoad, Error, TEXT("  Slot[%d] -> Target[%d]: Failed to set item in slot"), SaveIndex, TargetIndex);
            }
        }
        else {
            UE_LOG(MythSaveLoad, Error, TEXT("  Slot[%d]: Failed to load class %s"),
                   SaveIndex, *SavedSlot.ItemData.ItemClass.ToString());
        }
    }
    else {
        Component->SetItemInSlotInternal(TargetIndex, nullptr);
        UE_LOG(MythSaveLoad, Log, TEXT("  Slot[%d] -> Target[%d]: Cleared (was empty in save)"), SaveIndex, TargetIndex);
    }
}

TArray<int32> FSerializedInventoryData::ComputeSlotRestoreMapping(const TArray<FSoftObjectPath> &SavedSlotDefs,
                                                                  const TArray<FSoftObjectPath> &TargetSlotDefs) {
    TArray<int32> SaveToTarget;
    SaveToTarget.Init(INDEX_NONE, SavedSlotDefs.Num());

    TArray<bool> TargetClaimed;
    TargetClaimed.Init(false, TargetSlotDefs.Num());

    for (int32 i = 0; i < SavedSlotDefs.Num(); ++i) {
        if (TargetSlotDefs.IsValidIndex(i) && TargetSlotDefs[i] == SavedSlotDefs[i]) {
            SaveToTarget[i] = i;
            TargetClaimed[i] = true;
        }
    }

    for (int32 i = 0; i < SavedSlotDefs.Num(); ++i) {
        if (SaveToTarget[i] != INDEX_NONE) {
            continue;
        }
        for (int32 t = 0; t < TargetSlotDefs.Num(); ++t) {
            if (!TargetClaimed[t] && TargetSlotDefs[t] == SavedSlotDefs[i]) {
                SaveToTarget[i] = t;
                TargetClaimed[t] = true;
                break;
            }
        }
    }

    return SaveToTarget;
}

void FSerializedInventoryData::Deserialize(UMythicInventoryComponent *Component, const FSerializedInventoryData &InData) {
    if (!Component) {
        UE_LOG(MythSaveLoad, Error, TEXT("Inventory Deserialize: Component is null"));
        return;
    }

    TArray<FMythicInventorySlotEntry> &TargetSlots = Component->GetAllSlotsMutable();

    UE_LOG(MythSaveLoad, Log, TEXT("Inventory Deserialize: Loading %d saved slots into %d target slots"),
           InData.Slots.Num(), TargetSlots.Num());

    TArray<FSoftObjectPath> SavedDefs;
    SavedDefs.Reserve(InData.Slots.Num());
    for (const FSerializedSlotData &Saved : InData.Slots) {
        SavedDefs.Add(Saved.SlotDefinition);
    }
    TArray<FSoftObjectPath> TargetDefs;
    TargetDefs.Reserve(TargetSlots.Num());
    for (const FMythicInventorySlotEntry &Target : TargetSlots) {
        TargetDefs.Add(FSoftObjectPath(Target.SlotDefinition));
    }
    const TArray<int32> Mapping = FSerializedInventoryData::ComputeSlotRestoreMapping(SavedDefs, TargetDefs);

    int32 RestoredCount = 0;
    int32 MismatchCount = 0;

    for (int32 i = 0; i < Mapping.Num(); ++i) {
        if (Mapping[i] == i) {
            ProcessSlot(Component, i, i, InData.Slots[i], RestoredCount);
        }
    }
    for (int32 i = 0; i < Mapping.Num(); ++i) {
        if (Mapping[i] != INDEX_NONE && Mapping[i] != i) {
            ProcessSlot(Component, i, Mapping[i], InData.Slots[i], RestoredCount);
        }
    }
    for (int32 i = 0; i < Mapping.Num(); ++i) {
        if (Mapping[i] == INDEX_NONE) {
            MismatchCount++;
            UE_LOG(MythSaveLoad, Warning, TEXT("  Slot[%d]: No matching unprocessed slot found for def %s"),
                   i, *InData.Slots[i].SlotDefinition.ToString());
        }
    }

    UE_LOG(MythSaveLoad, Log, TEXT("Inventory Deserialize: Restored %d items, %d slot mismatches"),
           RestoredCount, MismatchCount);
}

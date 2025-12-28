#include "SavedInventory.h"
#include "Mythic/Itemization/Inventory/MythicInventoryComponent.h"
#include "Mythic/Itemization/Inventory/MythicItemInstance.h"
#include "Mythic/Mythic.h"
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

        SavedSlot.bIsActive = SlotEntry.bIsActive;
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

    // Restore active state
    FoundSlot.bIsActive = SavedSlot.bIsActive;

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

            // Use the component's internal API to set the item
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
        // Clear the slot using the component's internal API
        Component->SetItemInSlotInternal(TargetIndex, nullptr);
        UE_LOG(MythSaveLoad, Log, TEXT("  Slot[%d] -> Target[%d]: Cleared (was empty in save)"), SaveIndex, TargetIndex);
    }
}

void FSerializedInventoryData::Deserialize(UMythicInventoryComponent *Component, const FSerializedInventoryData &InData) {
    if (!Component) {
        UE_LOG(MythSaveLoad, Error, TEXT("Inventory Deserialize: Component is null"));
        return;
    }

    TArray<FMythicInventorySlotEntry> &TargetSlots = Component->GetAllSlotsMutable();

    UE_LOG(MythSaveLoad, Log, TEXT("Inventory Deserialize: Loading %d saved slots into %d target slots"),
           InData.Slots.Num(), TargetSlots.Num());

    int32 RestoredCount = 0;
    int32 MismatchCount = 0;

    // Track which target slots have been processed to avoid multiple save slots mapping to the same target slot
    TArray<bool> TargetProcessed;
    TargetProcessed.SetNumZeroed(TargetSlots.Num());

    // Step 1: Try to match by index first for stability
    TArray<int32> SaveSlotsToProcess;
    for (int32 i = 0; i < InData.Slots.Num(); ++i) {
        bool bMatched = false;
        if (TargetSlots.IsValidIndex(i)) {
            if (FSoftObjectPath(TargetSlots[i].SlotDefinition) == InData.Slots[i].SlotDefinition) {
                // Perfect match by index and definition
                ProcessSlot(Component, i, i, InData.Slots[i], RestoredCount);
                TargetProcessed[i] = true;
                bMatched = true;
            }
        }

        if (!bMatched) {
            SaveSlotsToProcess.Add(i);
        }
    }

    // Step 2: For slots that didn't match by index, find the first available slot with the same definition
    for (int32 i : SaveSlotsToProcess) {
        const FSerializedSlotData &SavedSlot = InData.Slots[i];
        FSoftObjectPath DefPath = SavedSlot.SlotDefinition;

        int32 FoundIndex = INDEX_NONE;
        for (int32 SlotIdx = 0; SlotIdx < TargetSlots.Num(); ++SlotIdx) {
            if (!TargetProcessed[SlotIdx] && FSoftObjectPath(TargetSlots[SlotIdx].SlotDefinition) == DefPath) {
                FoundIndex = SlotIdx;
                break;
            }
        }

        if (FoundIndex != INDEX_NONE) {
            ProcessSlot(Component, i, FoundIndex, SavedSlot, RestoredCount);
            TargetProcessed[FoundIndex] = true;
        }
        else {
            MismatchCount++;
            UE_LOG(MythSaveLoad, Warning, TEXT("  Slot[%d]: No matching unprocessed slot found for def %s"),
                   i, *DefPath.ToString());
        }
    }

    UE_LOG(MythSaveLoad, Log, TEXT("Inventory Deserialize: Restored %d items, %d slot mismatches"),
           RestoredCount, MismatchCount);
}

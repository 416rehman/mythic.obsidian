#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPath.h"
#include "SavedInventory.generated.h"

class UMythicInventoryComponent;
class UMythicSaveGameSubsystem;

USTRUCT(BlueprintType)
struct FSerializedItemData {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FSoftClassPath ItemClass;

    UPROPERTY(BlueprintReadWrite)
    TArray<uint8> ByteData;
};

USTRUCT(BlueprintType)
struct FSerializedSlotData {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FSoftObjectPath SlotDefinition;

    UPROPERTY(BlueprintReadWrite)
    bool bIsActive = false;

    UPROPERTY(BlueprintReadWrite)
    FSerializedItemData ItemData;

    UPROPERTY(BlueprintReadWrite)
    bool bHasItem = false;
};

USTRUCT(BlueprintType)
struct FSerializedInventoryData {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    TArray<FSerializedSlotData> Slots;

    // Serialize from component
    static void Serialize(UMythicInventoryComponent *Component, FSerializedInventoryData &OutData);

    // Deserialize to component
    static void Deserialize(UMythicInventoryComponent *Component, const FSerializedInventoryData &InData);

    /**
     * Maps each saved slot (by SlotDefinition path) to a target slot index for restore, or INDEX_NONE if no target
     * matches (e.g. the equipment-slot layout changed between save versions). Two passes: (1) index-stable — saved
     * slot i keeps index i if the target at i has the same definition; (2) fallback — the first not-yet-claimed target
     * with the same definition. A target is claimed by at most one saved slot. Pure (operates on definition paths only)
     * so the restore-matching contract is unit-testable without a live inventory component.
     * Returns SaveToTarget where SaveToTarget[savedIdx] is the chosen target index (or INDEX_NONE).
     */
    static TArray<int32> ComputeSlotRestoreMapping(const TArray<FSoftObjectPath> &SavedSlotDefs, const TArray<FSoftObjectPath> &TargetSlotDefs);
};

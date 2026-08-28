#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPath.h"
#include "SavedInventory.generated.h"

class UMythicInventoryComponent;
class UMythicSaveGameSubsystem;

/** Explicit identity metadata supplied by the owning save/container schema. */
struct MYTHIC_API FMythicInventoryRestoreContext {
    FGuid SaveGameGuid;
    FString StableContainerId;

    bool IsValid() const { return SaveGameGuid.IsValid() && !StableContainerId.IsEmpty(); }
};

USTRUCT(BlueprintType)
struct FSerializedItemData {
    GENERATED_BODY()

    /** Class identity needed to reconstruct the serialized item instance. */
    UPROPERTY(BlueprintReadWrite)
    FSoftClassPath ItemClass;

    /** Bounded framed item payload produced by the item's save serializer. */
    UPROPERTY(BlueprintReadWrite)
    TArray<uint8> ByteData;
};

USTRUCT(BlueprintType)
struct FSerializedSlotData {
    GENERATED_BODY()

    /** Stable slot-definition identity used to remap saved slots when layouts change. */
    UPROPERTY(BlueprintReadWrite)
    FSoftObjectPath SlotDefinition;

    /** Serialized item payload stored in this slot when Has Item is true. */
    UPROPERTY(BlueprintReadWrite)
    FSerializedItemData ItemData;

    /** Whether this slot contains an item payload. */
    UPROPERTY(BlueprintReadWrite)
    bool bHasItem = false;
};

USTRUCT(BlueprintType)
struct FSerializedInventoryData {
    GENERATED_BODY()

    /** Exact current save schema version; unsupported layouts are rejected rather than converted at runtime. */
    UPROPERTY(BlueprintReadWrite)
    int32 SaveFormatVersion = 0;

    /** Save-wide stable identity used to prevent inventory payloads from being replayed across saves. */
    UPROPERTY(BlueprintReadWrite)
    FGuid SaveGameGuid;

    /** Stable inventory/container identity used to bind this payload to its owning container. */
    UPROPERTY(BlueprintReadWrite)
    FString StableContainerId;

    /** Serialized inventory slots in their saved layout order. */
    UPROPERTY(BlueprintReadWrite)
    TArray<FSerializedSlotData> Slots;

    static bool Serialize(UMythicInventoryComponent *Component, FSerializedInventoryData &OutData,
                          const FMythicInventoryRestoreContext &Context = FMythicInventoryRestoreContext());

    static bool Deserialize(UMythicInventoryComponent *Component, const FSerializedInventoryData &InData,
                            const FMythicInventoryRestoreContext &Context = FMythicInventoryRestoreContext());

    static TArray<int32> ComputeSlotRestoreMapping(const TArray<FSoftObjectPath> &SavedSlotDefs, const TArray<FSoftObjectPath> &TargetSlotDefs);
};

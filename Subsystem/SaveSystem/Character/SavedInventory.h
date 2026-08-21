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

    static void Serialize(UMythicInventoryComponent *Component, FSerializedInventoryData &OutData);

    static void Deserialize(UMythicInventoryComponent *Component, const FSerializedInventoryData &InData);

    static TArray<int32> ComputeSlotRestoreMapping(const TArray<FSoftObjectPath> &SavedSlotDefs, const TArray<FSoftObjectPath> &TargetSlotDefs);
};

#pragma once

#include "CoreMinimal.h"
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
};

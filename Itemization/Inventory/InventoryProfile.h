#pragma once

#include "CoreMinimal.h"
#include "InventorySlotDefinition.h"
#include "Itemization/MythicDataAsset.h"
#include "InventoryProfile.generated.h"

USTRUCT(BlueprintType)
struct FInventoryProfileEntry {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
    TObjectPtr<UInventorySlotDefinition> SlotDefinition = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot", meta = (ClampMin = "1", UIMin = "1"))
    int32 Count = 1;

    // If true, activates items when placed in this slot (e.g. equipment slots).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
    bool bIsActiveSlot = false;
};

/**
 * Defines a collection of slots for an inventory (e.g., Player Inventory, Storage Chest).
 */
UCLASS(BlueprintType, Blueprintable)
class MYTHIC_API UInventoryProfile : public UMythicDataAsset {
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Profile", meta=(TitleProperty="SlotDefinition"))
    TArray<FInventoryProfileEntry> Slots;
};

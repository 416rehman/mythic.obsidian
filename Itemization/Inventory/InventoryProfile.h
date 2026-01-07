#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "InventorySlotDefinition.h"
#include "Itemization/MythicDataAsset.h"
#include "InventoryProfile.generated.h"

/**
 * Entry for a slot within a group - defines what slot type and how many.
 */
USTRUCT(BlueprintType)
struct FInventoryProfileEntry {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
    TObjectPtr<UInventorySlotDefinition> SlotDefinition = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot", meta = (ClampMin = "1", UIMin = "1"))
    int32 Count = 1;

    // If true, items in these slots must be unique (no duplicate item definitions within this entry's slots)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
    bool bRequireUniqueItems = false;
};

/**
 * A group of inventory slots that share a UI tab and behavior settings.
 */
USTRUCT(BlueprintType)
struct FInventorySlotGroup {
    GENERATED_BODY()

    // Display name for this group (used for tab name in UI)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Group")
    FText GroupName;

    // Icon for this group's tab
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Group")
    TObjectPtr<UTexture2D> Icon = nullptr;

    // Sort order for UI tabs (lower = first)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Group")
    int32 DisplayOrder = 0;

    // If true, items in this group are "equipped" - activates items when slotted
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Group")
    bool bIsEquipmentGroup = false;

    // If true, items cannot be dropped, sold, or transferred out by the player
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Group")
    bool bProtectedItems = false;

    // When group is full and item needs to be added, expand by this many slots (0 = no auto-expand)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Group", meta = (ClampMin = "0"))
    int32 AutoExpandStep = 0;

    // Slots in this group
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Group", meta = (TitleProperty = "SlotDefinition"))
    TArray<FInventoryProfileEntry> Slots;
};

/**
 * Defines a collection of slot groups for an inventory (e.g., Player Inventory, Storage Chest).
 */
UCLASS(BlueprintType, Blueprintable)
class MYTHIC_API UInventoryProfile : public UMythicDataAsset {
    GENERATED_BODY()

public:
    // Slot groups mapped by gameplay tag for easy lookup and identification
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Profile", meta = (Categories = "Inventory.Group"))
    TMap<FGameplayTag, FInventorySlotGroup> SlotGroups;
};

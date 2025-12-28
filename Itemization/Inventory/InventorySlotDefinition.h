#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Itemization/MythicTags_Inventory.h" // Assuming this exists based on previous files, or defines standard tags
#include "Itemization/MythicDataAsset.h"
#include "InventorySlotDefinition.generated.h"

/**
 * Defines a type of inventory slot (e.g., Head, Chest, Weapon).
 */
UCLASS(BlueprintType, Blueprintable)
class MYTHIC_API UInventorySlotDefinition : public UMythicDataAsset {
    GENERATED_BODY()

public:
    // Permit types of items that can be placed in this slot. To permit all items, leave this tag container empty.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot", meta = (Categories = "Itemization.Type"))
    FGameplayTagContainer WhitelistedItemTypes;

    // Display Name for this slot (e.g. "Head", "Weapon") - used for Tab Names in UI
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
    FText DisplayName;

    // Icon to display for this slot
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
    TObjectPtr<UTexture2D> Icon = nullptr;
};

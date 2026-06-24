#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Itemization/MythicTags_Inventory.h" // Assuming this exists based on previous files, or defines standard tags
#include "Itemization/MythicDataAsset.h"
#include "InventorySlotDefinition.generated.h"

UENUM(BlueprintType)
enum class EInventorySlotType : uint8 {
    None,
    Helmet,
    Chest,
    Hands,
    Legs,
    Feet,
    MainHand,
    OffHand
};

/**
 * Defines a type of inventory slot (e.g., Head, Chest, Weapon).
 */
UCLASS(BlueprintType, Blueprintable)
class MYTHIC_API UInventorySlotDefinition : public UMythicDataAsset {
    GENERATED_BODY()

public:
    // permit types of items that can be placed in this slot, empty accepts all
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot", meta = (Categories = "Itemization.Type"))
    FGameplayTagContainer WhitelistedItemTypes;

    // display name for this slot used for tab names in ui
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
    FText DisplayName;

    // icon to display for this slot
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
    TObjectPtr<UTexture2D> Icon = nullptr;

    // the specific equipment slot category this represents
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
    EInventorySlotType SlotType = EInventorySlotType::None;
};

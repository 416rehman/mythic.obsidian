#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "InventorySlotDefinition.h"
#include "Itemization/MythicDataAsset.h"
#include "InventoryProfile.generated.h"

/**
 * Gameplay ownership domain for an inventory slot. The domain is authored by the inventory profile and is never
 * persisted per save slot: current cooked profile data is the sole authority for slot behavior.
 */
UENUM(BlueprintType)
enum class EMythicInventorySlotDomain : uint8 {
    /** Carried storage; items are owned but contribute no equipped lifecycle, attack, or equipment-affix state. */
    Carried,

    /**
     * Equipped gear - armor, accessories, the combat weapon, and each harvesting tool in its own slot. Occupying the
     * slot is the whole of being equipped: the item is never wielded or selected, and slot membership alone owns its
     * active lifecycle and equipment-affix state.
     */
    Equipment,
};

USTRUCT(BlueprintType)
struct FInventoryProfileEntry {
    GENERATED_BODY()

    /** Slot definition repeated Count times in this profile entry. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
    TObjectPtr<UInventorySlotDefinition> SlotDefinition = nullptr;

    /** Number of slots instantiated from Slot Definition. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot", meta = (ClampMin = "1", UIMin = "1"))
    int32 Count = 1;

    /** When true, this entry rejects duplicate item definitions across its instantiated slots. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
    bool bRequireUniqueItems = false;
};

USTRUCT(BlueprintType)
struct FInventorySlotGroup {
    GENERATED_BODY()

    /** Localized group name used by inventory tabs and loadout surfaces. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Group")
    FText GroupName;

    /** Group icon used by inventory tabs and loadout surfaces. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Group")
    TObjectPtr<UTexture2D> Icon = nullptr;

    /** Stable UI sort key; lower values are displayed first. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Group")
    int32 DisplayOrder = 0;

    /**
     * Typed gameplay domain shared by every slot in this group. Equipment is active for as long as it occupies the
     * slot; Carried is inactive storage.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Group")
    EMythicInventorySlotDomain SlotDomain = EMythicInventorySlotDomain::Carried;

    /** Whether player-authored transactions may remove items from this group. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Group")
    bool bCanPlayerTake = true;

    /** Whether player-authored transactions may insert items into this group. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Group")
    bool bCanPlayerPut = true;

    /** Number of slots added when this group is full; zero disables automatic expansion. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Group", meta = (ClampMin = "0"))
    int32 AutoExpandStep = 0;

    /** Ordered slot-definition runs instantiated for this group. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Group", meta = (TitleProperty = "SlotDefinition"))
    TArray<FInventoryProfileEntry> Slots;
};

UCLASS(BlueprintType, Blueprintable)
class MYTHIC_API UInventoryProfile : public UMythicDataAsset {
    GENERATED_BODY()

public:
    /** Root tag whose groups own gear behaviour; a group under it must declare the Equipment domain. */
    static const FName EquipmentGroupTagRoot;

    /** Data-driven slot groups keyed by canonical Inventory.Group gameplay tags. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Profile", meta = (Categories = "Inventory.Group"))
    TMap<FGameplayTag, FInventorySlotGroup> SlotGroups;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext &Context) const override;
#endif
};

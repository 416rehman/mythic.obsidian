// 

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Itemization/Inventory/Fragments/ItemFragment.h"
#include "CraftableFragment.generated.h"

USTRUCT(BlueprintType, Blueprintable)
struct FCraftingRequirement {
    GENERATED_BODY()

    // Soft reference to the data asset of the item that is required to craft the item
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TSoftObjectPtr<UItemDefinition> RequiredItem = nullptr;

    // The amount of the item that is required to craft the item
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 RequiredAmount = 2;

    // The amount of the item that will be returned on dismantle
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 DismantleReward = 1;

    // == Operator
    bool operator==(const FCraftingRequirement &Other) const {
        return RequiredItem == Other.RequiredItem && RequiredAmount == Other.RequiredAmount && DismantleReward == Other.DismantleReward;
    }
};

/**
 * This fragment allows this item to be crafted or dismantled at a crafting station
 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class MYTHIC_API UCraftableFragment : public UItemFragment {
    GENERATED_BODY()

public:
    DECLARE_FRAGMENT(Craftable)

    virtual bool CanBeStackedWith(const UItemFragment *Other) const override;

    // The items that are required to craft or dismantle this item
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, meta=(DisplayName="CraftingRequirements", MakeStructureDefaultValue="None"))
    TArray<FCraftingRequirement> CraftingRequirements;

    // The tag that is required to be on the player to craft this item. I.e this tag could be granted by reading a "schematic" item
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, meta=(DisplayName="RequiredTag", Categories="Itemization.Schematic"))
    FGameplayTag RequiredTag;

    // The tag that is required to be on the crafting station to craft this item (e.g., forge, alchemy table)
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, meta=(DisplayName="RequiredStationTag", Categories="Itemization.Station"))
    FGameplayTag RequiredStationTag;

    // The time in seconds it takes to craft this item
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, meta=(ClampMin="0.0", UIMin="0.0"))
    float TimeNeeded = 3.0f;
};

inline bool UCraftableFragment::CanBeStackedWith(const UItemFragment *Other) const {
    auto OtherCraftable = Cast<UCraftableFragment>(Other);
    if (!OtherCraftable) {
        return false;
    }

    return Super::CanBeStackedWith(Other) &&
        this->CraftingRequirements == OtherCraftable->CraftingRequirements &&
        this->RequiredTag == OtherCraftable->RequiredTag;
}

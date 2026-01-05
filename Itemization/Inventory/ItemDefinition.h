#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Itemization/MythicTags_Inventory.h"
#include "Itemization/MythicDataAsset.h"
#include "ItemDefinition.generated.h"

UENUM(BlueprintType, Blueprintable)
enum EItemRarity {
    Common = 0,
    Rare = 1,
    Epic = 2,
    Legendary = 3,
    Mythic = 4,
};

class UItemFragment;

/**
 * Base class for all item definitions.
 * Use the buttons in "Set Category" to auto-configure ItemType and required fragments.
 */
UCLASS(Blueprintable, BlueprintType, EditInlineNew)
class MYTHIC_API UItemDefinition : public UMythicDataAsset {
    GENERATED_BODY()

public:
    UItemDefinition();

    /** The name of the item */
    UPROPERTY(BlueprintReadOnly, EditAnywhere, meta=(DisplayName="Name"))
    FText Name;

    /** Short description of the item */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="Description"))
    FText Description;

    /** The type of the item, stored in gameplay tag Itemization.Type */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(Categories="Itemization.Type", DisplayName="ItemType"))
    FGameplayTag ItemType;

    /** Rarity of the item, stored in gameplay tag Itemization.Rarity */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(Categories="Itemization.Rarity", DisplayName="Rarity"))
    TEnumAsByte<EItemRarity> Rarity;

    /** Static mesh to use for the item when it is in the world */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="WorldMesh", MakeStructureDefaultValue="None"))
    TSoftObjectPtr<UStaticMesh> WorldMesh;

    /** 2d icon to use for the item when it is in the inventory */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="2dIcon", MakeStructureDefaultValue="None"))
    TSoftObjectPtr<UTexture2D> Icon2d;

    /** Stack size of the item. If greater than 1, the item can be stacked in the inventory if the fragments allow it */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="StackSizeMax", MakeStructureDefaultValue="0"))
    int32 StackSizeMax;

    // Set of ItemDefinitionFragments that define the item
    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(ShowOnlyInnerProperties, NoElementDuplicate), Instanced)
    TArray<TObjectPtr<UItemFragment>> Fragments;

    template <typename T>
    static const T *GetFragment(UItemDefinition *ItemDef) {
        for (auto frag : ItemDef->Fragments) {
            if (auto talent = Cast<T>(frag)) {
                return talent;
            }
        }
        return nullptr;
    }

#if WITH_EDITOR
    //~ Set Category - Auto-configure this item for a specific category

    /** Sets ItemType to Weapon and adds AttackFragment */
    UFUNCTION(CallInEditor, Category="Set Category")
    void Weapon();

    /** Sets ItemType to Tool and adds AttackFragment */
    UFUNCTION(CallInEditor, Category="Set Category")
    void Tool();

    /** Sets ItemType to Gear and adds AffixesFragment */
    UFUNCTION(CallInEditor, Category="Set Category")
    void Gear();

    /** Sets ItemType to Accessory and adds AffixesFragment */
    UFUNCTION(CallInEditor, Category="Set Category")
    void Accessory();

    /** Sets ItemType to Artifact and adds AffixesFragment */
    UFUNCTION(CallInEditor, Category="Set Category")
    void Artifact();

    /** Sets ItemType to Consumable and adds ConsumableActionFragment */
    UFUNCTION(CallInEditor, Category="Set Category")
    void Consumable();

    /** Sets ItemType to Learning and adds ConsumableActionFragment */
    UFUNCTION(CallInEditor, Category="Set Category")
    void Learning();

    /** Sets ItemType to Farming */
    UFUNCTION(CallInEditor, Category="Set Category")
    void Farming();

    /** Sets ItemType to Mining */
    UFUNCTION(CallInEditor, Category="Set Category")
    void Mining();

    /** Sets ItemType to Placable */
    UFUNCTION(CallInEditor, Category="Set Category")
    void Placable();

    /** Sets ItemType to Exploration */
    UFUNCTION(CallInEditor, Category="Set Category")
    void Exploration();

    /** Sets ItemType to Misc */
    UFUNCTION(CallInEditor, Category="Set Category")
    void Misc();

    virtual void PostLoad() override;
    virtual void PreSave(FObjectPreSaveContext SaveContext) override;
    virtual EDataValidationResult IsDataValid(class FDataValidationContext &Context) const override;

private:
    template <typename T>
    void EnsureFragment();
#endif
};

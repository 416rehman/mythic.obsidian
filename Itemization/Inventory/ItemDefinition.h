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
class USkeletalMesh;
class UStaticMesh;
class UTexture2D;

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

    /** Optional gameplay tag a player must own to EQUIP this item (e.g. a class/proficiency unlock). EMPTY = no
     *  requirement (any player can equip) — so this is non-breaking by default. Mirrors the crafting RequiredTag
     *  pattern (UCraftableFragment::RequiredTag); gates only player-driven equips via CanSlotAcceptItem. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="RequiredEquipTag"))
    FGameplayTag RequiredEquipTag;

    // static mesh to use for the item when it is in the world
    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="WorldMesh", MakeStructureDefaultValue="None"))
    TSoftObjectPtr<UStaticMesh> WorldMesh;

    // skeletal mesh to use for the item when it is equipped on the character
    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="EquippedMesh", MakeStructureDefaultValue="None"))
    TSoftObjectPtr<USkeletalMesh> EquippedMesh;

    /** 2d icon to use for the item when it is in the inventory */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="2dIcon", MakeStructureDefaultValue="None"))
    TSoftObjectPtr<UTexture2D> Icon2d;

    /** Stack size of the item. If greater than 1, the item can be stacked in the inventory if the fragments allow it */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="StackSizeMax", MakeStructureDefaultValue="0"))
    int32 StackSizeMax;

    /** Carry weight of ONE unit of this item (encumbrance). 0 = weightless (the default → non-breaking: a world of
     *  weightless items leaves every player Unencumbered even with encumbrance enabled). Summed × stack across the
     *  inventory and compared to the carry capacity by the encumbrance decision (see MythicEncumbrance). */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="Weight", ClampMin="0.0"))
    float Weight = 0.0f;

    /** Base monetary VALUE of ONE unit of this item, in currency units (gold). Drives vendor buy/sell pricing (see
     *  MythicCurrency). 0 = valueless / not sellable (the default → non-breaking until a designer prices items). For an
     *  Itemization.Type.Currency item this is the coin's own denomination; for everything else it's the merchant price. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="Value", ClampMin="0"))
    int32 Value = 0;

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

    /**
     * Single source of truth for the rarity -> display color mapping (Common/Rare/Epic/Legendary/Mythic).
     * Used by both the inventory-slot background tint (UItemSlotVM) and the loot-pickup callout. Do NOT
     * duplicate the hex literals at call sites — call this helper.
     */
    UFUNCTION(BlueprintCallable, Category = "Itemization")
    static FLinearColor GetRarityColor(EItemRarity InRarity);

#if WITH_EDITOR

    /**
     * Ensures the weapon's one Attack and one Affixes Fragment authoring shape after an exact supported weapon-class
     * ItemType has been selected. The generic Weapon parent is rejected because combat requires a concrete class.
     */
    UFUNCTION(CallInEditor, Category="Set Category")
    void Weapon();

    /** Configures this definition as a tool and ensures its one Attack Fragment authoring shape. */
    UFUNCTION(CallInEditor, Category="Set Category")
    void Tool();

    /** Configures this definition as gear and ensures its one Affixes Fragment authoring shape. */
    UFUNCTION(CallInEditor, Category="Set Category")
    void Gear();

    /** Configures this definition as an accessory and ensures its one Affixes Fragment authoring shape. */
    UFUNCTION(CallInEditor, Category="Set Category")
    void Accessory();

    /** Configures this definition as an artifact and ensures its one Affixes Fragment authoring shape. */
    UFUNCTION(CallInEditor, Category="Set Category")
    void Artifact();

    /** Configures this definition as a consumable and ensures its one Consumable Action Fragment authoring shape. */
    UFUNCTION(CallInEditor, Category="Set Category")
    void Consumable();

    /** Configures this definition as a learning item and ensures its one Consumable Action Fragment authoring shape. */
    UFUNCTION(CallInEditor, Category="Set Category")
    void Learning();

    /** Configures this definition as a farming item. */
    UFUNCTION(CallInEditor, Category="Set Category")
    void Farming();

    /** Configures this definition as a mining item. */
    UFUNCTION(CallInEditor, Category="Set Category")
    void Mining();

    /** Configures this definition as a placeable item. */
    UFUNCTION(CallInEditor, Category="Set Category")
    void Placable();

    /** Configures this definition as an exploration item. */
    UFUNCTION(CallInEditor, Category="Set Category")
    void Exploration();

    /** Configures this definition as a miscellaneous item. */
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

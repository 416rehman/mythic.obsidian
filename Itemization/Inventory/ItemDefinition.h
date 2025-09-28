#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Itemization/MythicTags_Inventory.h"
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

UCLASS(Blueprintable, BlueprintType, EditInlineNew)
class MYTHIC_API UItemDefinition : public UPrimaryDataAsset {
    GENERATED_BODY()

public:
    /** The name of the item */
    UPROPERTY(BlueprintReadOnly, EditAnywhere, meta=(DisplayName="Name"))
    FText Name;

    /** Short description of the item */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="Description"))
    FText Description;

    /** The type of the item, stored in gameplay tag Itemization.Type */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(Categories="Itemization.Type", DisplayName="ItemType"))
    FGameplayTag ItemType = ITEMIZATION_TYPE_MISC;

    /** Rarity of the item, stored in gameplay tag Itemization.Rarity */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(Categories="Itemization.Rarity", DisplayName="Rarity"))
    TEnumAsByte<EItemRarity> Rarity = Common;

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

    // Add these to the public section of UItemDefinition

#if WITH_EDITOR
    virtual void PostLoad() override;
    virtual void PreSave(FObjectPreSaveContext SaveContext) override;
    virtual EDataValidationResult IsDataValid(class FDataValidationContext &Context) const override;

    // Export to JSON
    void ExportAsJSONString(FString &OutJsonString) const;
    UFUNCTION(CallInEditor, Category="ItemDefinition")
    void ExportToJSON() const;
    UFUNCTION(CallInEditor, Category="ItemDefinition")
    void CopyJSONToClipboard() const;

    // Import from JSON
    UFUNCTION(CallInEditor, Category="ItemDefinition")
    void ImportFromJSON();
#endif
};

UCLASS(BlueprintType)
class MYTHIC_API UItemDefinitionJSONImportLibrary : public UBlueprintFunctionLibrary {
    GENERATED_BODY()

public:
#if WITH_EDITOR
    // Call in Editor only
    UFUNCTION(Blueprintable, BlueprintCallable, CallInEditor, Category="Mythic|Editor")
    static void ImportItemDefinitionsFromFolder();
#endif
};

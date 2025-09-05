#pragma once
#include "ActiveGameplayEffectHandle.h"
#include "AttributeSet.h"
#include "Itemization/Inventory/Fragments/FragmentTypes.h"
#include "Itemization/Inventory/Fragments/ItemFragment.h"
#include "Net/UnrealNetwork.h"
#include "AffixesFragment.generated.h"

USTRUCT(BlueprintType, Blueprintable)
struct FRolledAffix : public FRolledAttributeSpec {
    GENERATED_BODY()

    // Whether the affix can be refined. By default, the Forge's "Refine" system will lock these after it has refined 1 of the affixes. 
    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess=true))
    bool bIsLocked = false;

    // Constructor
    FRolledAffix(FGameplayAttribute Attribute, int ItemLvl, FRollDefinition &RollDef, bool IsLocked = true) : FRolledAttributeSpec(
        Attribute, ItemLvl, RollDef) {
        bIsLocked = IsLocked;
    }

    // Default Constructor
    FRolledAffix() : FRolledAttributeSpec() {
        bIsLocked = false;
    }
};

USTRUCT(BlueprintType)
struct FAffixesRuntimeReplicatedData {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    TArray<FRolledAffix> RolledCoreAffixes = TArray<FRolledAffix>();

    UPROPERTY(BlueprintReadWrite)
    TArray<FRolledAffix> RolledAffixes = TArray<FRolledAffix>();

    UPROPERTY(BlueprintReadOnly)
    UAbilitySystemComponent *ASC = nullptr;
};

USTRUCT(BlueprintType)
struct FAffixesRuntimeClientOnlyData {
    GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct FAffixesRuntimeServerOnlyData {
    GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct FAffixesConfig {
    GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct FAffixesBuildData {
    GENERATED_BODY()

    // Core Stats are GUARANTEED to be rolled when this item is instanced
    UPROPERTY(EditDefaultsOnly)
    TMap<FGameplayAttribute, FRollDefinition> CoreAffixes = TMap<FGameplayAttribute, FRollDefinition>();

    // Affixes from this pool will be applied to this item when it is instanced.
    // The amount of affixes on the final item is determined by the "Rarity" of the item.
    // Common = 1; Rare = 2; Epic = 3; Legendary = 4; Exotic = 5;
    UPROPERTY(EditDefaultsOnly)
    TMap<FGameplayAttribute, FRollDefinition> AffixPoolMap = TMap<FGameplayAttribute, FRollDefinition>();
};


/**
 * AffixesFragment can be used to modify attributes without any additional logic.
 * It can be used to add core stats to an item, or to add random affixes to an item.
 */
UCLASS(Blueprintable, BlueprintType)
class MYTHIC_API UAffixesFragment : public UItemFragment {
    GENERATED_BODY()

public:
    DECLARE_FRAGMENT(Affixes)

    /** Designer friendly configuration data that defines this fragment. */
    /** REPLICATED and fields should be BlueprintReadOnly */
    UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, meta=(ShowOnlyInnerProperties))
    FAffixesConfig AffixesConfig = FAffixesConfig();

    /** This is used in the OnInstanced method to calculate/fill the rest of the data. */
    /** This should not be replicated or blueprint accessible and safely discarded after being used in the OnInstanced method. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ShowOnlyInnerProperties))
    FAffixesBuildData AffixesBuildData = FAffixesBuildData();

    /** Contains the runtime state of the fragment (replicated to client) */
    /** REPLICATED */
    UPROPERTY(Replicated, BlueprintReadOnly)
    FAffixesRuntimeReplicatedData AffixesRuntimeReplicatedData = FAffixesRuntimeReplicatedData();

    /** Contains the runtime client side state of the fragment for use in methods like OnActiveItemClient */
    /** Shouldn't be accessed on server side methods like OnActiveItem. */
    UPROPERTY(BlueprintReadOnly)
    FAffixesRuntimeClientOnlyData AffixesRuntimeClientOnlyData = FAffixesRuntimeClientOnlyData();

    /** Contains the runtime server-only state of the fragment for use in methods like OnActiveItem */
    /** Shouldn't be accessed on client side methods like OnActiveItemClient. */
    UPROPERTY(BlueprintReadOnly)
    FAffixesRuntimeServerOnlyData AffixesRuntimeServerOnlyData = FAffixesRuntimeServerOnlyData();

    // Rolls the affixes for this item
    void RollAffixes(int ItemLevel, int Qty);

    // Rolls the core affixes for this item
    void RollCoreAffixes(int ItemLevel);

    // Applies affixes
    static void ApplyAffixes(UAbilitySystemComponent *ASC, TArray<FRolledAffix> &InRolledAffixes);
    // Removes applied affixes
    static void RemoveAffixes(UAbilitySystemComponent *ASC, TArray<FRolledAffix> &InRolledAffixes);

    // Checks if an affix is already rolled
    static bool IsAffixRolled(const FGameplayAttribute &Affix, TArray<FRolledAffix> &InRolledAffixes);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override {
        Super::GetLifetimeReplicatedProps(OutLifetimeProps);
        REP_FRAGMENT_DATA(Affixes)
    }

    //~ Overrides
#if WITH_EDITOR
    virtual bool IsValidFragment(FText &OutErrorMessage) const override;
#endif
    virtual void OnInstanced(UMythicItemInstance *Instance) override;
    virtual void OnItemActivated(UMythicItemInstance *ItemInstance) override;
    virtual void OnItemDeactivated(UMythicItemInstance *ItemInstance) override;

    virtual bool CanBeStackedWith(const UItemFragment *Other) const override;
    //~
};

// 

#pragma once

#include "CoreMinimal.h"
#include "Itemization/Inventory/Fragments/FragmentTypes.h"
#include "Itemization/Inventory/Fragments/ItemFragment.h"
#include "TalentFragment.generated.h"

UCLASS(Blueprintable, BlueprintType)
class UTalentPool : public UDataAsset {
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "TalentDefs")
    TArray<UTalentDefinition *> TalentDefs;
};

USTRUCT(Blueprintable, BlueprintType)
struct FTalentSpec : public FAbilityRollSpec {
    GENERATED_BODY()

    // The Talent Definition used to roll this talent spec
    UPROPERTY(BlueprintReadOnly)
    TSoftObjectPtr<UTalentDefinition> TalentDef = nullptr;

    // Whether this Talent can be swapped by another;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "TalentDefs")
    bool bIsLocked = false;

    // Constructor
    FTalentSpec(FAbilityDefinition &AbilityRoll, TSoftObjectPtr<UTalentDefinition> SourceTalentDef, bool IsLocked = false) : FAbilityRollSpec(AbilityRoll) {
        this->TalentDef = SourceTalentDef;
        bIsLocked = IsLocked;
    }

    FTalentSpec() : FAbilityRollSpec() {
        bIsLocked = false;
        TalentDef = nullptr;
    }
};

/// Contains the runtime state of the fragment (replicated to client)
/// This should be REPLICATED 
USTRUCT(BlueprintType)
struct FTalentRuntimeReplicatedData {
    GENERATED_BODY()

    // The rolled talents
    UPROPERTY(BlueprintReadOnly, Blueprintable)
    TArray<FTalentSpec> RolledTalents;
};

/// Contains the runtime client side state of the fragment for use in methods like OnActiveItemClient
/// Shouldn't be accessed on server side methods like OnActiveItem.
USTRUCT(BlueprintType)
struct FTalentRuntimeClientOnlyData {
    GENERATED_BODY()
};

/// Contains the runtime server-only state of the fragment for use in methods like OnActiveItem
/// Shouldn't be accessed on client side methods like OnActiveItemClient.
USTRUCT(BlueprintType)
struct FTalentRuntimeServerOnlyData {
    GENERATED_BODY()
};

/// Designer friendly configuration data that defines this fragment. This is accessible from the ItemDefinition Data Asset.
/// This should be REPLICATED and fields should be BlueprintReadOnly
USTRUCT(BlueprintType, Blueprintable, meta=(ShowOnlyInnerProperties))
struct FTalentConfig {
    GENERATED_BODY()
};

/// Designer friendly data that is used to create the instance data structs. This is used in the OnInstanced method to calculate/fill the rest of the data.
/// This should not be replicated or blueprint accessible and safely discarded after being used in the OnInstanced method.
USTRUCT(BlueprintType, meta=(ShowOnlyInnerProperties))
struct FTalentBuildData {
    GENERATED_BODY()

    // TalentDefs from this pool will be randomly chosen and applied to the item.
    // Amount of TalentDefs picked depends on Item Rarity.
    // Common, Rare, Epic = 0; Legendary = 1; Exotic = 2;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(ShowOnlyInnerProperties))
    TSoftObjectPtr<UTalentPool> TalentPool;
};

/**
 * TALENT: Grants an ability when the item becomes active and removed when the item becomes inactive
 */
UCLASS(Blueprintable, BlueprintType)
class MYTHIC_API UTalentFragment : public UItemFragment {
    GENERATED_BODY()

public:
    DECLARE_FRAGMENT(Talent)

    /** Designer friendly configuration data that defines this fragment. */
    /** REPLICATED and fields should be BlueprintReadOnly */
    UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, meta=(ShowOnlyInnerProperties))
    FTalentConfig TalentConfig;

    /** This is used in the OnInstanced method to calculate/fill the rest of the data. */
    /** This should not be replicated or blueprint accessible and safely discarded after being used in the OnInstanced method. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ShowOnlyInnerProperties))
    FTalentBuildData TalentBuildData;

    /** Contains the runtime state of the fragment (replicated to client) */
    /** REPLICATED */
    UPROPERTY(Replicated, BlueprintReadOnly)
    FTalentRuntimeReplicatedData TalentRuntimeReplicatedData;

    /** Contains the runtime client side state of the fragment for use in methods like OnActiveItemClient */
    /** Shouldn't be accessed on server side methods like OnActiveItem. */
    UPROPERTY(BlueprintReadOnly)
    FTalentRuntimeClientOnlyData TalentRuntimeClientOnlyData;

    /** Contains the runtime server-only state of the fragment for use in methods like OnActiveItem */
    /** Shouldn't be accessed on client side methods like OnActiveItemClient. */
    UPROPERTY(BlueprintReadOnly)
    FTalentRuntimeServerOnlyData TalentRuntimeServerOnlyData;

    //~ Overrides
#if WITH_EDITOR
    virtual bool IsValidFragment(FText &OutErrorMessage) const override;
#endif
    virtual void OnInstanced(UMythicItemInstance *ItemInstance) override;
    virtual void OnItemActivated(UMythicItemInstance *ItemInstance) override;
    virtual void OnItemDeactivated(UMythicItemInstance *ItemInstance) override;

    virtual bool CanBeStackedWith(const UItemFragment *Other) const override;
    //~

    void RollTalents(UTalentPool *TalentPool, int NumTalentsToRoll);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override {
        Super::GetLifetimeReplicatedProps(OutLifetimeProps);
        REP_FRAGMENT_DATA(Talent)
    }

    //~ GiveAbility only works on the server
    UFUNCTION(Server, Reliable)
    void ServerHandleGrantAbility();
    //~

    //~ RemoveAbility only works on the server
    UFUNCTION(Server, Reliable)
    void ServerRemoveAbility();
    //~

    // Helper function to get the talent for a specific TalentDefinition
    UFUNCTION(BlueprintCallable, Category = "Talent")
    bool GetTalentSpec(const UTalentDefinition *TalentDef, FTalentSpec &OutTalentSpec) {
        for (FTalentSpec TalentSpec : this->TalentRuntimeReplicatedData.RolledTalents) {
            if (TalentSpec.TalentDef == TalentDef) {
                OutTalentSpec = TalentSpec;
                return true;
            }
        }

        return false;
    }
};

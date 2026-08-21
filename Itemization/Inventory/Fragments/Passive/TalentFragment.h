
#pragma once

#include "CoreMinimal.h"
#include "Itemization/Inventory/Fragments/FragmentTypes.h"
#include "GAS/Abilities/MythicAbilityRollSource.h"
#include "Itemization/Inventory/Fragments/ItemFragment.h"
#include "TalentFragment.generated.h"

class UMythicLootSettings;

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
    UPROPERTY(BlueprintReadOnly, SaveGame)
    TSoftObjectPtr<UTalentDefinition> TalentDef = nullptr;

    // Whether this Talent can be swapped by another;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "TalentDefs", SaveGame)
    bool bIsLocked = false;

    FTalentSpec(FAbilityDefinition &AbilityRoll, TSoftObjectPtr<UTalentDefinition> SourceTalentDef, bool IsLocked = false) : FAbilityRollSpec(AbilityRoll) {
        this->TalentDef = SourceTalentDef;
        bIsLocked = IsLocked;
    }

    FTalentSpec() : FAbilityRollSpec() {
        bIsLocked = false;
        TalentDef = nullptr;
    }
};

USTRUCT(BlueprintType)
struct FTalentRuntimeReplicatedData {
    GENERATED_BODY()

    // The rolled talents
    UPROPERTY(BlueprintReadOnly, Blueprintable, SaveGame)
    TArray<FTalentSpec> RolledTalents;
};

USTRUCT(BlueprintType)
struct FTalentRuntimeClientOnlyData {
    GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct FTalentRuntimeServerOnlyData {
    GENERATED_BODY()
};

USTRUCT(Blueprintable, BlueprintType)
struct FTalentConfig {
    GENERATED_BODY()
};

USTRUCT(BlueprintType, meta=(ShowOnlyInnerProperties))
struct FTalentBuildData {
    GENERATED_BODY()

    // TalentDefs from this pool will be randomly chosen and applied to the item.
    // Amount of TalentDefs picked depends on Item Rarity, set in Project Settings -> Game -> Mythic Loot Settings
    // (TalentCountByRarity). Shipped defaults: Common = 0; Rare, Epic, Legendary = 1; Mythic = 2.
    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(ShowOnlyInnerProperties))
    TSoftObjectPtr<UTalentPool> TalentPool;
};

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class MYTHIC_API UTalentFragment : public UItemFragment, public IMythicAbilityRollSource {
    GENERATED_BODY()

public:
    DECLARE_FRAGMENT(Talent)

    /** Designer friendly configuration data that defines this fragment. */
    /** REPLICATED and fields should be BlueprintReadOnly */
    UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, meta=(ShowOnlyInnerProperties), SaveGame)
    FTalentConfig TalentConfig;

    /** This is used in the OnInstanced method to calculate/fill the rest of the data. */
    /** This should not be replicated or blueprint accessible and safely discarded after being used in the OnInstanced method. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ShowOnlyInnerProperties))
    FTalentBuildData TalentBuildData;

    /** Contains the runtime state of the fragment (replicated to client) */
    /** REPLICATED */
    UPROPERTY(Replicated, BlueprintReadOnly, SaveGame)
    FTalentRuntimeReplicatedData TalentRuntimeReplicatedData;

    /** Contains the runtime client side state of the fragment for use in methods like OnActiveItemClient */
    /** Shouldn't be accessed on server side methods like OnActiveItem. */
    UPROPERTY(BlueprintReadOnly)
    FTalentRuntimeClientOnlyData TalentRuntimeClientOnlyData;

    /** Contains the runtime server-only state of the fragment for use in methods like OnActiveItem */
    /** Shouldn't be accessed on client side methods like OnActiveItemClient. */
    UPROPERTY(BlueprintReadOnly)
    FTalentRuntimeServerOnlyData TalentRuntimeServerOnlyData;

#if WITH_EDITOR
    virtual bool IsValidFragment(FText &OutErrorMessage) const override;
#endif
    virtual void OnInstanced(UMythicItemInstance *ItemInstance) override;
    virtual void OnItemActivated(UMythicItemInstance *ItemInstance) override;
    virtual void OnItemDeactivated(UMythicItemInstance *ItemInstance) override;

    virtual bool CanBeStackedWith(const UItemFragment *Other) const override;

    void RollTalents(UTalentPool *TalentPool, int NumTalentsToRoll, EItemRarity ItemRarity, const FGameplayTagContainer &TypeProbe);


    static int32 ResolveTalentCount(int32 Rarity, const UMythicLootSettings *LootSettings);

    static bool IsTalentEligible(EItemRarity ItemRarity, EItemRarity MinRarity) {
        return static_cast<int32>(ItemRarity) >= static_cast<int32>(MinRarity);
    }

    static bool IsTalentAllowedOnItem(const FGameplayTagQuery &AllowedItemTypes, const FGameplayTagContainer &TypeProbe) {
        return AllowedItemTypes.IsEmpty() || AllowedItemTypes.Matches(TypeProbe);
    }

    static TArray<int32> SampleWithoutReplacement(const TArray<int32> &EligibleIndexes, int32 NumToPick, FRandomStream &Rng);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override {
        Super::GetLifetimeReplicatedProps(OutLifetimeProps);
        REP_FRAGMENT_DATA(Talent)
    }

    UFUNCTION(Server, Reliable)
    void ServerHandleGrantAbility();

    UFUNCTION(Server, Reliable)
    void ServerRemoveAbility();

    virtual bool GetRolledAbilityValue(const FGameplayAbilitySpecHandle &Handle, const FGameplayTag &Parameter, float &OutValue) const override;

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

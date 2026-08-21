#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ConversionTypes.h"
#include "Itemization/MythicDataAsset.h"
#include "ConversionRecipe.generated.h"

class UItemDefinition;
class UMythicItemInstance;
class UTexture2D;
class UProficiencyDefinition;
class UConversionStationComponent;
class AController;

struct FMythicConversionProductContext {
    int32 SnapshotInputLevel = 0;

    int32 SnapshotCrafterProficiencyLevel = 0;

    float SnapshotAvgQualityTierValue = -1.0f;

    float SnapshotMinFreshnessFraction = 1.0f;

    AController *InstigatorController = nullptr;

    UConversionStationComponent *Station = nullptr;
};

USTRUCT(BlueprintType)
struct MYTHIC_API FConversionIngredient {
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
    EConversionMatchMode MatchMode = EConversionMatchMode::ExactItem;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta=(EditCondition="MatchMode == EConversionMatchMode::ExactItem", EditConditionHides))
    TSoftObjectPtr<UItemDefinition> ExactItem = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta=(EditCondition="MatchMode == EConversionMatchMode::TypeQuery", EditConditionHides))
    FGameplayTagQuery TypeQuery;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta=(ClampMin="1", ClampMax="9999"))
    int32 RequiredAmount = 1;

    // FALSE = catalyst/tool: must be PRESENT each cycle but NOT consumed.
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
    bool bConsumed = true;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta=(ClampMin="0"))
    int32 MinItemLevel = 0;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
    FGameplayTagContainer RequiredItemTags;

    // Editor-only readable label for array TitleProperty.
    UPROPERTY(VisibleAnywhere, Transient)
    FString DisplayLabel;

    bool MatchesInstance(const UMythicItemInstance *Inst) const;
};

USTRUCT(BlueprintType)
struct MYTHIC_API FConversionProduct {
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
    EConversionProductMode Mode = EConversionProductMode::Create;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="1.0"))
    float Probability = 1.0f;

    // ---- Create ----
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta=(EditCondition="Mode == EConversionProductMode::Create", EditConditionHides))
    TSoftObjectPtr<UItemDefinition> ItemDefinition = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta=(ClampMin="1", EditCondition="Mode == EConversionProductMode::Create", EditConditionHides))
    int32 Quantity = 1;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta=(EditCondition="Mode == EConversionProductMode::Create", EditConditionHides))
    EProductLevelMode LevelMode = EProductLevelMode::FixedLevel;

    // FixedLevel: the product level (FixedLevel mode) OR the BASE level that proficiency scaling adds to (ProficiencyScaled mode).
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite,
        meta=(ClampMin="0", EditCondition="Mode == EConversionProductMode::Create && (LevelMode == EProductLevelMode::FixedLevel || LevelMode == EProductLevelMode::ProficiencyScaled)", EditConditionHides))
    int32 FixedLevel = 1;

    // Item-levels added per crafter proficiency level (ProficiencyScaled mode — the skill is the recipe's CraftingProficiency).
    // ClampMax keeps the scaled product well within int32 (the math is overflow-hardened regardless, but a sane editor
    // range avoids absurd configs).
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite,
        meta=(ClampMin="0", ClampMax="10000", EditCondition="Mode == EConversionProductMode::Create && LevelMode == EProductLevelMode::ProficiencyScaled", EditConditionHides))
    int32 ProficiencyLevelBonus = 1;

    // Upper cap on the proficiency-scaled product level (0 = uncapped).
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite,
        meta=(ClampMin="0", EditCondition="Mode == EConversionProductMode::Create && LevelMode == EProductLevelMode::ProficiencyScaled", EditConditionHides))
    int32 MaxProductLevel = 0;

    // ---- Transform (mutates the consumed input instance in place) ----
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite,
        meta=(Categories="Itemization.Type", EditCondition="Mode == EConversionProductMode::Transform", EditConditionHides))
    FGameplayTag NewItemType;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta=(EditCondition="Mode == EConversionProductMode::Transform", EditConditionHides))
    FGameplayTagContainer TagsToAdd;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta=(EditCondition="Mode == EConversionProductMode::Transform", EditConditionHides))
    FGameplayTagContainer TagsToRemove;

    // Optional def swap. If set AND it changes ItemType, IsDataValid requires NewItemType == TransformToDefinition->ItemType.
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta=(EditCondition="Mode == EConversionProductMode::Transform", EditConditionHides))
    TSoftObjectPtr<UItemDefinition> TransformToDefinition = nullptr;

    // After the transform applies, fully restore the transformed instance's UDurabilityFragment (clears the broken
    // latch). This lets a Transform product double as a REPAIR job — pair a durable item (the transformed input)
    // with a consumed repair-material ingredient and leave the type/tag fields empty for a pure repair. No-op if the
    // instance has no durability fragment. Transform mode only (repair operates on the held input instance).
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta=(EditCondition="Mode == EConversionProductMode::Transform", EditConditionHides))
    bool bRepairToFull = false;

    UPROPERTY(VisibleAnywhere, Transient)
    FString DisplayLabel;
};

USTRUCT(BlueprintType)
struct MYTHIC_API FConversionRequirements {
    GENERATED_BODY()

    // Matched against the instigator's GetSchematicsASC() owned tags. Empty => no instigator gate.
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
    FGameplayTagQuery InstigatorTagQuery;

    // Matched against the station's owned StationTags. Empty => any station.
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
    FGameplayTagQuery StationTagQuery;
};

USTRUCT(BlueprintType)
struct MYTHIC_API FFuelDefinition {
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta=(ShowOnlyInnerProperties))
    FConversionIngredient FuelMatch;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta=(ClampMin="0.0"))
    float BurnSecondsPerUnit = 4.0f;

    UPROPERTY(VisibleAnywhere, Transient)
    FString DisplayLabel;
};

USTRUCT(BlueprintType)
struct MYTHIC_API FConversionProcess {
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
    EConversionTrigger Trigger = EConversionTrigger::ManualSelect;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
    EConversionTiming Timing = EConversionTiming::Timed;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta=(ClampMin="0.0", EditCondition="Timing != EConversionTiming::Instant", EditConditionHides))
    float Duration = 3.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta=(EditCondition="Timing == EConversionTiming::Continuous", EditConditionHides))
    bool bRepeatWhileInputsAvailable = true;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
    EConversionOutputRouting OutputRouting = EConversionOutputRouting::StationOutputSlots;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
    bool bRequiresFuel = false;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta=(EditCondition="bRequiresFuel", EditConditionHides, TitleProperty="DisplayLabel"))
    TArray<FFuelDefinition> AcceptedFuels;

    // Tie-break among recipes matching the same inputs at a station: higher wins.
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
    int32 Priority = 0;
};

UCLASS(BlueprintType, Blueprintable)
class MYTHIC_API UConversionRecipe : public UMythicDataAsset {
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Recipe|Display")
    FText DisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Recipe|Display", meta=(MultiLine=true))
    FText Description;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Recipe|Display")
    TSoftObjectPtr<UTexture2D> Icon;

    // Stable identity for this recipe (also the key used by the job queue on the wire). Mod recipes use a Mod.* tag.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Recipe|Display", meta=(Categories="Itemization.Recipe"))
    FGameplayTag RecipeId;

    // Optional categorization for UI grouping.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Recipe|Display", meta=(Categories="Itemization.Process"))
    FGameplayTag ProcessCategory;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Recipe|Inputs", meta=(TitleProperty="DisplayLabel"))
    TArray<FConversionIngredient> Inputs;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Recipe|Outputs", meta=(TitleProperty="DisplayLabel"))
    TArray<FConversionProduct> Products;

    // ---- Proficiency / progression (the ONE skill this recipe trains) ----
    // Which proficiency this recipe trains. Drives BOTH the ProficiencyScaled product level (crafter's level → output
    // quality) AND the XP awarded on completion. Null = this recipe trains no skill (no scaling, no XP).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Recipe|Proficiency")
    TObjectPtr<UProficiencyDefinition> CraftingProficiency = nullptr;

    // XP granted to the crafter's CraftingProficiency on each completed produce cycle. 0 = no XP (conservative default —
    // a recipe opts into training). Requires CraftingProficiency set.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Recipe|Proficiency", meta=(ClampMin="0"))
    float CraftingXpReward = 0.0f;

    // Anti-grind: once the crafter's level in CraftingProficiency is at or above this, the recipe grants no more XP
    // (you don't master smithing by forging nails forever). 0 = no cap (always grants while CraftingXpReward > 0).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Recipe|Proficiency", meta=(ClampMin="0"))
    int32 XpNoGainAtOrAboveLevel = 0;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Recipe|Requirements", meta=(ShowOnlyInnerProperties))
    FConversionRequirements Requirements;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Recipe|Process", meta=(ShowOnlyInnerProperties))
    FConversionProcess Process;

    bool MatchesStation(const FGameplayTagContainer &StationOwnedTags) const;

    bool MatchesInputs(const TArray<UMythicItemInstance *> &InInputs) const;


    virtual void PostProcessProduct(UMythicItemInstance *ProductInstance, const FMythicConversionProductContext &Context) const {}

    virtual bool IsVisibleTo(const FGameplayTagContainer &InstigatorOwnedTags) const { return true; }

    virtual bool PassesDynamicGates(AController *Instigator, FText &OutReason) const { return true; }

#if WITH_EDITOR
    virtual void PostLoad() override;
    virtual void PostEditChangeProperty(struct FPropertyChangedEvent &PropertyChangedEvent) override;
    virtual EDataValidationResult IsDataValid(class FDataValidationContext &Context) const override;

    void RebuildDisplayLabels();
#endif
};

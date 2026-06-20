#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ConversionTypes.h"
#include "Itemization/MythicDataAsset.h"
#include "ConversionRecipe.generated.h"

class UItemDefinition;
class UMythicItemInstance;
class UTexture2D;

/**
 * One ingredient slot of a recipe. Matches a concrete item either by exact definition or by a tag query
 * over the item's effective type ({def ItemType} ∪ runtime ItemTags), so "any ore" works. Non-consumed
 * ingredients are catalysts/tools: required to be present each cycle but never removed.
 */
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

    // Server-side: does this concrete instance satisfy this slot? (does NOT check amount.)
    bool MatchesInstance(const UMythicItemInstance *Inst) const;
};

/**
 * One product of a recipe. Either CREATES a new item, or TRANSFORMS a consumed input in place (preserving
 * the input instance's fragments / level / seed / quantity), optionally swapping its definition.
 */
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

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite,
        meta=(ClampMin="0", EditCondition="Mode == EConversionProductMode::Create && LevelMode == EProductLevelMode::FixedLevel", EditConditionHides))
    int32 FixedLevel = 1;

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

/** Gate queries evaluated against the instigator's schematic/proficiency tags and the station's owned tags. */
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

/** A fuel item type and how many burn-seconds one unit yields. Reuses ingredient matching ("any wood"). */
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

/** How a recipe processes: trigger, timing, fuel, repeat, output routing. */
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

/**
 * The atomic, configurable unit of conversion: {Inputs, Products, Requirements, Process}.
 * A station type is expressed purely as data: a set of recipes + a station tag + an inventory profile.
 * Discovered by UConversionSubsystem as a primary asset (mirrors UItemDefinition).
 */
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

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Recipe|Requirements", meta=(ShowOnlyInnerProperties))
    FConversionRequirements Requirements;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Recipe|Process", meta=(ShowOnlyInnerProperties))
    FConversionProcess Process;

    // True if this recipe's station gate is satisfied by the given station tags.
    bool MatchesStation(const FGameplayTagContainer &StationOwnedTags) const;

    // True if every ingredient (consumed + catalyst) is satisfiable by the provided instances (presence/amount).
    bool MatchesInputs(const TArray<UMythicItemInstance *> &InInputs) const;

#if WITH_EDITOR
    virtual void PostLoad() override;
    virtual void PostEditChangeProperty(struct FPropertyChangedEvent &PropertyChangedEvent) override;
    virtual EDataValidationResult IsDataValid(class FDataValidationContext &Context) const override;

    // Rebuilds the transient DisplayLabel strings used as array TitleProperty in the editor.
    void RebuildDisplayLabels();
#endif
};

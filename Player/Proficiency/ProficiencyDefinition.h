#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "GameplayEffectTypes.h"
#include "Engine/DataAsset.h"
#include "Rewards/RewardBase.h"
#include "Stats/MythicStatTypes.h"
#include "ProficiencyDefinition.generated.h"
class URewardBase;

const float STARTING_XP = 100.0f;

/** Typed permanent-stat target and end-of-track tuning goal for generated proficiency rewards. */
USTRUCT(BlueprintType, Blueprintable)
struct FAttributeGoal {
    GENERATED_BODY()

    FAttributeGoal();

    FAttributeGoal(FMythicStatDefinitionHandle InStatDefinition,
                   float InGoal,
                   EGameplayModOp::Type InModifier);

    /** Canonical Stat Definition improved by the generated permanent milestone rewards. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attribute Goal")
    FMythicStatDefinitionHandle TargetStat;

    /** Total authored operand distributed across generated rewards by the end of this proficiency track. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attribute Goal")
    float Goal = 0.0f;

    /** Permanent-stat operation applied by each generated reward. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attribute Goal")
    TEnumAsByte<EGameplayModOp::Type> Modifier = EGameplayModOp::Additive;
};

/** Authored milestone presentation and reward templates placed onto the compiled proficiency track. */
USTRUCT(BlueprintType, Blueprintable)
struct FMilestone {
    GENERATED_BODY()

    /** Player-facing milestone icon shown in track previews and reward callouts. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Milestone")
    TSoftObjectPtr<UTexture2D> Icon;

    /** Localized player-facing milestone name. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Milestone")
    FText Name;

    /**
     * Reward templates cloned into transient runtime instances when this milestone is compiled. Attribute reward
     * identity derives from this authored milestone index and reward slot, so balance-driven level placement can move.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Milestone")
    TArray<URewardBase *> Rewards;
};

/** Primary Data Asset that owns the complete semantic, progression, presentation, and reward design for one track. */
UCLASS(BlueprintType, Blueprintable)
class MYTHIC_API UProficiencyDefinition : public UPrimaryDataAsset {
    GENERATED_BODY()

public:
    UProficiencyDefinition();

    /** Returns the registered typed identity used by persistence and data-driven proficiency discovery. */
    virtual FPrimaryAssetId GetPrimaryAssetId() const override;

    /** Localized player-facing name of this proficiency track. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency Track")
    FText Name;

    /** Localized explanation of what advances this track and what mastery provides. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency Track")
    FText Description;

    /**
     * The track's own mark -- the pick for Mining, the sickle for Harvesting. Shown on the track's row and at the
     * head of its rail, so a player finds a trade by its tool rather than by reading twelve names.
     *
     * Soft, and loaded by the page when it opens rather than per row: a TryLoad on a recycled row is a synchronous
     * disk hitch mid-scroll.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency Track")
    TSoftObjectPtr<UTexture2D> Icon;

    /**
     * Identifies this track to gameplay rules, e.g. Proficiency.Woodcutting. Carried on the GAS.Event.Proficiency.Gained
     * event this track fires, which is what lets a talent or a woven spell say "when you fell a tree" instead of only
     * "when you do anything productive". UNSET = the event still fires, it just cannot be gated by track.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency Track")
    FGameplayTag TrackTag;

    /**
     * Canonical current-value Stat Definition for this track's XP. Its paired capacity Stat Definition stores
     * Max XP, so progression, GAS, save data, and the data-driven stat registry share one typed source.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency Track | Rewards")
    FMythicStatDefinitionHandle ProgressStat;

    /** Hand-authored milestone rewards distributed across the generated progression track. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency Track | Rewards")
    TArray<FMilestone> KeyMilestones;

    /** Permanent stat gains distributed across milestones so each target reaches its authored goal at maximum level. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency Track | Rewards")
    TArray<FAttributeGoal> AttributeGoals;

    /** Highest attainable track level and the endpoint used to distribute generated milestone rewards. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency Track | Balancing")
    int32 MaxLevel = 30;

    /** Multiplier applied to each successive level-up cost; 1.2 makes every next level cost 20% more XP. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency Track | Balancing")
    float GrowthRate = 1.2f;

    /** Baseline XP for one representative action before reward percentage, level, and player bonuses are applied. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency Track | Balancing")
    float BaseXPPerAction = 10.0f;

    /**
     * The authored definition whose track fills the given attribute, found once through the asset
     * registry and cached. This is how systems outside the proficiency component (the growth MMC, the
     * overall-level formula) reach a track's authored curve without a hardcoded asset path.
     */
    static const UProficiencyDefinition *FindByProgressAttribute(const FGameplayAttribute &Attribute);

    /** Returns the loaded canonical current-XP Stat Definition, or null while semantic data is unavailable. */
    const UMythicStatDefinition *GetProgressStatDefinition() const;

    /** Returns the GAS attribute owned by Progress Stat, or an invalid attribute when unavailable. */
    FGameplayAttribute GetProgressAttribute() const;

    /** Returns the GAS attribute owned by Progress Stat's paired capacity definition. */
    FGameplayAttribute GetProgressCapacityAttribute() const;

    /** Returns the XP cost to advance from Level to Level + 1 using the definition's progression curve. */
    UFUNCTION(BlueprintCallable, Category = "Proficiency Track")
    static float CalcXPCostForLevelUp(int32 Level, const UProficiencyDefinition *ProficiencyDefinition);

    /** Returns total lifetime XP required to reach Level; level one always starts at zero XP. */
    UFUNCTION(BlueprintCallable, Category = "Proficiency Track")
    static float CalcCumulativeXPForLevel(int32 Level, const UProficiencyDefinition *ProficiencyDefinition);

    /** Converts lifetime XP into the clamped level reached on the supplied proficiency definition. */
    UFUNCTION(BlueprintCallable, Category = "Proficiency Track")
    static int32 CalcLevelAtXP(float XP, const UProficiencyDefinition *ProficiencyDefinition);

    /** Returns non-negative XP still required to reach TargetLevel from the supplied lifetime XP. */
    UFUNCTION(BlueprintCallable, Category = "Proficiency Track")
    static float CalcXPRemainingForLevel(float CurrentXP, int32 TargetLevel, const UProficiencyDefinition *ProficiencyDefinition);

#if WITH_EDITOR

public:
    /** Validates progression tuning and every typed permanent-stat reward target. */
    virtual EDataValidationResult IsDataValid(FDataValidationContext &Context) const override;

    /** Builds a designer-facing level and cumulative-XP table for auditing this asset's progression curve. */
    UFUNCTION(BlueprintCallable, Category = "Proficiency Track | Debug")
    FString GetProgressionBreakdown() const;

    /** Estimates real time to maximum level at a constant positive action rate for balance review. */
    UFUNCTION(BlueprintCallable, Category = "Proficiency Track | Debug")
    FString GetTimeToMaxLevelEstimate(float ActionsPerMinute = 1.0f) const;

    virtual void PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent) override;
#endif
};

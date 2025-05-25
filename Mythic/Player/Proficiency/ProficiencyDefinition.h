#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "GameplayEffectTypes.h"
#include "Engine/DataAsset.h"
#include "Rewards/RewardBase.h"
#include "ProficiencyDefinition.generated.h"
class URewardBase;

const float STARTING_XP = 100.0f;

USTRUCT(BlueprintType, Blueprintable)
struct FAttributeGoal {
    GENERATED_BODY()

    // constructor
    FAttributeGoal();

    FAttributeGoal(FGameplayAttribute InAttribute, float InGoal, EGameplayModOp::Type InModifier);

    // The attribute to improve
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attribute Goal")
    FGameplayAttribute Attribute = FGameplayAttribute();

    // The value to reach for this attribute by the end of the proficiency track
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attribute Goal")
    float Goal = 0.0f;

    // The operation to perform on the attribute
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attribute Goal")
    TEnumAsByte<EGameplayModOp::Type> Modifier = EGameplayModOp::Additive;
};

USTRUCT(BlueprintType, Blueprintable)
struct FMilestone {
    GENERATED_BODY()

    // Icon for the milestone
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Milestone")
    TSoftObjectPtr<UTexture2D> Icon;

    // The name of the milestone
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Milestone")
    FText Name;

    // Reward(s) for reaching this milestone
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Milestone")
    TArray<URewardBase *> Rewards;
};

/**
 * This can be used to create proficiency tracks for the player to level up in.
 * Give it a name, a description, list of milestone rewards, and list of attributes to improve up to a maximum level,
 * and the attribute improvements will be scattered across the milestones.
 */
UCLASS(BlueprintType, Blueprintable)
class MYTHIC_API UProficiencyDefinition : public UDataAsset {
    GENERATED_BODY()

public:
    // Constructor for the proficiency track
    UProficiencyDefinition();

    // The name of the proficiency track
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency Track")
    FText Name;

    // The description of the proficiency track
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency Track")
    FText Description;

    // The attribute to use to track the progress of this proficiency track
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency Track | Rewards")
    FGameplayAttribute ProgressAttribute = FGameplayAttribute();

    // The list of milestones for this proficiency track
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency Track | Rewards")
    TArray<FMilestone> KeyMilestones;

    // The list of attributes to improve. By the end of the proficiency track, the attributes will reach the goal values.
    // These will be spread across the milestones.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency Track | Rewards")
    TArray<FAttributeGoal> AttributeGoals;

    // Maximum level of the proficiency track
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency Track | Balancing")
    int32 MaxLevel = 30;

    // Growth rate of the proficiency track. After each level, the required experience to level up will be multiplied by this value.
    // For example, if the growth rate is 1.2, the required experience for the next level will be:
    //   XP_for_next = current_level_cost * 1.2
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency Track | Balancing")
    float GrowthRate = 1.2f;

    // Base XP per action. For instance, a combat proficiency track might reward XP per enemy kill.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency Track | Balancing")
    float BaseXPPerAction = 10.0f;
    
    //----------------------------------------------------------------------------
    // 1. Returns the XP cost to level up from the specified level (i.e. from level L to L+1)
    //----------------------------------------------------------------------------
    UFUNCTION(BlueprintCallable, Category = "Proficiency Track")
    static float CalcXPCostForLevelUp(int32 Level, const UProficiencyDefinition *ProficiencyDefinition);

    //----------------------------------------------------------------------------
    // 2. Returns the cumulative XP required to reach the specified level.
    //    Level 1 is reached at 0 XP; for level > 1, this is the sum of all previous level-up costs.
    //    For a geometric progression:
    //       Cumulative XP = STARTING_XP * ((GrowthRate^(Level - 1) - 1) / (GrowthRate - 1))
    //----------------------------------------------------------------------------
    UFUNCTION(BlueprintCallable, Category = "Proficiency Track")
    static float CalcCumulativeXPForLevel(int32 Level, const UProficiencyDefinition *ProficiencyDefinition);

    //----------------------------------------------------------------------------
    // 3. Given cumulative XP, returns the current level.
    //    This inverts the cumulative XP formula.
    //----------------------------------------------------------------------------
    UFUNCTION(BlueprintCallable, Category = "Proficiency Track")
    static int32 CalcLevelAtXP(float XP, const UProficiencyDefinition *ProficiencyDefinition);

    //----------------------------------------------------------------------------
    // 4. Given current cumulative XP and a target level, returns the XP remaining to reach that target level.
    //    If the current XP already meets or exceeds the cumulative XP for TargetLevel, this returns 0.
    //----------------------------------------------------------------------------
    UFUNCTION(BlueprintCallable, Category = "Proficiency Track")
    static float CalcXPRemainingForLevel(float CurrentXP, int32 TargetLevel, const UProficiencyDefinition *ProficiencyDefinition);

#if WITH_EDITOR

public:
    // A debug helper that prints out the progression table.
    // Note: In gameplay, a player starts at level 1 with 0 XP and must earn XP for level 2, 3, etc.
    UFUNCTION(BlueprintCallable, Category = "Proficiency Track | Debug")
    FString GetProgressionBreakdown() const;

    UFUNCTION(BlueprintCallable, Category = "Proficiency Track | Debug")
    FString GetTimeToMaxLevelEstimate(float ActionsPerMinute = 1.0f) const;

    virtual void PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent) override;
#endif
};

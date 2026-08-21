#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Narrative/MythicStoryCondition.h"
#include "Rewards/RewardBase.h"
#include "MythicUnlockEngine.h"
#include "MythicUnlockRule.generated.h"

UCLASS(BlueprintType)
class MYTHIC_API UMythicUnlockRule : public UDataAsset {
    GENERATED_BODY()

public:
    // Stable identity persisted to mark this rule "applied" (so it never re-fires across a save-load). Author a unique
    // Unlock.Rule.* tag per rule. If left invalid, the component falls back to EffectPayloadTag for persistence.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Unlock")
    FGameplayTag RuleId;

    // Gate over the owned tag set. Empty = fires once immediately (a baseline/starter unlock).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Unlock")
    FMythicStoryCondition Precondition;

    // What this rule grants when it fires.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Unlock")
    EMythicUnlockEffect Effect = EMythicUnlockEffect::GrantTitle;

    // The payload the effect operates on: the Title.*/Cosmetic.* tag to grant, or the Perk.*/Skill.*/Recipe.*/POI tag to
    // unlock. Unused by GrantReward.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Unlock")
    FGameplayTag EffectPayloadTag;

    // Reward given when the rule fires. Required for GrantReward; a permitted bonus for any other effect. NOT re-given on
    // reload (the component gates on the persisted applied-rule set + a restore guard).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Unlock")
    FRewardsToGive OptionalReward;
};

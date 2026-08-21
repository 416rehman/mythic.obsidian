
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Rewards/RewardBase.h"
#include "Narrative/MythicStoryCondition.h"
#include "MythicQuestOutcome.generated.h"

USTRUCT(BlueprintType)
struct FMythicQuestOutcome {
    GENERATED_BODY()

    // Canonical id of this ending (telemetry / "which outcome was reached"). Not used for matching — `When` decides.
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Narrative|Quest")
    FGameplayTag OutcomeTag;

    // Tag gate deciding whether this is the ending reached, evaluated against the player's owned story tags. The FIRST
    // outcome (in array order) whose `When` passes wins. Empty (all clauses empty) always passes → catch-all default.
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Narrative|Quest")
    FMythicStoryCondition When;

    // Rewards granted (server-side) when this outcome resolves. Reuses the canonical FRewardsToGive so the derived XP /
    // item-level contexts are built correctly — a bare TArray<URewardBase*> would zero those.
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Narrative|Quest")
    FRewardsToGive Rewards;

    // Story tags stamped into the player's narrative ledger when this outcome resolves — the ending's lasting consequence.
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Narrative|Quest")
    FGameplayTagContainer GrantStoryTags;

    static int32 ResolveQuestOutcome(const TArray<FMythicQuestOutcome> &Outcomes, const FGameplayTagContainer &OwnedTags) {
        for (int32 i = 0; i < Outcomes.Num(); ++i) {
            if (FMythicStoryCondition::Evaluate(Outcomes[i].When, OwnedTags)) {
                return i;
            }
        }
        return INDEX_NONE;
    }
};

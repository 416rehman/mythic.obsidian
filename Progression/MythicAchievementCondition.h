#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Templates/Function.h"
#include "Narrative/MythicStoryCondition.h"
#include "MythicAchievementCondition.generated.h"

USTRUCT(BlueprintType)
struct FMythicStatRequirement {
    GENERATED_BODY()

    // The Stat.* counter (or, when bHierarchical, the rollup PREFIX) this requirement reads.
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Achievement")
    FGameplayTag StatTag;

    // Minimum value the counter/rollup must reach for this requirement to be met (>= comparison). int64 to match the
    // ledger's counter width.
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Achievement")
    int64 MinValue = 0;

    // When true, read the hierarchical rollup under StatTag (GetCounterRollup / SumByPrefix); when false, the exact-tag
    // counter (GetCounter).
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Achievement")
    bool bHierarchical = false;
};

USTRUCT(BlueprintType)
struct FMythicAchievementCondition {
    GENERATED_BODY()

    // Tag clause (RequireAll / RequireAny / BlockAny over the player's owned tags). Reused verbatim from the narrative
    // gate so achievement gating and story gating share one evaluator.
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Achievement")
    FMythicStoryCondition TagCondition;

    // Every stat threshold here must be met (AND). Empty = no stat requirement.
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Achievement")
    TArray<FMythicStatRequirement> StatRequirements;

    static bool Evaluate(const FMythicAchievementCondition &Condition, const FGameplayTagContainer &OwnedTags,
                         TFunctionRef<int64(FGameplayTag, bool)> StatLookup) {
        if (!FMythicStoryCondition::Evaluate(Condition.TagCondition, OwnedTags)) {
            return false;
        }
        for (const FMythicStatRequirement &Req : Condition.StatRequirements) {
            if (!Req.StatTag.IsValid()) {
                continue;
            }
            const int64 Value = StatLookup(Req.StatTag, Req.bHierarchical);
            if (Value < Req.MinValue) {
                return false;
            }
        }
        return true;
    }
};

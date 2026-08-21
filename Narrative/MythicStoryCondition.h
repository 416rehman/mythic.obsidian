
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MythicStoryCondition.generated.h"

USTRUCT(BlueprintType)
struct FMythicStoryCondition {
    GENERATED_BODY()

    // Every tag here must be present in the owned set (hierarchical HasTag). Empty = no requirement.
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Narrative")
    FGameplayTagContainer RequireAll;

    // At least one tag here must be present in the owned set. Empty = no requirement (passes).
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Narrative")
    FGameplayTagContainer RequireAny;

    // If ANY tag here is present in the owned set, the condition fails (a mutually-exclusive route already taken).
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Narrative")
    FGameplayTagContainer BlockAny;

    bool IsEmpty() const {
        return RequireAll.IsEmpty() && RequireAny.IsEmpty() && BlockAny.IsEmpty();
    }

    static bool Evaluate(const FMythicStoryCondition &Condition, const FGameplayTagContainer &Owned) {
        if (!Condition.RequireAll.IsEmpty() && !Owned.HasAll(Condition.RequireAll)) {
            return false;
        }
        if (!Condition.RequireAny.IsEmpty() && !Owned.HasAny(Condition.RequireAny)) {
            return false;
        }
        if (!Condition.BlockAny.IsEmpty() && Owned.HasAny(Condition.BlockAny)) {
            return false;
        }
        return true;
    }
};

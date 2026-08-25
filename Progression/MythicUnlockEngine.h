#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Containers/ArrayView.h"
#include "Narrative/MythicStoryCondition.h"
#include "MythicUnlockEngine.generated.h"

UENUM(BlueprintType)
enum class EMythicUnlockEffect : uint8 {
    GrantTitle,
    GrantCosmetic,
    UnlockRecipe,
    UnlockPerk,
    UnlockSkill,
    UnlockFastTravel,
    GrantReward,
    GrantPerkSlot,
    GrantSkillSlot,
    GrantSkillModifierSlot
};

struct FMythicUnlockEngine {
    static void CollectNewlySatisfied(TConstArrayView<FMythicStoryCondition> RulePreconds, const FGameplayTagContainer &Owned,
                                      const TSet<int32> &AlreadyApplied, TArray<int32> &OutFire) {
        for (int32 i = 0; i < RulePreconds.Num(); ++i) {
            if (AlreadyApplied.Contains(i)) {
                continue;
            }
            if (FMythicStoryCondition::Evaluate(RulePreconds[i], Owned)) {
                OutFire.Add(i);
            }
        }
    }
};

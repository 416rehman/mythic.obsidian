#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "World/Camping/MythicTags_Camping.h"

namespace MythicCampsite {
inline float ReadRestedXpMultiplier(const UAbilitySystemComponent *ASC) {
    if (!ASC || !ASC->HasMatchingGameplayTag(TAG_Status_Rested)) {
        return 1.0f;
    }
    const FGameplayEffectQuery Query =
        FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(TAG_Status_Rested));
    float Best = 1.0f;
    for (const FActiveGameplayEffectHandle &Handle : ASC->GetActiveEffects(Query)) {
        if (const FActiveGameplayEffect *Active = ASC->GetActiveGameplayEffect(Handle)) {
            Best = FMath::Max(Best, Active->Spec.GetSetByCallerMagnitude(TAG_Data_Camping_RestedXpMult,
 false, 1.0f));
        }
    }
    return FMath::Max(1.0f, Best);
}
}

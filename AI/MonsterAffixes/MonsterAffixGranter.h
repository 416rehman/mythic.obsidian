
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "ActiveGameplayEffectHandle.h"
#include "Containers/ArrayView.h"

class UAbilitySystemComponent;
class UMonsterAffixPool;

struct MYTHIC_API FMonsterAffixGrantHandles {
    TArray<FGameplayAbilitySpecHandle> AbilityHandles;
    TArray<FActiveGameplayEffectHandle> EffectHandles;

    bool IsEmpty() const { return AbilityHandles.Num() == 0 && EffectHandles.Num() == 0; }
    void Reset() { AbilityHandles.Reset(); EffectHandles.Reset(); }
};

struct MYTHIC_API FMonsterAffixGranter {
    static FMonsterAffixGrantHandles GrantMonsterAffixes(UAbilitySystemComponent *NpcASC,
                                                         TConstArrayView<FGameplayTag> AffixTags,
                                                         const UMonsterAffixPool *PoolAsset = nullptr);

    static void RemoveMonsterAffixes(UAbilitySystemComponent *NpcASC, FMonsterAffixGrantHandles &Handles);
};

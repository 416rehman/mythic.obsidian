#pragma once
#include "CoreMinimal.h"
class UItemDefinition;
class UItemFragment;
struct FGameplayAbilityActorInfo;
namespace MythicWeaponAction {
    /** Packaged-runtime shape validation is also called by editor validation and authority item grants. */
    MYTHIC_API bool ValidateDefinition(const UItemDefinition* Definition,FText* Error=nullptr);
    MYTHIC_API bool MatchesDefinition(const UItemFragment* Fragment);
    MYTHIC_API bool IsEquippedSource(const UItemFragment* Fragment,const FGameplayAbilityActorInfo* Info);
}

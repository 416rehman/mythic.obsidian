
#include "MythicAbilityCost_Stamina.h"

#include "MythicGameplayAbility.h"
#include "NativeGameplayTags.h"
#include "GAS/MythicTags_GAS.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Utility.h"
#include "GAS/AttributeSets/Shared/MythicLifeComponent.h"
#include "AbilitySystemComponent.h"

UMythicAbilityCost_Stamina::UMythicAbilityCost_Stamina() {
    Cost.SetValue(10.0f);
    FailureTag = NOTIFY_ABILITY_ACTIVATION_FAILED_COST;
}

bool UMythicAbilityCost_Stamina::CheckCost(const UMythicGameplayAbility *Ability, const FGameplayAbilitySpecHandle Handle,
                                           const FGameplayAbilityActorInfo *ActorInfo, FGameplayTagContainer *OptionalRelevantTags) const {
    if (!Ability || !ActorInfo) {
        return false;
    }

    const UMythicLifeComponent *Life = UMythicLifeComponent::FindHealthComponent(ActorInfo->AvatarActor.Get());
    if (!Life) {
        return false;
    }

    const int32 AbilityLevel = Ability->GetAbilityLevel(Handle, ActorInfo);
    const float RawCost = Cost.GetValueAtLevel(AbilityLevel);
    const bool bCanApplyCost = Life->CanSpendStamina(RawCost);

    if (!bCanApplyCost && OptionalRelevantTags && FailureTag.IsValid()) {
        OptionalRelevantTags->AddTag(FailureTag);
    }

    return bCanApplyCost;
}

void UMythicAbilityCost_Stamina::ApplyCost(const UMythicGameplayAbility *Ability, const FGameplayAbilitySpecHandle Handle,
                                           const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) {
    if (!ActorInfo || !ActorInfo->IsNetAuthority() || !Ability) {
        return;
    }

    const int32 AbilityLevel = Ability->GetAbilityLevel(Handle, ActorInfo);
    const float RawCost = Cost.GetValueAtLevel(AbilityLevel);

    if (UMythicLifeComponent *Life = UMythicLifeComponent::FindHealthComponent(ActorInfo->AvatarActor.Get())) {
        Life->TrySpendStamina(RawCost);
    }
}

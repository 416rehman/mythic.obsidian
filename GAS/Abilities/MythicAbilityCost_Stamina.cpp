// Mythic — Stamina ability cost

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

    // Resolve through the SAME LifeComponent ApplyCost spends through (FindHealthComponent on the avatar), so a checked
    // cost is always payable. Reading stamina via the ASC's Utility set here while ApplyCost spends via the LifeComponent
    // could otherwise disagree — a missing component would pass CheckCost but no-op ApplyCost (a free cast). No
    // component => can't pay.
    const UMythicLifeComponent *Life = UMythicLifeComponent::FindHealthComponent(ActorInfo->AvatarActor.Get());
    if (!Life) {
        return false;
    }

    const int32 AbilityLevel = Ability->GetAbilityLevel(Handle, ActorInfo);
    const float RawCost = Cost.GetValueAtLevel(AbilityLevel);
    const bool bCanApplyCost = Life->CanSpendStamina(RawCost); // single source of the reduction-aware affordability rule

    // Inform other systems why activation was blocked (e.g. an out-of-stamina UI cue).
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

    // TrySpendStamina applies StaminaCostReduction + the server-side deduction — the single source of the spend rule.
    if (UMythicLifeComponent *Life = UMythicLifeComponent::FindHealthComponent(ActorInfo->AvatarActor.Get())) {
        Life->TrySpendStamina(RawCost);
    }
}

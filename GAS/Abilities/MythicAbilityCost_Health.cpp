#include "MythicAbilityCost_Health.h"
#include "MythicGA_AbilityWindow.h"
#include "AbilitySystemComponent.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/MythicTags_GAS.h"
float UMythicAbilityCost_Health::ResolveFraction(const UMythicGameplayAbility* Ability) const {
    const auto* Window=Cast<UMythicGA_AbilityWindow>(Ability);
    const float Fraction=Window && FractionParameter.IsValid() ? Window->GetAbilityNumber(FractionParameter,MaxHealthFraction) : MaxHealthFraction;
    return FMath::IsFinite(Fraction) ? FMath::Clamp(Fraction,0.0f,0.95f) : 1.0f;
}
bool UMythicAbilityCost_Health::CheckCost(const UMythicGameplayAbility* Ability,FGameplayAbilitySpecHandle,const FGameplayAbilityActorInfo* Info,FGameplayTagContainer* Tags) const {
    const auto* ASC=Info ? Info->AbilitySystemComponent.Get() : nullptr;
    const auto* Life=ASC ? ASC->GetSet<UMythicAttributeSet_Life>() : nullptr;
    const float Cost=Life ? Life->GetMaxHealth()*ResolveFraction(Ability) : 0;
    const bool Valid=Life && FMath::IsFinite(Cost) && Life->GetHealth()>Cost && Life->GetHealth()>0;
    if (!Valid && Tags) Tags->AddTag(NOTIFY_ABILITY_ACTIVATION_FAILED_COST);
    return Valid;
}
void UMythicAbilityCost_Health::ApplyCost(const UMythicGameplayAbility* Ability,FGameplayAbilitySpecHandle,const FGameplayAbilityActorInfo* Info,FGameplayAbilityActivationInfo) {
    auto* ASC=Info && Info->IsNetAuthority() ? Info->AbilitySystemComponent.Get() : nullptr;
    const auto* Life=ASC ? ASC->GetSet<UMythicAttributeSet_Life>() : nullptr;
    if (Life) ASC->SetNumericAttributeBase(UMythicAttributeSet_Life::GetHealthAttribute(),FMath::Max(0.01f,Life->GetHealth()-Life->GetMaxHealth()*ResolveFraction(Ability)));
}

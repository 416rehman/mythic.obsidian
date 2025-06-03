#include "MythicAbilitySystemComponent.h"

void UMythicAbilitySystemComponent::GetAdditionalActivationTagRequirements(const FGameplayTagContainer &AbilityTags,
                                                                           FGameplayTagContainer &OutActivationRequired,
                                                                           FGameplayTagContainer &OutActivationBlocked) const {
    if (AbilityTagRelationshipMapping) {
        AbilityTagRelationshipMapping->GetRequiredAndBlockedActivationTags(AbilityTags, &OutActivationRequired, &OutActivationBlocked);
    }
}

void UMythicAbilitySystemComponent::BeginPlay() {
    Super::BeginPlay();
}

const TArray<UMythicAttributeSet *> &UMythicAbilitySystemComponent::GetAttributeSets() const {
    return reinterpret_cast<const TArray<UMythicAttributeSet*>&>(this->GetSpawnedAttributes());
}
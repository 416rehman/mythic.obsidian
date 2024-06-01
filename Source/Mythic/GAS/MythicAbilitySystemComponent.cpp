#include "MythicAbilitySystemComponent.h"

void UMythicAbilitySystemComponent::AddAttributeSet(UAttributeSet* AttributeSet) {
    this->AddSpawnedAttribute(AttributeSet);
}

void UMythicAbilitySystemComponent::RemoveAttributeSet(UAttributeSet *AttributeSet) {
    this->RemoveSpawnedAttribute(AttributeSet);
}

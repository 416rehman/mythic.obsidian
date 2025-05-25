#include "MythicAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"

void UMythicAttributeSet::GetAttributes(TArray<FGameplayAttribute> &OutAttributes) const {
    this->GetAttributesFromSetClass(this->GetClass(), OutAttributes);
}

void UMythicAttributeSet::PostAttributeChange(const FGameplayAttribute &Attribute, float OldValue, float NewValue) {
    Super::PostAttributeChange(Attribute, OldValue, NewValue);

    this->OnAttributeChanged.Broadcast(Attribute, OldValue, NewValue);
}

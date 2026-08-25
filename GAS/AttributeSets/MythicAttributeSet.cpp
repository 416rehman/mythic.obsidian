#include "MythicAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"

void UMythicAttributeSet::GetAttributes(TArray<FGameplayAttribute> &OutAttributes) const {
    this->GetAttributesFromSetClass(this->GetClass(), OutAttributes);
}

void UMythicAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    // Every DOREPLIFETIME in a derived set registers by FProperty::RepIndex, which the engine assigns lazily and
    // only from the replication path. A caller that reads lifetime props without going near that path - the
    // Gameplay Debugger does exactly this - finds every index still at 0, so the whole set collapses into one
    // entry, silently when the conditions agree and as an assert when they do not (#148).
    GetClass()->SetUpRuntimeReplicationData();

    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void UMythicAttributeSet::PostAttributeChange(const FGameplayAttribute &Attribute, float OldValue, float NewValue) {
    Super::PostAttributeChange(Attribute, OldValue, NewValue);

    this->OnAttributeChanged.Broadcast(Attribute, OldValue, NewValue);
}

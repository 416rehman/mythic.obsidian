#include "MythicAbilitySystemComponent.h"
#include "Engine/World.h"
#include "Player/MythicCharacter.h"

void UMythicAbilitySystemComponent::PublishAbilityPoseRemnant(const FMythicAbilityPoseRemnantEvent& Event,bool bCancel) {
    if (GetOwner() && GetOwner()->HasAuthority() && Event.HasValidKey())
        if (auto* Character=Cast<AMythicCharacter>(Event.Avatar)) Character->PublishAbilityPoseRemnant(Event,bCancel);
}

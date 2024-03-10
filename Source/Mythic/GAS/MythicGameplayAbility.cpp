// 


#include "MythicGameplayAbility.h"

#include "AbilitySystemComponent.h"

void UMythicGameplayAbility::OnAvatarSet(const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilitySpec &Spec) {
    Super::OnAvatarSet(ActorInfo, Spec);

    if (bIsPassive) {
        // Give the ability to the actor
        ActorInfo->AbilitySystemComponent.Get()->TryActivateAbility(Spec.Handle, true);
    }
}

// APCMythic * UMythicGameplayAbility::GetMythicPlayerControllerFromActorInfo() const {
    // TODO
    // return (CurrentActorInfo ? Cast<APCMythic>(CurrentActorInfo->PlayerController.Get()) : nullptr);
// }

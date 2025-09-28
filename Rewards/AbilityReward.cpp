// 


#include "AbilityReward.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Mythic.h"

bool UAbilityReward::Give(FRewardContext& Context) const {
    // Check if the context is an ability system component
    auto ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Context.PlayerController);
    checkf(ASC, TEXT("AbilitySystemComponent is null"));

    // Check if the ability is valid
    if (!Ability) {
        UE_LOG(Myth, Error, TEXT("Ability is not valid"));
        return false;
    }

    // Check if the ability is valid
    auto AbilityClass = this->Ability;
    checkf(AbilityClass, TEXT("Ability is null in %s"), *this->GetName())

    // Give the ability
    auto AbilitySpec = ASC->BuildAbilitySpecFromClass(AbilityClass);
    auto AbilitySpecHandle = ASC->GiveAbility(AbilitySpec);

    UE_LOG(Myth, Log, TEXT("Gave ability %s"), *AbilityClass->GetName());

    // If the ability should be activated, activate it
    if (this->Activate) {
        if (ASC->TryActivateAbility(AbilitySpecHandle)) {
            UE_LOG(Myth, Log, TEXT("Activated ability %s"), *AbilityClass->GetName());
        }
        else {
            UE_LOG(Myth, Error, TEXT("Failed to activate ability %s"), *AbilityClass->GetName());
        }
    }

    return true;
}

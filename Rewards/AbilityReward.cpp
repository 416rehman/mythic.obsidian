

#include "AbilityReward.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Mythic.h"
#include "GameFramework/PlayerController.h"

bool UAbilityReward::Give(FRewardContext &Context) const {
    auto ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Context.PlayerController);
    checkf(ASC, TEXT("AbilitySystemComponent is null"));

    if (!Ability) {
        UE_LOG(Myth, Error, TEXT("Ability is not valid"));
        return false;
    }

    auto AbilityClass = this->Ability;
    checkf(AbilityClass, TEXT("Ability is null in %s"), *this->GetName())

    if (ASC->FindAbilitySpecFromClass(AbilityClass)) {
        UE_LOG(Myth, Verbose, TEXT("Ability %s already granted; skipping duplicate grant"), *AbilityClass->GetName());
        return true;
    }

    auto AbilitySpec = ASC->BuildAbilitySpecFromClass(AbilityClass);
    auto AbilitySpecHandle = ASC->GiveAbility(AbilitySpec);

    UE_LOG(Myth, Log, TEXT("Gave ability %s"), *AbilityClass->GetName());

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

FText UAbilityReward::GetPreviewText() const {
    if (!Ability) {
        return FText::GetEmpty();
    }
    return FText::FromString(FString::Printf(TEXT("Ability: %s"), *Ability->GetName()));
}

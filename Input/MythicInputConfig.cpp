// Copyright Mythic Games. All Rights Reserved.

#include "MythicInputConfig.h"
#include "Mythic/Mythic.h"
#include "GameplayTagContainer.h"

UMythicInputConfig::UMythicInputConfig(const FObjectInitializer &ObjectInitializer) {}

const UInputAction *UMythicInputConfig::FindNativeInputActionForTag(const FGameplayTag &InputTag, bool bLogNotFound) const {
    for (const FMythicInputAction &Action : NativeInputActions) {
        if (Action.InputAction && (Action.InputTag == InputTag)) {
            return Action.InputAction;
        }
    }

    if (bLogNotFound) {
        UE_LOG(Myth, Error, TEXT("Can't find NativeInputAction for InputTag [%s] on InputConfig [%s]."), *InputTag.ToString(), *GetNameSafe(this));
    }

    return nullptr;
}

const UInputAction *UMythicInputConfig::FindAbilityInputActionForTag(const FGameplayTag &InputTag, bool bLogNotFound) const {
    for (const FMythicInputAction &Action : AbilityInputActions) {
        if (Action.InputAction && (Action.InputTag == InputTag)) {
            return Action.InputAction;
        }
    }

    if (bLogNotFound) {
        UE_LOG(Myth, Error, TEXT("Can't find AbilityInputAction for InputTag [%s] on InputConfig [%s]."), *InputTag.ToString(), *GetNameSafe(this));
    }

    return nullptr;
}

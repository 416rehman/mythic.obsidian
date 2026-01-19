// Copyright Mythic Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EnhancedInputComponent.h"
#include "MythicInputConfig.h"
#include "GameplayTagContainer.h"
#include "Mythic/Mythic.h"
#include "MythicInputComponent.generated.h"

class UEnhancedInputLocalPlayerSubsystem;
class UInputAction;
class UObject;


/**
 * UMythicInputComponent
 *
 *	Component used to manage input mappings and bindings using an input config data asset.
 */
UCLASS(Config = Input)
class MYTHIC_API UMythicInputComponent : public UEnhancedInputComponent {
    GENERATED_BODY()

public:
    UMythicInputComponent(const FObjectInitializer &ObjectInitializer);

    void AddInputMappings(const UMythicInputConfig *InputConfig, UEnhancedInputLocalPlayerSubsystem *InputSubsystem) const;
    void RemoveInputMappings(const UMythicInputConfig *InputConfig, UEnhancedInputLocalPlayerSubsystem *InputSubsystem) const;

    template <class UserClass, typename FuncType>
    void BindNativeAction(const UMythicInputConfig *InputConfig, const FGameplayTag &InputTag, ETriggerEvent TriggerEvent, UserClass *Object, FuncType Func,
                          bool bLogIfNotFound);

    template <class UserClass, typename PressedFuncType, typename ReleasedFuncType>
    void BindAbilityActions(const UMythicInputConfig *InputConfig, UserClass *Object, PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc,
                            TArray<uint32> &BindHandles);

    void RemoveBinds(TArray<uint32> &BindHandles);
};


template <class UserClass, typename FuncType>
void UMythicInputComponent::BindNativeAction(const UMythicInputConfig *InputConfig, const FGameplayTag &InputTag, ETriggerEvent TriggerEvent, UserClass *Object,
                                             FuncType Func, bool bLogIfNotFound) {
    check(InputConfig);
    if (const UInputAction *IA = InputConfig->FindNativeInputActionForTag(InputTag, bLogIfNotFound)) {
        BindAction(IA, TriggerEvent, Object, Func);
    }
}

template <class UserClass, typename PressedFuncType, typename ReleasedFuncType>
void UMythicInputComponent::BindAbilityActions(const UMythicInputConfig *InputConfig, UserClass *Object, PressedFuncType PressedFunc,
                                               ReleasedFuncType ReleasedFunc, TArray<uint32> &BindHandles) {
    check(InputConfig);

    for (const FMythicInputAction &Action : InputConfig->AbilityInputActions) {
        if (Action.InputAction && Action.InputTag.IsValid()) {
            if (PressedFunc) {
                BindHandles.Add(BindAction(Action.InputAction, ETriggerEvent::Triggered, Object, PressedFunc, Action.InputTag).GetHandle());
                UE_LOG(Myth, Log, TEXT("BindAbilityActions: Bound Action %s to Tag %s (Triggered)"), *GetNameSafe(Action.InputAction),
                       *Action.InputTag.ToString());
            }

            if (ReleasedFunc) {
                BindHandles.Add(BindAction(Action.InputAction, ETriggerEvent::Completed, Object, ReleasedFunc, Action.InputTag).GetHandle());
                UE_LOG(Myth, Log, TEXT("BindAbilityActions: Bound Action %s to Tag %s (Completed/Released)"), *GetNameSafe(Action.InputAction),
                       *Action.InputTag.ToString());
            }
        }
        else {
            UE_LOG(Myth, Warning, TEXT("BindAbilityActions: Invalid Action or Tag in Config. Action: %s, Tag: %s"), *GetNameSafe(Action.InputAction),
                   *Action.InputTag.ToString());
        }
    }
}

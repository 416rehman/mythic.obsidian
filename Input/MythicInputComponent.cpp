// Copyright Mythic Games. All Rights Reserved.

#include "MythicInputComponent.h"
#include "EnhancedInputSubsystems.h"

UMythicInputComponent::UMythicInputComponent(const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer) {}

void UMythicInputComponent::AddInputMappings(const UMythicInputConfig *InputConfig, UEnhancedInputLocalPlayerSubsystem *InputSubsystem) const {
    check(InputConfig);
    check(InputSubsystem);

    // Here we could implement the mapping context logic if the config had it, 
    // but looking at Lyra's file, it seems the mapping contexts are applied separately usually.
    // However, if we want to store MappingContexts in the config, we can add them there. 
    // For now, we assume this function might be expanded or unused if we manage context elsewhere.
}

void UMythicInputComponent::RemoveInputMappings(const UMythicInputConfig *InputConfig, UEnhancedInputLocalPlayerSubsystem *InputSubsystem) const {
    check(InputConfig);
    check(InputSubsystem);

    // Mirror AddInputMappings
}

void UMythicInputComponent::RemoveBinds(TArray<uint32> &BindHandles) {
    for (uint32 Handle : BindHandles) {
        RemoveBindingByHandle(Handle);
    }
    BindHandles.Reset();
}

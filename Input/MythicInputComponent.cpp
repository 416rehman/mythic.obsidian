// Copyright Mythic Games. All Rights Reserved.

#include "MythicInputComponent.h"
#include "EnhancedInputSubsystems.h"

UMythicInputComponent::UMythicInputComponent(const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer) {}

void UMythicInputComponent::AddInputMappings(const UMythicInputConfig *InputConfig, UEnhancedInputLocalPlayerSubsystem *InputSubsystem) const {
    check(InputConfig);
    check(InputSubsystem);
}

void UMythicInputComponent::RemoveInputMappings(const UMythicInputConfig *InputConfig, UEnhancedInputLocalPlayerSubsystem *InputSubsystem) const {
    check(InputConfig);
    check(InputSubsystem);
}

void UMythicInputComponent::RemoveBinds(TArray<uint32> &BindHandles) {
    for (uint32 Handle : BindHandles) {
        RemoveBindingByHandle(Handle);
    }
    BindHandles.Reset();
}

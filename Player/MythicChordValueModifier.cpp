#include "MythicChordValueModifier.h"
#include "EnhancedPlayerInput.h"
#include "InputAction.h"
#include "InputTriggers.h"
#include "Player/MythicPlayerController.h"
#include "Input/MythicControlsComponent.h"

FInputActionValue UMythicChordValueModifier::ModifyRaw_Implementation(const UEnhancedPlayerInput* PlayerInput,FInputActionValue Value,float DeltaTime) {
    const auto* PC=PlayerInput ? Cast<AMythicPlayerController>(PlayerInput->GetOuter()) : nullptr;
    // UE 5.8 camera bindings may poll raw values even when an action's chord blocked its Triggered event.
    // The higher-priority common context evaluates this configured chord before the camera context.
    if (!ChordAction || !PC) { Value*=0.0f; return Value; }
    const auto* Controls=PC->FindComponentByClass<UMythicControlsComponent>();
    // Dedicated camera-axis opt-in only. All ability/reticle gates retain normal blocked-input policy.
    if (bAllowDuringCameraAdjustment && Controls && Controls->IsCameraAdjustmentOpen()
        && !PC->IsLookInputIgnored() && !Controls->IsGameplayWheelOpen()) return Value;
    if (PC->IsGameplayAbilityInputBlocked()) { Value*=0.0f; return Value; }
    // UE 5.8 does not export FInputActionInstance's current-state getter. Reuse the
    // engine's stateless chord evaluator through its exported API; one probe per
    // mapping is retained instead of allocating or sampling last-frame events.
    if (!ChordProbe) ChordProbe=NewObject<UInputTriggerChordAction>(this);
    ChordProbe->ChordAction=ChordAction;
    if ((ChordProbe->UpdateState(PlayerInput,Value,DeltaTime)==ETriggerState::Triggered)!=bPassWhenChordActive) Value*=0.0f;
    return Value;
}

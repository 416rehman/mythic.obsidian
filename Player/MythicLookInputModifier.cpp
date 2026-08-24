// Copyright Stellar Games. All Rights Reserved.

#include "Player/MythicLookInputModifier.h"

#include "CommonInputSubsystem.h"
#include "CommonInputTypeEnum.h"
#include "EnhancedPlayerInput.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "UI/Settings/MythicUserSettings.h"

FInputActionValue UMythicLookInputModifier::ModifyRaw_Implementation(const UEnhancedPlayerInput *PlayerInput, FInputActionValue CurrentValue, float DeltaTime) {
    const UMythicUserSettings *Settings = UMythicUserSettings::Get();
    if (!Settings || CurrentValue.GetValueType() != EInputActionValueType::Axis2D) {
        return CurrentValue;
    }

    bool bGamepad = false;
    if (PlayerInput) {
        if (const APlayerController *PC = Cast<APlayerController>(PlayerInput->GetOuter())) {
            if (const ULocalPlayer *LP = PC->GetLocalPlayer()) {
                if (const UCommonInputSubsystem *Input = ULocalPlayer::GetSubsystem<UCommonInputSubsystem>(LP)) {
                    bGamepad = Input->GetCurrentInputType() == ECommonInputType::Gamepad;
                }
            }
        }
    }

    FVector2D Value = CurrentValue.Get<FVector2D>();

    if (bGamepad) {
        // A settings-driven radial deadzone, so the stick threshold is the player's, not the asset's. Rescaled
        // past the threshold so aiming keeps its full range instead of jumping where the deadzone ends.
        const float Deadzone = FMath::Clamp(Settings->GetGamepadDeadzoneRight(), 0.0f, 0.9f);
        const float Size = Value.Size();
        if (Size < Deadzone) {
            return FInputActionValue(FVector2D::ZeroVector);
        }
        Value *= (Size - Deadzone) / ((1.0f - Deadzone) * Size);
        Value *= Settings->GetGamepadLookSensitivity();
    }
    else {
        Value *= Settings->GetMouseLookSensitivity();
    }

    Value *= Settings->GetLookSensitivity();
    Value.Y *= Settings->GetVerticalLookScale();
    if (Settings->GetInvertLookY()) {
        Value.Y = -Value.Y;
    }
    if (Settings->GetInvertLookX()) {
        Value.X = -Value.X;
    }
    return FInputActionValue(Value);
}

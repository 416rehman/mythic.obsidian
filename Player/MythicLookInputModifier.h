// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InputModifiers.h"
#include "MythicLookInputModifier.generated.h"

/**
 * The one place the look-feel settings actually reach the stick and the mouse.
 *
 * Sensitivity, per-device sensitivity, vertical scale, invert X/Y and the gamepad deadzones were stored and
 * saved by the settings screen but consumed by nothing - the classic row that "does not take effect". Add this
 * modifier to the Look action's mappings and every one of those rows works, per device, with no per-Blueprint
 * wiring and no restart.
 */
UCLASS(meta = (DisplayName = "Mythic Look Settings"))
class MYTHIC_API UMythicLookInputModifier : public UInputModifier {
    GENERATED_BODY()

protected:
    virtual FInputActionValue ModifyRaw_Implementation(const UEnhancedPlayerInput *PlayerInput, FInputActionValue CurrentValue, float DeltaTime) override;
};

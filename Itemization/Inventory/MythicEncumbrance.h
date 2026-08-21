#pragma once

#include "CoreMinimal.h"
#include "MythicEncumbrance.generated.h"

UENUM(BlueprintType)
enum class EMythicEncumbranceTier : uint8 {
    Unencumbered,
    Heavy,
    Overloaded,
};

namespace MythicEncumbrance {
    MYTHIC_API EMythicEncumbranceTier ComputeTier(float TotalWeight, float SoftCapacity, float HardCapacity);

    MYTHIC_API float SpeedMultiplierForTier(EMythicEncumbranceTier Tier, float HeavyMult, float OverloadedMult);
}

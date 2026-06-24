#include "MythicEncumbrance.h"

namespace MythicEncumbrance {
    EMythicEncumbranceTier ComputeTier(float TotalWeight, float SoftCapacity, float HardCapacity) {
        if (HardCapacity > 0.0f && TotalWeight > HardCapacity) {
            return EMythicEncumbranceTier::Overloaded;
        }
        if (SoftCapacity > 0.0f && TotalWeight > SoftCapacity) {
            return EMythicEncumbranceTier::Heavy;
        }
        return EMythicEncumbranceTier::Unencumbered;
    }

    float SpeedMultiplierForTier(EMythicEncumbranceTier Tier, float HeavyMult, float OverloadedMult) {
        switch (Tier) {
        case EMythicEncumbranceTier::Heavy:
            return FMath::Clamp(HeavyMult, 0.0f, 1.0f);
        case EMythicEncumbranceTier::Overloaded:
            return FMath::Clamp(OverloadedMult, 0.0f, 1.0f);
        default:
            return 1.0f;
        }
    }
}

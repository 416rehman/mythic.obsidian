
#include "World/LivingWorld/Territory/MythicDanger.h"

int32 FMythicDanger::ChebyshevDistance(const FMythicCellCoord& A, const FMythicCellCoord& B) {
    return FMath::Max(FMath::Abs(A.X - B.X), FMath::Abs(A.Y - B.Y));
}

EMythicDangerTier FMythicDanger::ComputeDangerTier(int32 DistanceCells, float MilitaryStrength, bool bHasCapital,
                                                   const FMythicDangerTierParams& Params) {
    if (bHasCapital) {
        return EMythicDangerTier::Safe;
    }

    int32 Tier = 0;
    if (DistanceCells >= Params.ExtremeTierDistance) {
        Tier = static_cast<int32>(EMythicDangerTier::Extreme);
    }
    else if (DistanceCells >= Params.HighTierDistance) {
        Tier = static_cast<int32>(EMythicDangerTier::High);
    }
    else if (DistanceCells >= Params.ModerateTierDistance) {
        Tier = static_cast<int32>(EMythicDangerTier::Moderate);
    }
    else if (DistanceCells >= Params.LowTierDistance) {
        Tier = static_cast<int32>(EMythicDangerTier::Low);
    }

    const float ClampedStrength = FMath::Clamp(MilitaryStrength, 0.0f, 1.0f);
    Tier += FMath::RoundToInt(ClampedStrength * FMath::Max(0.0f, Params.StrengthTierBoost));

    const int32 MaxTier = static_cast<int32>(EMythicDangerTier::COUNT) - 1;
    Tier = FMath::Clamp(Tier, 0, MaxTier);
    return static_cast<EMythicDangerTier>(Tier);
}

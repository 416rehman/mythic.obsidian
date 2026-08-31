#include "GAS/Combat/MythicCombatThreatAssessment.h"

namespace {
int32 BandSeverity(const EMythicThreatBand Band) {
    switch (Band) {
    case EMythicThreatBand::Overwhelming:
        return 4;
    case EMythicThreatBand::Deadly:
        return 3;
    case EMythicThreatBand::Risky:
        return 2;
    case EMythicThreatBand::None:
        return 1;
    case EMythicThreatBand::Unknown:
    default:
        return 0;
    }
}

EMythicThreatBand MaxKnownBand(const EMythicThreatBand Left, const EMythicThreatBand Right) {
    return BandSeverity(Left) >= BandSeverity(Right) ? Left : Right;
}

FMythicCombatThreatThresholds SanitizeThresholds(const FMythicCombatThreatThresholds &In) {
    FMythicCombatThreatThresholds Out;
    Out.RiskyPressureRatio = FMath::IsFinite(In.RiskyPressureRatio)
        ? FMath::Max(1.0f, In.RiskyPressureRatio)
        : Out.RiskyPressureRatio;
    Out.DeadlyPressureRatio = FMath::IsFinite(In.DeadlyPressureRatio)
        ? FMath::Max(Out.RiskyPressureRatio, In.DeadlyPressureRatio)
        : FMath::Max(Out.RiskyPressureRatio, Out.DeadlyPressureRatio);
    Out.OverwhelmingPressureRatio = FMath::IsFinite(In.OverwhelmingPressureRatio)
        ? FMath::Max(Out.DeadlyPressureRatio, In.OverwhelmingPressureRatio)
        : FMath::Max(Out.DeadlyPressureRatio, Out.OverwhelmingPressureRatio);
    return Out;
}
}

EMythicThreatBand FMythicCombatThreatAssessment::RankFloor(const EMythicCombatThreatRank Rank,
                                                            const bool bRankKnownToViewer) {
    if (!bRankKnownToViewer) {
        return EMythicThreatBand::None;
    }
    switch (Rank) {
    case EMythicCombatThreatRank::Elite:
        return EMythicThreatBand::Risky;
    case EMythicCombatThreatRank::Boss:
        return EMythicThreatBand::Deadly;
    case EMythicCombatThreatRank::WorldBoss:
        return EMythicThreatBand::Overwhelming;
    case EMythicCombatThreatRank::NonCombatant:
    case EMythicCombatThreatRank::Standard:
    default:
        return EMythicThreatBand::None;
    }
}

EMythicThreatBand FMythicCombatThreatAssessment::Assess(
    const FMythicCombatThreatAssessmentInputs &Inputs,
    const FMythicCombatThreatThresholds &Thresholds) {
    if (!Inputs.bAssessmentPermitted) {
        return EMythicThreatBand::Unknown;
    }
    if (!Inputs.bCombatCapable
        || (Inputs.bRankKnownToViewer && Inputs.Rank == EMythicCombatThreatRank::NonCombatant)) {
        return EMythicThreatBand::None;
    }
    if ((Inputs.bDamageabilityKnownToViewer && !Inputs.bDamageable)
        || (Inputs.bImmunityKnownToViewer && Inputs.bImmuneToViewerDamage)) {
        return EMythicThreatBand::Overwhelming;
    }

    const EMythicThreatBand Floor = RankFloor(Inputs.Rank, Inputs.bRankKnownToViewer);
    if (!FMath::IsFinite(Inputs.ViewerEffectivePressure) || Inputs.ViewerEffectivePressure <= UE_SMALL_NUMBER
        || !FMath::IsFinite(Inputs.SubjectEffectivePressure) || Inputs.SubjectEffectivePressure < 0.0f) {
        return Floor == EMythicThreatBand::None ? EMythicThreatBand::Unknown : Floor;
    }

    const FMythicCombatThreatThresholds Safe = SanitizeThresholds(Thresholds);
    const double Ratio = Inputs.SubjectEffectivePressure / Inputs.ViewerEffectivePressure;
    EMythicThreatBand PressureBand = EMythicThreatBand::None;
    if (Ratio >= Safe.OverwhelmingPressureRatio) {
        PressureBand = EMythicThreatBand::Overwhelming;
    }
    else if (Ratio >= Safe.DeadlyPressureRatio) {
        PressureBand = EMythicThreatBand::Deadly;
    }
    else if (Ratio >= Safe.RiskyPressureRatio) {
        PressureBand = EMythicThreatBand::Risky;
    }
    return MaxKnownBand(PressureBand, Floor);
}

#include "Interaction/Attention/MythicEntityAttentionRules.h"

FMythicEntityAttentionConfig FMythicEntityAttentionRules::SanitizeConfig(
    const FMythicEntityAttentionConfig &Config) {
    FMythicEntityAttentionConfig Result = Config;
    const FMythicEntityAttentionConfig Defaults;
#define MYTHIC_ATTENTION_FINITE_OR_DEFAULT(Field) \
    if (!FMath::IsFinite(Result.Field)) {          \
        Result.Field = Defaults.Field;             \
    }
    MYTHIC_ATTENTION_FINITE_OR_DEFAULT(DecisionRateHz);
    MYTHIC_ATTENTION_FINITE_OR_DEFAULT(MinimumImmediatePassIntervalSeconds);
    MYTHIC_ATTENTION_FINITE_OR_DEFAULT(MaximumAttentionDistanceCentimeters);
    MYTHIC_ATTENTION_FINITE_OR_DEFAULT(AmbientPersonalSpaceDistanceCentimeters);
    MYTHIC_ATTENTION_FINITE_OR_DEFAULT(GazeProbeRadiusCentimeters);
    MYTHIC_ATTENTION_FINITE_OR_DEFAULT(GazeMinimumViewDot);
    MYTHIC_ATTENTION_FINITE_OR_DEFAULT(GazeReleaseMinimumViewDot);
    MYTHIC_ATTENTION_FINITE_OR_DEFAULT(FocusMinimumViewDot);
    MYTHIC_ATTENTION_FINITE_OR_DEFAULT(FocusAcquireDwellSeconds);
    MYTHIC_ATTENTION_FINITE_OR_DEFAULT(WhisperAcquireDwellSeconds);
    MYTHIC_ATTENTION_FINITE_OR_DEFAULT(FocusReleaseGraceSeconds);
    MYTHIC_ATTENTION_FINITE_OR_DEFAULT(ReplacementDwellSeconds);
    MYTHIC_ATTENTION_FINITE_OR_DEFAULT(ReplacementScoreMultiplier);
    MYTHIC_ATTENTION_FINITE_OR_DEFAULT(SlotReleaseGraceSeconds);
    MYTHIC_ATTENTION_FINITE_OR_DEFAULT(DistanceDeadbandCentimeters);
    MYTHIC_ATTENTION_FINITE_OR_DEFAULT(LineOfSightCacheLifetimeSeconds);
    MYTHIC_ATTENTION_FINITE_OR_DEFAULT(LineOfSightMovementInvalidationCentimeters);
    MYTHIC_ATTENTION_FINITE_OR_DEFAULT(OcclusionHideGraceSeconds);
    MYTHIC_ATTENTION_FINITE_OR_DEFAULT(OcclusionRevealGraceSeconds);
    MYTHIC_ATTENTION_FINITE_OR_DEFAULT(ScreenEdgePaddingPixels);
    MYTHIC_ATTENTION_FINITE_OR_DEFAULT(ViewAlignmentWeight);
    MYTHIC_ATTENTION_FINITE_OR_DEFAULT(ProximityWeight);
    MYTHIC_ATTENTION_FINITE_OR_DEFAULT(OnScreenBonus);
    MYTHIC_ATTENTION_FINITE_OR_DEFAULT(LineOfSightBonus);
    MYTHIC_ATTENTION_FINITE_OR_DEFAULT(AwarenessPriorityBonus);
    MYTHIC_ATTENTION_FINITE_OR_DEFAULT(OpportunityPriorityBonus);
    MYTHIC_ATTENTION_FINITE_OR_DEFAULT(SafetyPriorityBonus);
    MYTHIC_ATTENTION_FINITE_OR_DEFAULT(InteractionOverrideBonus);
    MYTHIC_ATTENTION_FINITE_OR_DEFAULT(HardTargetOverrideBonus);
    MYTHIC_ATTENTION_FINITE_OR_DEFAULT(InspectOverrideBonus);
#undef MYTHIC_ATTENTION_FINITE_OR_DEFAULT

    Result.DecisionRateHz = FMath::Clamp(Result.DecisionRateHz, 1.0f, 10.0f);
    Result.MinimumImmediatePassIntervalSeconds =
        FMath::Clamp(Result.MinimumImmediatePassIntervalSeconds, 0.0f, 0.1f);
    Result.MaxPublishedObservations = FMath::Clamp(Result.MaxPublishedObservations, 1, 16);
    Result.MaxEvaluatedCandidates =
        FMath::Clamp(Result.MaxEvaluatedCandidates, Result.MaxPublishedObservations, 128);
    Result.MaxLineOfSightTracesPerPass =
        FMath::Clamp(Result.MaxLineOfSightTracesPerPass, 1, 8);
    Result.MaxSpatialCandidatesPerPass =
        FMath::Clamp(Result.MaxSpatialCandidatesPerPass, 8, 256);
    Result.MaxRetainedEventSubjects =
        FMath::Clamp(Result.MaxRetainedEventSubjects, 8, 128);
    Result.MaxRegistryFallbackChecksPerPass =
        FMath::Clamp(Result.MaxRegistryFallbackChecksPerPass, 0, 64);
    Result.MaximumAttentionDistanceCentimeters =
        FMath::Max(100.0f, Result.MaximumAttentionDistanceCentimeters);
    Result.AmbientPersonalSpaceDistanceCentimeters = FMath::Clamp(
        Result.AmbientPersonalSpaceDistanceCentimeters, 0.0f,
        Result.MaximumAttentionDistanceCentimeters);
    Result.GazeProbeRadiusCentimeters =
        FMath::Clamp(Result.GazeProbeRadiusCentimeters, 1.0f, 300.0f);
    Result.GazeMinimumViewDot = FMath::Clamp(Result.GazeMinimumViewDot, -1.0f, 1.0f);
    Result.GazeReleaseMinimumViewDot = FMath::Clamp(
        Result.GazeReleaseMinimumViewDot, -1.0f, Result.GazeMinimumViewDot);
    Result.FocusMinimumViewDot = FMath::Clamp(
        Result.FocusMinimumViewDot, Result.GazeMinimumViewDot, 1.0f);
    Result.FocusAcquireDwellSeconds = FMath::Max(0.0f, Result.FocusAcquireDwellSeconds);
    Result.WhisperAcquireDwellSeconds = FMath::Max(0.0f, Result.WhisperAcquireDwellSeconds);
    Result.FocusReleaseGraceSeconds = FMath::Max(0.0f, Result.FocusReleaseGraceSeconds);
    Result.ReplacementDwellSeconds = FMath::Max(0.0f, Result.ReplacementDwellSeconds);
    Result.ReplacementScoreMultiplier = FMath::Max(1.0f, Result.ReplacementScoreMultiplier);
    Result.SlotReleaseGraceSeconds = FMath::Max(0.0f, Result.SlotReleaseGraceSeconds);
    Result.DistanceDeadbandCentimeters = FMath::Max(0.0f, Result.DistanceDeadbandCentimeters);
    Result.LineOfSightCacheLifetimeSeconds = FMath::Max(0.0f, Result.LineOfSightCacheLifetimeSeconds);
    Result.LineOfSightMovementInvalidationCentimeters =
        FMath::Max(0.0f, Result.LineOfSightMovementInvalidationCentimeters);
    Result.OcclusionHideGraceSeconds = FMath::Max(0.0f, Result.OcclusionHideGraceSeconds);
    Result.OcclusionRevealGraceSeconds = FMath::Max(0.0f, Result.OcclusionRevealGraceSeconds);
    Result.MaxCriticalEdgeIndicators = FMath::Clamp(Result.MaxCriticalEdgeIndicators, 0, 8);
    Result.ScreenEdgePaddingPixels = FMath::Max(0.0f, Result.ScreenEdgePaddingPixels);
    Result.ViewAlignmentWeight = FMath::Max(0.0f, Result.ViewAlignmentWeight);
    Result.ProximityWeight = FMath::Max(0.0f, Result.ProximityWeight);
    Result.OnScreenBonus = FMath::Max(0.0f, Result.OnScreenBonus);
    Result.LineOfSightBonus = FMath::Max(0.0f, Result.LineOfSightBonus);
    Result.AwarenessPriorityBonus = FMath::Max(0.0f, Result.AwarenessPriorityBonus);
    Result.OpportunityPriorityBonus = FMath::Max(
        Result.AwarenessPriorityBonus, Result.OpportunityPriorityBonus);
    Result.SafetyPriorityBonus = FMath::Max(
        Result.OpportunityPriorityBonus, Result.SafetyPriorityBonus);
    Result.InteractionOverrideBonus = FMath::Max(
        Result.SafetyPriorityBonus, Result.InteractionOverrideBonus);
    Result.HardTargetOverrideBonus = FMath::Max(
        Result.InteractionOverrideBonus, Result.HardTargetOverrideBonus);
    Result.InspectOverrideBonus = FMath::Max(
        Result.HardTargetOverrideBonus, Result.InspectOverrideBonus);
    return Result;
}

float FMythicEntityAttentionRules::GetGazeMinimumViewDot(
    const bool bRetainingRecentGaze,
    const FMythicEntityAttentionConfig &Config) {
    return bRetainingRecentGaze
        ? FMath::Clamp(Config.GazeReleaseMinimumViewDot, -1.0f,
                       Config.GazeMinimumViewDot)
        : FMath::Clamp(Config.GazeMinimumViewDot, -1.0f, 1.0f);
}

bool FMythicEntityAttentionRules::UpdateStableLineOfSight(
    FMythicEntityAttentionVisibilityState &State,
    const bool bRawHasLineOfSight, const double NowSeconds,
    const FMythicEntityAttentionConfig &Config) {
    if (!FMath::IsFinite(NowSeconds)) {
        return State.bInitialized && State.bStableHasLineOfSight;
    }

    if (!State.bInitialized) {
        State.bInitialized = true;
        State.bRawHasLineOfSight = bRawHasLineOfSight;
        State.bStableHasLineOfSight = bRawHasLineOfSight;
        State.RawStateSinceSeconds = NowSeconds;
        return State.bStableHasLineOfSight;
    }

    if (State.bRawHasLineOfSight != bRawHasLineOfSight) {
        State.bRawHasLineOfSight = bRawHasLineOfSight;
        State.RawStateSinceSeconds = NowSeconds;
    }

    const double RequiredSeconds = bRawHasLineOfSight
        ? FMath::Max(0.0f, Config.OcclusionRevealGraceSeconds)
        : FMath::Max(0.0f, Config.OcclusionHideGraceSeconds);
    if (FMath::Max(0.0, NowSeconds - State.RawStateSinceSeconds)
        >= RequiredSeconds) {
        State.bStableHasLineOfSight = bRawHasLineOfSight;
    }
    return State.bStableHasLineOfSight;
}

bool FMythicEntityAttentionRules::ShouldPreserveDeferredLineOfSight(
    const FMythicEntityAttentionVisibilityState &State,
    const double SecondsSinceSample,
    const FMythicEntityAttentionConfig &Config) {
    if (!State.bInitialized || !State.bStableHasLineOfSight
        || !FMath::IsFinite(SecondsSinceSample)
        || SecondsSinceSample < 0.0) {
        return false;
    }
    const double MaximumDeferredSeconds =
        FMath::Max(0.0f, Config.LineOfSightCacheLifetimeSeconds)
        + FMath::Max(0.0f, Config.OcclusionHideGraceSeconds);
    return SecondsSinceSample <= MaximumDeferredSeconds;
}

bool FMythicEntityAttentionRules::CanRetainRecentGaze(
    const double SecondsSinceEligible,
    const FMythicEntityAttentionConfig &Config) {
    return FMath::IsFinite(SecondsSinceEligible)
        && SecondsSinceEligible >= 0.0
        && SecondsSinceEligible
               <= FMath::Max(0.0f, Config.SlotReleaseGraceSeconds);
}

float FMythicEntityAttentionRules::CalculateScore(
    const FMythicEntityAttentionScoreInput &Input,
    const FMythicEntityAttentionConfig &Config) {
    const float SafeAlignment = FMath::IsFinite(Input.ViewAlignment)
        ? Input.ViewAlignment
        : -1.0f;
    const float SafeDistance = FMath::IsFinite(Input.DistanceCentimeters)
        ? FMath::Max(0.0f, Input.DistanceCentimeters)
        : Config.MaximumAttentionDistanceCentimeters;
    const float NormalizedAlignment = FMath::Clamp((SafeAlignment + 1.0f) * 0.5f, 0.0f, 1.0f);
    const float NormalizedProximity = 1.0f - FMath::Clamp(
        SafeDistance / FMath::Max(1.0f, Config.MaximumAttentionDistanceCentimeters), 0.0f, 1.0f);

    float PriorityBonus = 0.0f;
    switch (Input.PriorityClass) {
    case EMythicEntityAttentionPriorityClass::Safety:
        PriorityBonus = Config.SafetyPriorityBonus;
        break;
    case EMythicEntityAttentionPriorityClass::Opportunity:
        PriorityBonus = Config.OpportunityPriorityBonus;
        break;
    case EMythicEntityAttentionPriorityClass::Awareness:
        PriorityBonus = Config.AwarenessPriorityBonus;
        break;
    case EMythicEntityAttentionPriorityClass::Ambient:
    default:
        break;
    }

    const float Strength = FMath::IsFinite(Input.SignalStrength)
        ? FMath::Clamp(Input.SignalStrength, 0.0f, 1.0f)
        : 0.0f;
    float Score = Config.ViewAlignmentWeight * NormalizedAlignment
                  + Config.ProximityWeight * NormalizedProximity
                  + (Input.bOnScreen ? Config.OnScreenBonus : 0.0f)
                  + (Input.bHasLineOfSight ? Config.LineOfSightBonus : 0.0f)
                  + PriorityBonus * Strength;

    if (Input.bInteractionTarget) {
        Score += Config.InteractionOverrideBonus;
    }
    if (Input.bHardTarget) {
        Score += Config.HardTargetOverrideBonus;
    }
    if (Input.bInspectTarget) {
        Score += Config.InspectOverrideBonus;
    }
    return FMath::Max(0.0f, Score);
}

bool FMythicEntityAttentionRules::IsForcedFocus(
    const FMythicEntityAttentionScoreInput &Input) {
    return Input.bInspectTarget || Input.bHardTarget || Input.bInteractionTarget;
}

bool FMythicEntityAttentionRules::CanAcquireFocus(
    const float StableSeconds, const bool bForced,
    const FMythicEntityAttentionConfig &Config) {
    return bForced || StableSeconds >= Config.FocusAcquireDwellSeconds;
}

bool FMythicEntityAttentionRules::CanRetainFocus(
    const float SecondsSinceEligible,
    const FMythicEntityAttentionConfig &Config) {
    return SecondsSinceEligible <= Config.FocusReleaseGraceSeconds;
}

bool FMythicEntityAttentionRules::ShouldReplaceFocus(
    const float IncumbentScore, const float ChallengerScore,
    const float ChallengerStableSeconds, const bool bForced,
    const FMythicEntityAttentionConfig &Config) {
    if (bForced) {
        return true;
    }

    if (!FMath::IsFinite(IncumbentScore)
        || !FMath::IsFinite(ChallengerScore)
        || !FMath::IsFinite(ChallengerStableSeconds)) {
        return false;
    }

    return ChallengerScore
               >= FMath::Max(0.0f, IncumbentScore)
                      * Config.ReplacementScoreMultiplier
           && ChallengerStableSeconds >= Config.ReplacementDwellSeconds;
}

int32 FMythicEntityAttentionRules::GetPriorityRank(
    const EMythicEntityAttentionPriorityClass PriorityClass) {
    switch (PriorityClass) {
    case EMythicEntityAttentionPriorityClass::Safety:
        return 3;
    case EMythicEntityAttentionPriorityClass::Opportunity:
        return 2;
    case EMythicEntityAttentionPriorityClass::Awareness:
        return 1;
    case EMythicEntityAttentionPriorityClass::Ambient:
    default:
        return 0;
    }
}

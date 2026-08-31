#include "UI/Nameplate/MythicNameplatePolicy.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"

#define LOCTEXT_NAMESPACE "MythicNameplatePolicy"

EDataValidationResult UMythicNameplatePolicy::IsDataValid(FDataValidationContext &Context) const {
    EDataValidationResult Result = Super::IsDataValid(Context);
    auto AddError = [&Context, &Result](const FText &Message) {
        Context.AddError(Message);
        Result = EDataValidationResult::Invalid;
    };

    const int64 LaneCapacity = static_cast<int64>(Capacity.FocusLaneSlots)
        + static_cast<int64>(Capacity.SafetyLaneSlots)
        + static_cast<int64>(Capacity.OpportunityLaneSlots)
        + static_cast<int64>(Capacity.AwarenessLaneSlots);
    if (Capacity.FocusLaneSlots != 1) {
        AddError(LOCTEXT("FocusCapacity", "Contextual nameplates require exactly one Focus-lane slot."));
    }
    if (Capacity.PoolSize < 1 || Capacity.MaxDrawnPlates < 1 || Capacity.FadeReserveSlots < 0
        || Capacity.SafetyLaneSlots < 0 || Capacity.OpportunityLaneSlots < 0 || Capacity.AwarenessLaneSlots < 0) {
        AddError(LOCTEXT("NegativeCapacity", "Nameplate pool, draw, reserve, and lane capacities must be nonnegative and usable."));
    }
    if (LaneCapacity != Capacity.MaxDrawnPlates) {
        AddError(LOCTEXT("LaneCapacityMismatch", "Focus, Safety, Opportunity, and Awareness lane slots must sum to Max Drawn Plates."));
    }
    if (static_cast<int64>(Capacity.MaxDrawnPlates) + static_cast<int64>(Capacity.FadeReserveSlots)
        > static_cast<int64>(Capacity.PoolSize)) {
        AddError(LOCTEXT("PoolCapacity", "Pool Size must cover Max Drawn Plates plus Fade Reserve Slots."));
    }

    if (!FMath::IsFinite(Declutter.CollisionPaddingPixels)
        || !FMath::IsFinite(Declutter.ReleaseHysteresisPixels)
        || !FMath::IsFinite(Declutter.VerticalStepPixels)
        || Declutter.CollisionPaddingPixels < 0.0f
        || Declutter.CollisionPaddingPixels > 32.0f
        || Declutter.ReleaseHysteresisPixels < 0.0f
        || Declutter.ReleaseHysteresisPixels > 48.0f
        || Declutter.VerticalStepPixels < 4.0f
        || Declutter.VerticalStepPixels > 64.0f
        || Declutter.MaxVerticalSteps < 0
        || Declutter.MaxVerticalSteps > 4) {
        AddError(LOCTEXT(
            "Declutter",
            "Nameplate declutter values must remain finite and inside their bounded padding, hysteresis, step, and attempt ranges."));
    }

    const float AttentionScalars[] = {
        Attention.DecisionRateHz,
        Attention.MinimumImmediatePassIntervalSeconds,
        Attention.MaximumAttentionDistanceCentimeters,
        Attention.AmbientPersonalSpaceDistanceCentimeters,
        Attention.GazeMinimumViewDot,
        Attention.GazeReleaseMinimumViewDot,
        Attention.FocusMinimumViewDot,
        Attention.FocusAcquireDwellSeconds,
        Attention.WhisperAcquireDwellSeconds,
        Attention.FocusReleaseGraceSeconds,
        Attention.ReplacementDwellSeconds,
        Attention.ReplacementScoreMultiplier,
        Attention.SlotReleaseGraceSeconds,
        Attention.DistanceDeadbandCentimeters,
        Attention.LineOfSightCacheLifetimeSeconds,
        Attention.LineOfSightMovementInvalidationCentimeters,
        Attention.OcclusionHideGraceSeconds,
        Attention.OcclusionRevealGraceSeconds,
        Attention.ScreenEdgePaddingPixels,
        Attention.ViewAlignmentWeight,
        Attention.ProximityWeight,
        Attention.OnScreenBonus,
        Attention.LineOfSightBonus,
        Attention.AwarenessPriorityBonus,
        Attention.OpportunityPriorityBonus,
        Attention.SafetyPriorityBonus,
        Attention.InteractionOverrideBonus,
        Attention.HardTargetOverrideBonus,
        Attention.InspectOverrideBonus,
    };
    bool bAttentionFinite = true;
    for (const float Value : AttentionScalars) {
        bAttentionFinite &= FMath::IsFinite(Value);
    }

    const bool bAttentionRangesValid = Attention.DecisionRateHz >= 1.0f
        && Attention.DecisionRateHz <= 10.0f
        && Attention.MinimumImmediatePassIntervalSeconds >= 0.0f
        && Attention.MinimumImmediatePassIntervalSeconds <= 0.1f
        && Attention.MaxPublishedObservations >= Capacity.MaxDrawnPlates
        && Attention.MaxPublishedObservations <= Capacity.PoolSize
        && Attention.MaxPublishedObservations <= 16
        && Attention.MaxEvaluatedCandidates >= Attention.MaxPublishedObservations
        && Attention.MaxEvaluatedCandidates <= 128
        && Attention.MaxLineOfSightTracesPerPass >= 1
        && Attention.MaxLineOfSightTracesPerPass <= 8
        && Attention.MaxCriticalEdgeIndicators >= 0
        && Attention.MaxCriticalEdgeIndicators <= 8
        && Attention.MaximumAttentionDistanceCentimeters >= 100.0f
        && Attention.AmbientPersonalSpaceDistanceCentimeters >= 0.0f
        && Attention.AmbientPersonalSpaceDistanceCentimeters <= Attention.MaximumAttentionDistanceCentimeters
        && Attention.GazeMinimumViewDot >= -1.0f
        && Attention.GazeReleaseMinimumViewDot >= -1.0f
        && Attention.GazeReleaseMinimumViewDot <= Attention.GazeMinimumViewDot
        && Attention.FocusMinimumViewDot >= Attention.GazeMinimumViewDot
        && Attention.FocusMinimumViewDot <= 1.0f
        && Attention.FocusAcquireDwellSeconds >= 0.0f
        && Attention.WhisperAcquireDwellSeconds >= 0.0f
        && Attention.FocusReleaseGraceSeconds >= 0.0f
        && Attention.ReplacementDwellSeconds >= 0.0f
        && Attention.ReplacementScoreMultiplier >= 1.0f
        && Attention.SlotReleaseGraceSeconds >= 0.0f
        && Attention.DistanceDeadbandCentimeters >= 0.0f
        && Attention.LineOfSightCacheLifetimeSeconds >= 0.0f
        && Attention.LineOfSightMovementInvalidationCentimeters >= 0.0f
        && Attention.OcclusionHideGraceSeconds >= 0.0f
        && Attention.OcclusionRevealGraceSeconds >= 0.0f
        && Attention.ScreenEdgePaddingPixels >= 0.0f
        && Attention.ViewAlignmentWeight >= 0.0f
        && Attention.ProximityWeight >= 0.0f
        && Attention.OnScreenBonus >= 0.0f
        && Attention.LineOfSightBonus >= 0.0f
        && Attention.AwarenessPriorityBonus >= 0.0f
        && Attention.OpportunityPriorityBonus >= Attention.AwarenessPriorityBonus
        && Attention.SafetyPriorityBonus >= Attention.OpportunityPriorityBonus
        && Attention.InteractionOverrideBonus >= Attention.SafetyPriorityBonus
        && Attention.HardTargetOverrideBonus >= Attention.InteractionOverrideBonus
        && Attention.InspectOverrideBonus >= Attention.HardTargetOverrideBonus;
    if (!bAttentionFinite || !bAttentionRangesValid) {
        AddError(LOCTEXT(
            "Attention",
            "Entity Attention must use finite nonnegative timing, distance, visibility, and score values; retain the 16-observation/8-trace hard caps and semantic priority ordering."));
    }

    if (Statuses.ContextIconCap < 0 || Statuses.ContextIconCap > 4
        || Statuses.FocusIconCap < Statuses.ContextIconCap
        || Statuses.FocusIconCap > 4) {
        AddError(LOCTEXT("StatusCaps", "Status icon caps must stay between zero and four, and Focus Icon Cap cannot be smaller than Context Icon Cap."));
    }
    if (Actions.FocusActionCap < 1 || Actions.FocusActionCap > 2) {
        AddError(LOCTEXT("ActionCap", "Focus Action Cap must remain between one and two one-line rail entries."));
    }

    const float PassiveScalars[] = {
        PassiveIdentity.WhisperFullDistanceCentimeters,
        PassiveIdentity.WhisperAcquireDistanceCentimeters,
        PassiveIdentity.WhisperReleaseDistanceCentimeters,
        PassiveIdentity.WhisperFullAlpha,
        PassiveIdentity.WhisperReleaseScale,
        PassiveIdentity.FocusFullDistanceCentimeters,
        PassiveIdentity.FocusAcquireDistanceCentimeters,
        PassiveIdentity.FocusReleaseDistanceCentimeters,
        PassiveIdentity.FocusFullAlpha,
        PassiveIdentity.FocusReleaseScale,
        PassiveIdentity.AcquireTransitionSeconds,
        PassiveIdentity.ReleaseTransitionSeconds,
    };
    bool bPassiveFinite = true;
    for (const float Value : PassiveScalars) {
        bPassiveFinite &= FMath::IsFinite(Value);
    }
    const bool bPassiveRangesValid =
        PassiveIdentity.WhisperFullDistanceCentimeters >= 0.0f
        && PassiveIdentity.WhisperFullDistanceCentimeters
            <= PassiveIdentity.WhisperAcquireDistanceCentimeters
        && PassiveIdentity.WhisperAcquireDistanceCentimeters
            < PassiveIdentity.WhisperReleaseDistanceCentimeters
        && PassiveIdentity.FocusFullDistanceCentimeters >= 0.0f
        && PassiveIdentity.FocusFullDistanceCentimeters
            <= PassiveIdentity.FocusAcquireDistanceCentimeters
        && PassiveIdentity.FocusAcquireDistanceCentimeters
            < PassiveIdentity.FocusReleaseDistanceCentimeters
        && PassiveIdentity.WhisperFullAlpha >= 0.0f
        && PassiveIdentity.WhisperFullAlpha <= 1.0f
        && PassiveIdentity.FocusFullAlpha >= 0.0f
        && PassiveIdentity.FocusFullAlpha <= 1.0f
        && PassiveIdentity.WhisperReleaseScale >= 0.75f
        && PassiveIdentity.WhisperReleaseScale <= 1.0f
        && PassiveIdentity.FocusReleaseScale >= 0.75f
        && PassiveIdentity.FocusReleaseScale <= 1.0f
        && PassiveIdentity.AcquireTransitionSeconds >= 0.0f
        && PassiveIdentity.AcquireTransitionSeconds <= 0.5f
        && PassiveIdentity.ReleaseTransitionSeconds >= 0.0f
        && PassiveIdentity.ReleaseTransitionSeconds <= 0.5f;
    if (!bPassiveFinite || !bPassiveRangesValid) {
        AddError(LOCTEXT(
            "PassiveIdentity",
            "Passive identity distances must be finite and ordered Full <= Acquire < Release; alpha, scale, and transition values must remain inside their documented bounds."));
    }
    if (!FMath::IsFinite(Inspect.HoldDurationSeconds)
        || Inspect.HoldDurationSeconds < 0.20f
        || Inspect.HoldDurationSeconds > 1.50f
        || Inspect.MaxFactsPerSection < 1 || Inspect.MaxFactsPerSection > 16
        || Inspect.MaxTotalFacts < Inspect.MaxFactsPerSection
        || Inspect.MaxTotalFacts > 40) {
        AddError(LOCTEXT(
            "InspectCaps",
            "Inspect requires a 0.20 to 1.50 second hold, one to sixteen rows per section, and a total cap between that section cap and forty."));
    }

    return Result == EDataValidationResult::Invalid ? Result : EDataValidationResult::Valid;
}

#undef LOCTEXT_NAMESPACE
#endif

#include "World/Harvesting/MythicHarvestSettings.h"

#include "World/Harvesting/MythicHarvestRewardEscrowTypes.h"
#include "World/Harvesting/MythicHarvestSaveTypes.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "MythicHarvestSettings"

bool UMythicHarvestSettings::TryClampHarvestWorkMultiplier(
    const double RawMultiplier, double &OutMultiplier) const {
    OutMultiplier = 0.0;
    const double Minimum = static_cast<double>(MinimumWorkMultiplier);
    const double Maximum = static_cast<double>(MaximumWorkMultiplier);
    if (!FMath::IsFinite(RawMultiplier) || !FMath::IsFinite(Minimum)
        || !FMath::IsFinite(Maximum) || Minimum <= 0.0 || Maximum <= 0.0
        || Maximum < Minimum) {
        return false;
    }
    OutMultiplier = FMath::Clamp(RawMultiplier, Minimum, Maximum);
    return FMath::IsFinite(OutMultiplier);
}

bool UMythicHarvestSettings::AppendValidationErrors(TArray<FText> &OutErrors) const {
    const int32 InitialErrorCount = OutErrors.Num();
    auto RequireFinitePositive = [&OutErrors](const float Value, const FText &Message) {
        if (!FMath::IsFinite(Value) || Value <= 0.0f) {
            OutErrors.Add(Message);
        }
    };
    auto RequireFiniteNonNegative = [&OutErrors](const float Value, const FText &Message) {
        if (!FMath::IsFinite(Value) || Value < 0.0f) {
            OutErrors.Add(Message);
        }
    };

    RequireFinitePositive(AuthoritativeRangeCentimeters, LOCTEXT("InvalidRange", "Authoritative Range must be finite and positive."));
    if (LineOfSightTraceChannel.GetValue() < ECC_WorldStatic || LineOfSightTraceChannel.GetValue() >= ECC_MAX) {
        OutErrors.Add(LOCTEXT("InvalidTraceChannel", "Line Of Sight Trace Channel must be a valid collision channel."));
    }
    RequireFiniteNonNegative(CadenceToleranceSeconds, LOCTEXT("InvalidCadenceTolerance", "Cadence Tolerance must be finite and non-negative."));
    RequireFinitePositive(SoftClaimDurationSeconds, LOCTEXT("InvalidClaimDuration", "Soft Claim Duration must be finite and positive."));
    if (!FMath::IsFinite(MinimumContributionFraction) || MinimumContributionFraction < 0.0f || MinimumContributionFraction > 1.0f) {
        OutErrors.Add(LOCTEXT("InvalidContributionThreshold", "Minimum Contribution Fraction must be finite and inside [0,1]."));
    }
    RequireFinitePositive(MinimumWorkMultiplier, LOCTEXT("InvalidMinimumWorkMultiplier", "Minimum Work Multiplier must be finite and positive."));
    if (!FMath::IsFinite(MaximumWorkMultiplier) || MaximumWorkMultiplier < MinimumWorkMultiplier || MaximumWorkMultiplier <= 0.0f) {
        OutErrors.Add(LOCTEXT("InvalidMaximumWorkMultiplier", "Maximum Work Multiplier must be finite, positive, and at least the minimum."));
    }
    if (ContextInteractAction.IsNull() != ContextMappingContext.IsNull()) {
        OutErrors.Add(LOCTEXT("IncompleteContextInput", "Context Interact Action and Context Mapping Context must either both be assigned or both be empty."));
    }
    if (ContextMappingPriority < 1) {
        OutErrors.Add(LOCTEXT("InvalidContextMappingPriority", "Context Mapping Priority must be a positive Enhanced Input priority."));
    }
    RequireFinitePositive(FocusScanIntervalSeconds, LOCTEXT("InvalidFocusScanInterval", "Focus Scan Interval must be finite and positive."));
    RequireFiniteNonNegative(FocusRadiusCentimeters, LOCTEXT("InvalidFocusRadius", "Focus Radius must be finite and non-negative."));
    if (!FMath::IsFinite(FocusRangeCentimeters) || FocusRangeCentimeters <= 0.0f || FocusRangeCentimeters > AuthoritativeRangeCentimeters) {
        OutErrors.Add(LOCTEXT("InvalidFocusRange", "Focus Range must be finite, positive, and no greater than Authoritative Range."));
    }
    RequireFinitePositive(ReplicationGridSizeCentimeters, LOCTEXT("InvalidReplicationGrid", "Replication Grid Size must be finite and positive."));
    RequireFinitePositive(ReplicationCullDistanceCentimeters, LOCTEXT("InvalidReplicationCull", "Replication Cull Distance must be finite and positive."));
    RequireFiniteNonNegative(ReplicationRelevancyMarginCentimeters,
                             LOCTEXT("InvalidReplicationMargin", "Replication Relevancy Margin must be finite and non-negative."));
    if (FMath::IsFinite(ReplicationGridSizeCentimeters)
        && FMath::IsFinite(ReplicationCullDistanceCentimeters)
        && FMath::IsFinite(ReplicationRelevancyMarginCentimeters)) {
        constexpr double HalfDiagonalScale = 0.70710678118654752440;
        const double MinimumCull =
            static_cast<double>(ReplicationGridSizeCentimeters)
                * HalfDiagonalScale
            + static_cast<double>(ReplicationRelevancyMarginCentimeters);
        if (static_cast<double>(ReplicationCullDistanceCentimeters)
            < MinimumCull) {
            OutErrors.Add(LOCTEXT(
                "ReplicationCullDoesNotCoverCell",
                "Replication Cull Distance must cover the grid half-diagonal plus the configured relevancy margin."));
        }
    }
    if (RestoreMaximumTouchedNodes < 1
        || RestoreMaximumTouchedNodes
            > FMythicHarvestWorldSaveV1::AbsoluteMaximumNodes
        || RestoreMaximumContributorsPerNode < 1
        || RestoreMaximumContributorsPerNode
            > FMythicHarvestWorldSaveV1::
                AbsoluteMaximumContributorsPerNode
        || RestoreMaximumTotalContributors < 1
        || RestoreMaximumTotalContributors
            > FMythicHarvestWorldSaveV1::
                AbsoluteMaximumTotalContributors
        || RestoreMaximumReplicationCells < 1
        || RestoreMaximumReplicationCells
            > FMythicHarvestWorldSaveV1::
                AbsoluteMaximumReplicationCells
        || RestoreMaximumCellCoordinateMagnitude < 1
        || RestoreMaximumCellCoordinateMagnitude
            > FMythicHarvestWorldSaveV1::
                AbsoluteMaximumCellCoordinateMagnitude) {
        OutErrors.Add(LOCTEXT(
            "InvalidHarvestRestoreCapacity",
            "Harvest restore limits must be positive and no greater than their native serialization safety ceilings."));
    }
    RequireFiniteNonNegative(FeedbackRateLimitSeconds, LOCTEXT("InvalidFeedbackRate", "Feedback Rate Limit must be finite and non-negative."));
    RequireFinitePositive(RewardOutboxRetryIntervalSeconds,
                          LOCTEXT("InvalidRewardRetryInterval", "Reward Outbox Retry Interval must be finite and positive."));
    if (RewardOutboxGrantBudget < 1 || RewardOutboxGrantBudget > 256) {
        OutErrors.Add(LOCTEXT("InvalidRewardRetryBudget", "Reward Outbox Grant Budget must be inside [1,256]."));
    }
    if (RewardItemEscrowMaximumRows < 1
        || RewardItemEscrowMaximumRows
            > FMythicHarvestItemEscrowSaveV1::AbsoluteMaximumRows) {
        OutErrors.Add(LOCTEXT(
            "InvalidRewardItemEscrowMaximumRows",
            "Reward Item Escrow Maximum Rows must be inside [1,4096]."));
    }
    if (RewardReceiptCompactionThreshold < 1
        || RewardReceiptCompactionThreshold > 65536) {
        OutErrors.Add(LOCTEXT(
            "InvalidRewardReceiptCompactionThreshold",
            "Reward Receipt Compaction Threshold must be inside [1,65536]."));
    }
    if (RewardReceiptMaximumRows < RewardReceiptCompactionThreshold
        || RewardReceiptMaximumRows > 65536) {
        OutErrors.Add(LOCTEXT(
            "InvalidRewardReceiptMaximumRows",
            "Reward Receipt Maximum Rows must be inside [threshold,65536]."));
    }
    if (!FMath::IsFinite(RewardReceiptSaveDebounceSeconds)
        || RewardReceiptSaveDebounceSeconds < 0.0f
        || RewardReceiptSaveDebounceSeconds > 2.0f) {
        OutErrors.Add(LOCTEXT(
            "InvalidRewardReceiptSaveDebounce",
            "Reward Receipt Save Debounce must be finite and inside [0,2] seconds."));
    }
    RequireFiniteNonNegative(RespawnVisibilityRadiusCentimeters,
                             LOCTEXT("InvalidVisibilityRadius", "Respawn Visibility Radius must be finite and non-negative."));
    RequireFinitePositive(RespawnVisibilityRecheckSeconds, LOCTEXT("InvalidVisibilityRecheck", "Respawn Visibility Recheck must be finite and positive."));

    return OutErrors.Num() == InitialErrorCount;
}

#if WITH_EDITOR
EDataValidationResult UMythicHarvestSettings::IsDataValid(FDataValidationContext &Context) const {
    const EDataValidationResult ParentResult = Super::IsDataValid(Context);
    TArray<FText> Errors;
    AppendValidationErrors(Errors);
    for (const FText &Error : Errors) {
        Context.AddError(Error);
    }
    return ParentResult == EDataValidationResult::Invalid || !Errors.IsEmpty() ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif

#undef LOCTEXT_NAMESPACE

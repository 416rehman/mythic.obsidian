#include "Interaction/ContextActions/MythicContextActionProjectionPolicy.h"

#include <limits>

#include "Interaction/ContextActions/MythicTags_ContextActions.h"
#include "Misc/DataValidation.h"

#define LOCTEXT_NAMESPACE "MythicContextActionProjectionPolicy"

const FPrimaryAssetType UMythicContextActionProjectionPolicy::PrimaryAssetType(
    TEXT("MythicContextActionProjectionPolicy"));

namespace {
bool IsSafeActionTag(const FGameplayTag Tag) {
    return Tag.IsValid() && Tag.MatchesTag(CONTEXT_ACTION_ROOT)
           && !Tag.MatchesTagExact(CONTEXT_ACTION_ROOT)
           && !Tag.MatchesTag(CONTEXT_ACTION_REASON_ROOT);
}

bool IsSafeReasonTag(const FGameplayTag Tag) {
    return Tag.IsValid() && Tag.MatchesTag(CONTEXT_ACTION_REASON_ROOT)
           && !Tag.MatchesTagExact(CONTEXT_ACTION_REASON_ROOT);
}

bool IsOfferEnumValid(const EMythicContextActionAvailability Availability) {
    return Availability >= EMythicContextActionAvailability::Hidden
           && Availability <= EMythicContextActionAvailability::UnavailableWithReason;
}

bool IsStructurallySafeOffer(const FMythicContextActionOffer &Offer) {
    if (!IsValid(Offer.Definition) || !IsSafeActionTag(Offer.GetActionTag())
        || !IsOfferEnumValid(Offer.Availability) || Offer.SourceRevision < 0
        || Offer.SourceRevision > static_cast<int64>(MAX_uint32)) {
        return false;
    }

    const UMythicContextActionDefinition &Definition = *Offer.Definition;
    if (Definition.FocusPolicy < EMythicContextActionFocusPolicy::NotRequired
        || Definition.FocusPolicy > EMythicContextActionFocusPolicy::LockedSubject
        || Definition.RangePolicy < EMythicContextActionRangePolicy::NotRequired
        || Definition.RangePolicy > EMythicContextActionRangePolicy::DefinitionRange
        || Definition.LineOfSightPolicy
               < EMythicContextActionLineOfSightPolicy::NotRequired
        || Definition.LineOfSightPolicy
               > EMythicContextActionLineOfSightPolicy::ViewerInteractionOriginToSubject
        || !FMath::IsFinite(Definition.MaximumFocusAngleDegrees)
        || Definition.MaximumFocusAngleDegrees <= 0.0f
        || Definition.MaximumFocusAngleDegrees > 90.0f
        || !FMath::IsFinite(Definition.MaximumRangeCentimeters)
        || Definition.MaximumRangeCentimeters < 0.0f
        || !FMythicContextActionProjectionRules::IsHoldDurationValid(
            Definition.HoldDurationSeconds)
        || (Definition.RangePolicy == EMythicContextActionRangePolicy::DefinitionRange
            && Definition.MaximumRangeCentimeters <= 0.0f)) {
        return false;
    }
    return true;
}

bool IsPreferredForCapacity(const FMythicContextActionOffer &Left,
                            const FMythicContextActionOffer &Right) {
    const int32 LeftPriority = Left.Definition ? Left.Definition->PresentationPriority : MIN_int32;
    const int32 RightPriority = Right.Definition ? Right.Definition->PresentationPriority : MIN_int32;
    if (LeftPriority != RightPriority) {
        return LeftPriority > RightPriority;
    }
    return Left.GetActionTag().GetTagName().LexicalLess(
        Right.GetActionTag().GetTagName());
}

bool IsPreferredDuplicate(const FMythicContextActionOffer &Candidate,
                          const FMythicContextActionOffer &Existing) {
    const bool bCandidateFailClosed =
        Candidate.Availability == EMythicContextActionAvailability::UnavailableWithReason
        && Existing.Availability == EMythicContextActionAvailability::Available;
    const bool bSameStateAndNewer =
        Candidate.Availability == Existing.Availability
        && Candidate.SourceRevision > Existing.SourceRevision;
    return bCandidateFailClosed || bSameStateAndNewer;
}
} // namespace

FPrimaryAssetId UMythicContextActionProjectionPolicy::GetPrimaryAssetId() const {
    return FPrimaryAssetId(PrimaryAssetType, GetFName());
}

#if WITH_EDITOR
EDataValidationResult UMythicContextActionProjectionPolicy::IsDataValid(
    FDataValidationContext &Context) const {
    EDataValidationResult Result = Super::IsDataValid(Context);
    auto AddError = [&Context, &Result](const FText &Message) {
        Context.AddError(Message);
        Result = EDataValidationResult::Invalid;
    };

    if (!FMath::IsFinite(MaximumDiscoveryRangeCentimeters)
        || MaximumDiscoveryRangeCentimeters <= 0.0f
        || MaximumDiscoveryRangeCentimeters
               > FMythicContextActionProjectionRules::HardMaximumDiscoveryRangeCentimeters) {
        AddError(LOCTEXT("InvalidDiscoveryRange",
                         "Maximum Discovery Range must be finite and in the range (0, 10000] centimeters."));
    }
    if (DiscoveryTraceChannel >= ECC_MAX) {
        AddError(LOCTEXT("InvalidTraceChannel", "Discovery Trace Channel must be a valid collision channel."));
    }
    if (!FMath::IsFinite(MinimumClientRequestIntervalSeconds)
        || MinimumClientRequestIntervalSeconds
               < FMythicContextActionProjectionRules::HardMinimumRequestIntervalSeconds
        || MinimumClientRequestIntervalSeconds
               > FMythicContextActionProjectionRules::HardMaximumRequestIntervalSeconds) {
        AddError(LOCTEXT("InvalidRequestInterval",
                         "Minimum Client Request Interval must be finite and between 0.05 and 2 seconds."));
    }
    if (!FMath::IsFinite(AuthorityRefreshIntervalSeconds)
        || AuthorityRefreshIntervalSeconds < MinimumClientRequestIntervalSeconds
        || AuthorityRefreshIntervalSeconds
               > FMythicContextActionProjectionRules::HardMaximumRefreshIntervalSeconds) {
        AddError(LOCTEXT("InvalidRefreshInterval",
                         "Authority Refresh Interval must be finite, no faster than the request interval, and at most 5 seconds."));
    }
    if (!FMath::IsFinite(OfferLeaseDurationSeconds)
        || OfferLeaseDurationSeconds <= AuthorityRefreshIntervalSeconds
        || OfferLeaseDurationSeconds
               > FMythicContextActionProjectionRules::HardMaximumLeaseDurationSeconds) {
        AddError(LOCTEXT("InvalidLeaseDuration",
                         "Offer Lease Duration must be finite, greater than the refresh interval, and at most 30 seconds."));
    }
    if (MaximumProviderComponents < 1
        || MaximumProviderComponents
               > FMythicContextActionProjectionRules::HardMaximumProviderComponents) {
        AddError(LOCTEXT("InvalidProviderCap", "Maximum Provider Components must be in the range 1..32."));
    }
    if (MaximumOffersPerProvider < 1
        || MaximumOffersPerProvider
               > FMythicContextActionProjectionRules::HardMaximumOffersPerProvider) {
        AddError(LOCTEXT("InvalidProviderOfferCap", "Maximum Offers Per Provider must be in the range 1..32."));
    }
    if (MaximumProjectedOffers < 1
        || MaximumProjectedOffers
               > FMythicContextActionProjectionRules::HardMaximumProjectedOffers) {
        AddError(LOCTEXT("InvalidProjectionCap", "Maximum Projected Offers must be in the range 1..16."));
    }
    return Result;
}
#endif

FMythicContextActionProjectionRuntimePolicy
FMythicContextActionProjectionRules::BuildRuntimePolicy(
    const UMythicContextActionProjectionPolicy *Policy) {
    FMythicContextActionProjectionRuntimePolicy Result;
    if (!IsValid(Policy) || !FMath::IsFinite(Policy->MaximumDiscoveryRangeCentimeters)
        || Policy->MaximumDiscoveryRangeCentimeters <= 0.0f
        || Policy->MaximumDiscoveryRangeCentimeters > HardMaximumDiscoveryRangeCentimeters
        || Policy->DiscoveryTraceChannel >= ECC_MAX
        || !FMath::IsFinite(Policy->MinimumClientRequestIntervalSeconds)
        || Policy->MinimumClientRequestIntervalSeconds < HardMinimumRequestIntervalSeconds
        || Policy->MinimumClientRequestIntervalSeconds > HardMaximumRequestIntervalSeconds
        || !FMath::IsFinite(Policy->AuthorityRefreshIntervalSeconds)
        || Policy->AuthorityRefreshIntervalSeconds < Policy->MinimumClientRequestIntervalSeconds
        || Policy->AuthorityRefreshIntervalSeconds > HardMaximumRefreshIntervalSeconds
        || !FMath::IsFinite(Policy->OfferLeaseDurationSeconds)
        || Policy->OfferLeaseDurationSeconds <= Policy->AuthorityRefreshIntervalSeconds
        || Policy->OfferLeaseDurationSeconds > HardMaximumLeaseDurationSeconds
        || Policy->MaximumProviderComponents < 1
        || Policy->MaximumProviderComponents > HardMaximumProviderComponents
        || Policy->MaximumOffersPerProvider < 1
        || Policy->MaximumOffersPerProvider > HardMaximumOffersPerProvider
        || Policy->MaximumProjectedOffers < 1
        || Policy->MaximumProjectedOffers > HardMaximumProjectedOffers) {
        return Result;
    }

    Result.MaximumDiscoveryRangeCentimeters = Policy->MaximumDiscoveryRangeCentimeters;
    Result.MinimumClientRequestIntervalSeconds = Policy->MinimumClientRequestIntervalSeconds;
    Result.AuthorityRefreshIntervalSeconds = Policy->AuthorityRefreshIntervalSeconds;
    Result.OfferLeaseDurationSeconds = Policy->OfferLeaseDurationSeconds;
    Result.MaximumProviderComponents = Policy->MaximumProviderComponents;
    Result.MaximumOffersPerProvider = Policy->MaximumOffersPerProvider;
    Result.MaximumProjectedOffers = Policy->MaximumProjectedOffers;
    Result.DiscoveryTraceChannel = Policy->DiscoveryTraceChannel;
    Result.bTraceComplex = Policy->bTraceComplex;
    Result.bValid = true;
    return Result;
}

bool FMythicContextActionProjectionRules::IsExactResolvedSubject(
    const FMythicEntityPresentationInstance &Requested,
    const FMythicEntityPresentationInstance &Resolved) {
    return Requested.IsValid() && Resolved.IsValid() && Requested == Resolved;
}

double FMythicContextActionProjectionRules::GetRequestThrottleDelaySeconds(
    const double NowSeconds, const double LastAcceptedSeconds,
    const float MinimumIntervalSeconds) {
    if (!FMath::IsFinite(NowSeconds) || !FMath::IsFinite(MinimumIntervalSeconds)
        || MinimumIntervalSeconds < HardMinimumRequestIntervalSeconds
        || MinimumIntervalSeconds > HardMaximumRequestIntervalSeconds) {
        return std::numeric_limits<double>::infinity();
    }
    if (LastAcceptedSeconds == -DBL_MAX) {
        return 0.0;
    }
    if (!FMath::IsFinite(LastAcceptedSeconds)
        || LastAcceptedSeconds < 0.0) {
        return std::numeric_limits<double>::infinity();
    }
    return FMath::Max(0.0, LastAcceptedSeconds
                               + static_cast<double>(MinimumIntervalSeconds)
                               - NowSeconds);
}

bool FMythicContextActionProjectionRules::IsHoldDurationValid(
    const float HoldDurationSeconds) {
    return FMath::IsFinite(HoldDurationSeconds)
        && (HoldDurationSeconds == 0.0f
            || (HoldDurationSeconds >= HardMinimumHoldDurationSeconds
                && HoldDurationSeconds <= HardMaximumHoldDurationSeconds));
}

bool FMythicContextActionProjectionRules::IsHoldCompletionTimingValid(
    const double AuthorityStartSeconds,
    const double AuthorityCompletionSeconds,
    const float RequiredHoldDurationSeconds) {
    if (!FMath::IsFinite(AuthorityStartSeconds)
        || !FMath::IsFinite(AuthorityCompletionSeconds)
        || AuthorityStartSeconds < 0.0
        || AuthorityCompletionSeconds < AuthorityStartSeconds
        || !IsHoldDurationValid(RequiredHoldDurationSeconds)
        || RequiredHoldDurationSeconds <= 0.0f) {
        return false;
    }

    const double ElapsedSeconds =
        AuthorityCompletionSeconds - AuthorityStartSeconds;
    const double RequiredSeconds =
        static_cast<double>(RequiredHoldDurationSeconds);
    return ElapsedSeconds + HoldCompletionEarlyToleranceSeconds
               >= RequiredSeconds
        && ElapsedSeconds
               <= RequiredSeconds + HoldCompletionGraceSeconds;
}

bool FMythicContextActionProjectionRules::TryInsertBoundedOffer(
    const FMythicContextActionOffer &Candidate, const int32 MaximumOffers,
    TArray<FMythicContextActionOffer> &InOutOffers) {
    const int32 SafeMaximum = FMath::Clamp(MaximumOffers, 0, HardMaximumProjectedOffers);
    if (SafeMaximum <= 0 || !IsStructurallySafeOffer(Candidate)
        || Candidate.Availability == EMythicContextActionAvailability::Hidden
        || (Candidate.Availability == EMythicContextActionAvailability::UnavailableWithReason
            && !Candidate.Definition->bExplainWhenUnavailable)) {
        return false;
    }

    FMythicContextActionOffer SafeCandidate = Candidate;
    if (SafeCandidate.Availability == EMythicContextActionAvailability::Available
        || !IsSafeReasonTag(SafeCandidate.UnavailableReasonTag)) {
        SafeCandidate.UnavailableReasonTag = FGameplayTag();
    }

    if (FMythicContextActionOffer *Existing = InOutOffers.FindByPredicate(
            [&SafeCandidate](const FMythicContextActionOffer &Offer) {
                return Offer.GetActionTag() == SafeCandidate.GetActionTag();
            })) {
        if (!IsPreferredDuplicate(SafeCandidate, *Existing)) {
            return false;
        }
        *Existing = MoveTemp(SafeCandidate);
        return true;
    }

    if (InOutOffers.Num() < SafeMaximum) {
        InOutOffers.Add(MoveTemp(SafeCandidate));
        return true;
    }

    int32 WorstIndex = 0;
    for (int32 Index = 1; Index < InOutOffers.Num(); ++Index) {
        if (IsPreferredForCapacity(InOutOffers[WorstIndex], InOutOffers[Index])) {
            WorstIndex = Index;
        }
    }
    if (!IsPreferredForCapacity(SafeCandidate, InOutOffers[WorstIndex])) {
        return false;
    }
    InOutOffers[WorstIndex] = MoveTemp(SafeCandidate);
    return true;
}

#undef LOCTEXT_NAMESPACE

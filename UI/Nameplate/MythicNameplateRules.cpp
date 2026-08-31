#include "UI/Nameplate/MythicNameplateRules.h"

#include "UI/Settings/MythicUserSettings.h"

namespace {
int32 TierOrdinal(const EMythicNameplateDisclosureTier Tier) {
    return static_cast<int32>(Tier);
}
}

EMythicNameplateDisclosureTier
FMythicNameplateRules::ResolveDesiredDisclosure(
    const FMythicNameplateDisclosureEvidence &Evidence) {
    if (!Evidence.bPresentationPermitted) {
        return EMythicNameplateDisclosureTier::Silent;
    }

    // Ordinary world-space presentation never acquires or persists as x-ray UI.
    if (!Evidence.bHasLineOfSight) {
        return EMythicNameplateDisclosureTier::Silent;
    }

    if (Evidence.bHardTarget || Evidence.bInteractionTarget || Evidence.bFocusAttention) {
        return EMythicNameplateDisclosureTier::Focus;
    }
    if (Evidence.bContextSignal) {
        return EMythicNameplateDisclosureTier::Context;
    }
    if (Evidence.bGazeAttention || Evidence.bPersonalSpaceAttention) {
        return EMythicNameplateDisclosureTier::Whisper;
    }
    return EMythicNameplateDisclosureTier::Silent;
}

EMythicNameplateDisclosureTier FMythicNameplateRules::ApplyPresentationMode(
    const EMythicNameplateDisclosureTier EntitledTier,
    const EMythicNameplatePresentationMode Mode,
    const bool bMandatorySafetyOrCurrentTarget) {
    if (Mode == EMythicNameplatePresentationMode::Minimal
        && TierOrdinal(EntitledTier)
               < TierOrdinal(EMythicNameplateDisclosureTier::Focus)
        && !bMandatorySafetyOrCurrentTarget) {
        return EMythicNameplateDisclosureTier::Silent;
    }
    // Expanded changes bounded density only. Tier is an information boundary and can never be upgraded by settings.
    return EntitledTier;
}

bool FMythicNameplateRules::ShouldAdmitPassiveSurface(
    const float DistanceCentimeters,
    const float AcquireDistanceCentimeters,
    const float ReleaseDistanceCentimeters,
    const bool bPassiveIncumbent) {
    if (!FMath::IsFinite(DistanceCentimeters)
        || !FMath::IsFinite(AcquireDistanceCentimeters)
        || !FMath::IsFinite(ReleaseDistanceCentimeters)
        || DistanceCentimeters < 0.0f
        || AcquireDistanceCentimeters < 0.0f
        || ReleaseDistanceCentimeters <= AcquireDistanceCentimeters) {
        return false;
    }
    return bPassiveIncumbent
        ? DistanceCentimeters < ReleaseDistanceCentimeters
        : DistanceCentimeters <= AcquireDistanceCentimeters;
}

FMythicNameplateDistancePresentation
FMythicNameplateRules::ResolvePassiveDistancePresentation(
    const float DistanceCentimeters,
    const float FullDistanceCentimeters,
    const float ReleaseDistanceCentimeters,
    const float FullAlpha,
    const float ReleaseScale,
    const bool bSnapDistance) {
    FMythicNameplateDistancePresentation Result;
    if (!FMath::IsFinite(DistanceCentimeters)
        || !FMath::IsFinite(FullDistanceCentimeters)
        || !FMath::IsFinite(ReleaseDistanceCentimeters)
        || !FMath::IsFinite(FullAlpha)
        || !FMath::IsFinite(ReleaseScale)
        || DistanceCentimeters < 0.0f
        || FullDistanceCentimeters < 0.0f
        || ReleaseDistanceCentimeters <= FullDistanceCentimeters) {
        return Result;
    }

    Result.bBeyondRelease = DistanceCentimeters >= ReleaseDistanceCentimeters;
    if (Result.bBeyondRelease) {
        Result.Alpha = 0.0f;
        Result.Scale = FMath::Clamp(ReleaseScale, 0.75f, 1.0f);
        return Result;
    }

    const float SafeFullAlpha = FMath::Clamp(FullAlpha, 0.0f, 1.0f);
    if (bSnapDistance || DistanceCentimeters <= FullDistanceCentimeters) {
        Result.Alpha = SafeFullAlpha;
        Result.Scale = 1.0f;
        return Result;
    }

    const float LinearProgress = FMath::Clamp(
        (DistanceCentimeters - FullDistanceCentimeters)
            / (ReleaseDistanceCentimeters - FullDistanceCentimeters),
        0.0f, 1.0f);
    const float SmoothProgress = LinearProgress * LinearProgress
        * (3.0f - 2.0f * LinearProgress);
    Result.Alpha = SafeFullAlpha * (1.0f - SmoothProgress);
    Result.Scale = FMath::Lerp(1.0f,
        FMath::Clamp(ReleaseScale, 0.75f, 1.0f), SmoothProgress);
    return Result;
}

float FMythicNameplateRules::ResolveTemporalAlpha(
    const float StartAlpha, const bool bReleasing,
    const double TransitionStartSeconds, const double NowSeconds,
    const float AcquireTransitionSeconds,
    const float ReleaseTransitionSeconds,
    const bool bReducedMotion) {
    const float TargetAlpha = bReleasing ? 0.0f : 1.0f;
    if (bReducedMotion) {
        return TargetAlpha;
    }
    if (!FMath::IsFinite(StartAlpha)
        || !FMath::IsFinite(TransitionStartSeconds)
        || !FMath::IsFinite(NowSeconds)
        || !FMath::IsFinite(AcquireTransitionSeconds)
        || !FMath::IsFinite(ReleaseTransitionSeconds)) {
        return TargetAlpha;
    }
    const float Duration = FMath::Max(0.0f,
        bReleasing ? ReleaseTransitionSeconds : AcquireTransitionSeconds);
    if (Duration <= UE_SMALL_NUMBER) {
        return TargetAlpha;
    }
    const float LinearProgress = FMath::Clamp(
        static_cast<float>((NowSeconds - TransitionStartSeconds)
            / static_cast<double>(Duration)), 0.0f, 1.0f);
    const float SmoothProgress = LinearProgress * LinearProgress
        * (3.0f - 2.0f * LinearProgress);
    return FMath::Lerp(FMath::Clamp(StartAlpha, 0.0f, 1.0f),
                       TargetAlpha, SmoothProgress);
}

int32 FMythicNameplateRules::GetCuePrecedence(const EMythicNameplatePrimaryCue Cue) {
    switch (Cue) {
    case EMythicNameplatePrimaryCue::Dead:
        return 140;
    case EMythicNameplatePrimaryCue::Downed:
        return 132;
    case EMythicNameplatePrimaryCue::Dying:
        return 131;
    case EMythicNameplatePrimaryCue::AttackingViewer:
        return 110;
    case EMythicNameplatePrimaryCue::Surrendering:
        return 121;
    case EMythicNameplatePrimaryCue::Fleeing:
        return 120;
    case EMythicNameplatePrimaryCue::DirectedTalk:
        return 100;
    case EMythicNameplatePrimaryCue::QuestTurnIn:
        return 90;
    case EMythicNameplatePrimaryCue::QuestOffer:
        return 80;
    case EMythicNameplatePrimaryCue::Service:
        return 70;
    case EMythicNameplatePrimaryCue::OtherAction:
        return 60;
    case EMythicNameplatePrimaryCue::ObservableActivity:
        return 50;
    case EMythicNameplatePrimaryCue::Role:
        return 40;
    case EMythicNameplatePrimaryCue::Faction:
        return 30;
    case EMythicNameplatePrimaryCue::None:
    default:
        return 0;
    }
}

EMythicNameplatePrimaryCue FMythicNameplateRules::SelectPrimaryCue(
    const TConstArrayView<EMythicNameplatePrimaryCue> Candidates) {
    EMythicNameplatePrimaryCue Best = EMythicNameplatePrimaryCue::None;
    int32 BestPrecedence = GetCuePrecedence(Best);
    for (const EMythicNameplatePrimaryCue Candidate : Candidates) {
        const int32 CandidatePrecedence = GetCuePrecedence(Candidate);
        if (CandidatePrecedence > BestPrecedence
            || (CandidatePrecedence == BestPrecedence && static_cast<uint8>(Candidate) < static_cast<uint8>(Best))) {
            Best = Candidate;
            BestPrecedence = CandidatePrecedence;
        }
    }
    return Best;
}

EMythicNameplatePrimaryCue
FMythicNameplateRules::ResolveObservedFightingCue(
    const bool bRecentCombatSignal,
    const bool bRecentIncomingCombatSignal) {
    return bRecentCombatSignal && bRecentIncomingCombatSignal
        ? EMythicNameplatePrimaryCue::AttackingViewer
        : EMythicNameplatePrimaryCue::ObservableActivity;
}

EMythicNameplateLane FMythicNameplateRules::ResolveLane(const EMythicNameplatePrimaryCue Cue,
                                                         const bool bIsFocused) {
    if (bIsFocused) {
        return EMythicNameplateLane::Focus;
    }
    switch (Cue) {
    case EMythicNameplatePrimaryCue::Downed:
    case EMythicNameplatePrimaryCue::Dying:
    case EMythicNameplatePrimaryCue::AttackingViewer:
        return EMythicNameplateLane::Safety;

    case EMythicNameplatePrimaryCue::DirectedTalk:
    case EMythicNameplatePrimaryCue::QuestTurnIn:
    case EMythicNameplatePrimaryCue::QuestOffer:
    case EMythicNameplatePrimaryCue::Service:
    case EMythicNameplatePrimaryCue::OtherAction:
    case EMythicNameplatePrimaryCue::Dead:
        return EMythicNameplateLane::Opportunity;

    case EMythicNameplatePrimaryCue::Surrendering:
    case EMythicNameplatePrimaryCue::Fleeing:
    case EMythicNameplatePrimaryCue::ObservableActivity:
    case EMythicNameplatePrimaryCue::Role:
    case EMythicNameplatePrimaryCue::Faction:
    case EMythicNameplatePrimaryCue::None:
    default:
        return EMythicNameplateLane::Awareness;
    }
}

bool FMythicNameplateRules::ShouldShowHealth(
    const EMythicNameplateDisclosureTier Tier,
                                             const FMythicNameplateHealthContext &Context) {
    if (!Context.bHealthPresentationPermitted
        || TierOrdinal(Tier)
               < TierOrdinal(EMythicNameplateDisclosureTier::Context)) {
        return false;
    }
    if (Context.bCurrentCombatTarget || Context.bBoss || Context.bDownedOrDying) {
        return true;
    }
    if (!Context.bInjured) {
        return false;
    }
    return Context.bCombatRelevant || Context.bPartyOrCompanion || Context.bCanAssist
        || TierOrdinal(Tier)
               >= TierOrdinal(EMythicNameplateDisclosureTier::Focus);
}

bool FMythicNameplateRules::ShouldShowExactLevel(
    const EMythicNameplateDisclosureTier Tier,
                                                 const FMythicNameplateLevelContext &Context) {
    if (!Context.bExactLevelPermitted || !Context.bCombatCapable) {
        return false;
    }
    if (TierOrdinal(Tier)
        >= TierOrdinal(EMythicNameplateDisclosureTier::Focus)) {
        return true;
    }
    return Tier == EMythicNameplateDisclosureTier::Context
        && Context.bCurrentCombatTarget;
}

float FMythicNameplateRules::GetAcquireDwellSeconds(
    const EMythicNameplateDisclosureTier DesiredTier,
                                                     const FMythicEntityAttentionConfig &Attention) {
    switch (DesiredTier) {
    case EMythicNameplateDisclosureTier::Whisper:
        return FMath::Max(0.0f, Attention.WhisperAcquireDwellSeconds);
    case EMythicNameplateDisclosureTier::Focus:
        return FMath::Max(0.0f, Attention.FocusAcquireDwellSeconds);
    case EMythicNameplateDisclosureTier::Silent:
    case EMythicNameplateDisclosureTier::Context:
    default:
        return 0.0f;
    }
}

float FMythicNameplateRules::GetReleaseGraceSeconds(
    const EMythicNameplateDisclosureTier CurrentTier,
                                                     const FMythicEntityAttentionConfig &Attention) {
    switch (CurrentTier) {
    case EMythicNameplateDisclosureTier::Focus:
        return FMath::Max(0.0f, Attention.FocusReleaseGraceSeconds);
    case EMythicNameplateDisclosureTier::Whisper:
    case EMythicNameplateDisclosureTier::Context:
        return FMath::Max(0.0f, Attention.SlotReleaseGraceSeconds);
    case EMythicNameplateDisclosureTier::Silent:
    default:
        return 0.0f;
    }
}

bool FMythicNameplateRules::ShouldPromote(
    const EMythicNameplateDisclosureTier CurrentTier,
    const EMythicNameplateDisclosureTier DesiredTier,
                                          const float EvidenceHeldSeconds,
                                          const FMythicEntityAttentionConfig &Attention) {
    return TierOrdinal(DesiredTier) > TierOrdinal(CurrentTier)
        && FMath::IsFinite(EvidenceHeldSeconds)
        && EvidenceHeldSeconds >= GetAcquireDwellSeconds(DesiredTier, Attention);
}

bool FMythicNameplateRules::ShouldDemote(
    const EMythicNameplateDisclosureTier CurrentTier,
    const EMythicNameplateDisclosureTier DesiredTier,
                                         const float EvidenceLostSeconds,
                                         const FMythicEntityAttentionConfig &Attention) {
    return TierOrdinal(DesiredTier) < TierOrdinal(CurrentTier)
        && FMath::IsFinite(EvidenceLostSeconds)
        && EvidenceLostSeconds >= GetReleaseGraceSeconds(CurrentTier, Attention);
}

bool FMythicNameplateRules::HasReplacementScoreLead(
    const float IncumbentScore, const float ChallengerScore,
    const FMythicEntityAttentionConfig &Attention) {
    if (!FMath::IsFinite(IncumbentScore)
        || !FMath::IsFinite(ChallengerScore)
        || ChallengerScore <= 0.0f) {
        return false;
    }
    const float EffectiveIncumbent = FMath::Max(0.0f, IncumbentScore);
    if (EffectiveIncumbent <= UE_SMALL_NUMBER) {
        return true;
    }
    const float RequiredMultiplier = FMath::Max(
        1.0f, Attention.ReplacementScoreMultiplier);
    return ChallengerScore >= EffectiveIncumbent * RequiredMultiplier;
}

bool FMythicNameplateRules::ShouldReplaceIncumbent(const float IncumbentScore,
                                                   const float ChallengerScore,
                                                   const float ChallengerLeadSeconds,
                                                   const bool bChallengerFromHigherLane,
                                                   const FMythicEntityAttentionConfig &Attention) {
    if (!FMath::IsFinite(IncumbentScore)
        || !FMath::IsFinite(ChallengerScore)
        || !FMath::IsFinite(ChallengerLeadSeconds)
        || ChallengerScore <= 0.0f) {
        return false;
    }
    if (bChallengerFromHigherLane) {
        return true;
    }
    if (ChallengerLeadSeconds < FMath::Max(0.0f, Attention.ReplacementDwellSeconds)) {
        return false;
    }
    return HasReplacementScoreLead(IncumbentScore, ChallengerScore,
                                   Attention);
}

int32 FMythicNameplateRules::GetLaneCapacity(const EMythicNameplateLane Lane,
                                              const FMythicNameplateCapacityPolicy &Capacity) {
    switch (Lane) {
    case EMythicNameplateLane::Focus:
        return FMath::Max(0, Capacity.FocusLaneSlots);
    case EMythicNameplateLane::Safety:
        return FMath::Max(0, Capacity.SafetyLaneSlots);
    case EMythicNameplateLane::Opportunity:
        return FMath::Max(0, Capacity.OpportunityLaneSlots);
    case EMythicNameplateLane::Awareness:
    default:
        return FMath::Max(0, Capacity.AwarenessLaneSlots);
    }
}

int32 FMythicNameplateRules::GetAmbientWhisperCapacity(
    const bool bHasFocusedSurface,
    const EMythicNameplatePresentationMode Mode,
    const FMythicNameplateCapacityPolicy &Capacity) {
    if (bHasFocusedSurface
        || Mode == EMythicNameplatePresentationMode::Minimal) {
        return 0;
    }
    return Mode == EMythicNameplatePresentationMode::Expanded
        ? GetLaneCapacity(EMythicNameplateLane::Awareness, Capacity)
        : 1;
}

int32 FMythicNameplateRules::GetStatusIconCap(
    const EMythicNameplateDisclosureTier Tier,
                                              const FMythicNameplateStatusPolicy &Statuses) {
    if (TierOrdinal(Tier)
        >= TierOrdinal(EMythicNameplateDisclosureTier::Focus)) {
        return FMath::Clamp(Statuses.FocusIconCap, 0, 4);
    }
    return Tier == EMythicNameplateDisclosureTier::Context
        ? FMath::Clamp(Statuses.ContextIconCap, 0, 4) : 0;
}

bool FMythicNameplateRules::ResolveDeclutteredPlacement(
    const FVector2D DesiredCenter, const FVector2D LogicalFootprint,
    const EMythicNameplateLane Lane, const bool bWasSuppressed,
    const float ScreenPixelsPerLogicalPixel,
    const TConstArrayView<FBox2D> HigherPriorityBounds,
    const FMythicNameplateDeclutterPolicy &Declutter,
    FVector2D &OutResolvedCenter, FBox2D &OutResolvedBounds) {
    OutResolvedCenter = DesiredCenter;
    OutResolvedBounds = FBox2D(DesiredCenter, DesiredCenter);
    if (DesiredCenter.ContainsNaN() || LogicalFootprint.ContainsNaN()
        || !FMath::IsFinite(ScreenPixelsPerLogicalPixel)
        || ScreenPixelsPerLogicalPixel <= 0.0f) {
        return false;
    }

    const float Scale = FMath::Clamp(ScreenPixelsPerLogicalPixel,
                                     0.05f, 8.0f);
    const FVector2D HalfExtent(
        FMath::Max(FMath::Abs(LogicalFootprint.X) * Scale * 0.5f,
                   1.0f),
        FMath::Max(FMath::Abs(LogicalFootprint.Y) * Scale * 0.5f,
                   1.0f));
    const float Padding = FMath::Max(0.0f,
        Declutter.CollisionPaddingPixels
            + (bWasSuppressed
                   ? Declutter.ReleaseHysteresisPixels : 0.0f))
        * Scale;
    const FVector2D PaddingExtent(Padding, Padding);

    auto TryPosition = [&](const FVector2D Center) {
        const FBox2D Bounds(Center - HalfExtent, Center + HalfExtent);
        const FBox2D PaddedBounds(Bounds.Min - PaddingExtent,
                                  Bounds.Max + PaddingExtent);
        for (const FBox2D &Occupied : HigherPriorityBounds) {
            const bool bIntersects = PaddedBounds.Min.X < Occupied.Max.X
                && PaddedBounds.Max.X > Occupied.Min.X
                && PaddedBounds.Min.Y < Occupied.Max.Y
                && PaddedBounds.Max.Y > Occupied.Min.Y;
            if (bIntersects) {
                return false;
            }
        }
        OutResolvedCenter = Center;
        OutResolvedBounds = Bounds;
        return true;
    };

    if (TryPosition(DesiredCenter)) {
        return true;
    }
    if (Lane == EMythicNameplateLane::Awareness
        && Declutter.bSuppressAwarenessOnCollision) {
        return false;
    }

    const float VerticalStep = FMath::Max(
        4.0f, Declutter.VerticalStepPixels) * Scale;
    const int32 MaxSteps = FMath::Clamp(Declutter.MaxVerticalSteps,
                                        0, 4);
    const float MaximumDisplacement = VerticalStep
        * static_cast<float>(MaxSteps);
    FVector2D CandidateCenter = DesiredCenter;
    for (int32 Step = 0; Step < MaxSteps; ++Step) {
        const FBox2D CandidateBounds(
            CandidateCenter - HalfExtent - PaddingExtent,
            CandidateCenter + HalfExtent + PaddingExtent);
        float RequiredCenterY = CandidateCenter.Y;
        bool bFoundConflict = false;
        for (const FBox2D &Occupied : HigherPriorityBounds) {
            const bool bIntersects = CandidateBounds.Min.X < Occupied.Max.X
                && CandidateBounds.Max.X > Occupied.Min.X
                && CandidateBounds.Min.Y < Occupied.Max.Y
                && CandidateBounds.Max.Y > Occupied.Min.Y;
            if (!bIntersects) {
                continue;
            }
            bFoundConflict = true;
            RequiredCenterY = FMath::Min(
                RequiredCenterY,
                Occupied.Min.Y - Padding - HalfExtent.Y);
        }
        if (!bFoundConflict) {
            return TryPosition(CandidateCenter);
        }
        if (RequiredCenterY >= CandidateCenter.Y) {
            RequiredCenterY = CandidateCenter.Y - VerticalStep;
        }
        if (DesiredCenter.Y - RequiredCenterY
            > MaximumDisplacement + UE_KINDA_SMALL_NUMBER) {
            return false;
        }
        CandidateCenter.Y = RequiredCenterY;
        if (TryPosition(CandidateCenter)) {
            return true;
        }
    }
    return false;
}

EMythicNameplateVisualFamily FMythicNameplateRules::ResolveVisualFamily(
    const EMythicPresentedCombatRank PresentedRank,
    const bool bCombatPresentationEarned,
    const bool bUrgentProtectedAlly) {
    if (PresentedRank == EMythicPresentedCombatRank::Boss
        || PresentedRank == EMythicPresentedCombatRank::WorldBoss) {
        return EMythicNameplateVisualFamily::Boss;
    }
    if (bUrgentProtectedAlly) {
        return EMythicNameplateVisualFamily::AllySafety;
    }
    return bCombatPresentationEarned
        ? EMythicNameplateVisualFamily::Combat
        : EMythicNameplateVisualFamily::Identity;
}

void FMythicNameplateRules::SelectStatusCandidates(
    const TConstArrayView<FMythicNameplateStatusCandidate> Candidates,
    const int32 MaxIcons,
    TArray<FMythicNameplateStatusCandidate> &OutSelected,
    int32 &OutOverflowCount) {
    TArray<FMythicNameplateStatusCandidate> Eligible;
    Eligible.Reserve(Candidates.Num());
    for (const FMythicNameplateStatusCandidate &Candidate : Candidates) {
        if (Candidate.bPresentationPermitted && Candidate.StatusType.IsValid()) {
            Eligible.Add(Candidate);
        }
    }

    Eligible.Sort([](const FMythicNameplateStatusCandidate &Left,
                     const FMythicNameplateStatusCandidate &Right) {
        if (Left.Urgency != Right.Urgency) {
            return static_cast<uint8>(Left.Urgency) > static_cast<uint8>(Right.Urgency);
        }
        if (Left.bAppliedByViewerOrParty != Right.bAppliedByViewerOrParty) {
            return Left.bAppliedByViewerOrParty;
        }
        if (Left.AuthoredPriority != Right.AuthoredPriority) {
            return Left.AuthoredPriority > Right.AuthoredPriority;
        }
        if (Left.StableTieBreak != Right.StableTieBreak) {
            return Left.StableTieBreak < Right.StableTieBreak;
        }
        return Left.StatusType.GetTagName().LexicalLess(Right.StatusType.GetTagName());
    });

    const int32 CopyCount = FMath::Min(FMath::Max(0, MaxIcons), Eligible.Num());
    OutSelected.Reset(CopyCount);
    if (CopyCount > 0) {
        OutSelected.Append(Eligible.GetData(), CopyCount);
    }
    OutOverflowCount = MaxIcons > 0 ? Eligible.Num() - CopyCount : 0;
}

void FMythicNameplateRules::SanitizeProjectionForPresentation(
    FMythicNameplateProjection &InOutProjection) {
    if (InOutProjection.PrimaryCue
        == EMythicNameplatePrimaryCue::Dead) {
        const FMythicEntityPresentationInstance Instance =
            InOutProjection.Instance;
        InOutProjection = FMythicNameplateProjection();
        InOutProjection.Instance = Instance;
        return;
    }

    if (InOutProjection.DisclosureTier
        != EMythicNameplateDisclosureTier::Whisper) {
        return;
    }

    InOutProjection.PrimaryCue = EMythicNameplatePrimaryCue::None;
    InOutProjection.ResolvedSubtitle = FText::GetEmpty();
    InOutProjection.AttentionState = EMythicNameplateAttentionState::Observed;
    InOutProjection.VisualFamily = EMythicNameplateVisualFamily::Identity;
    InOutProjection.bShowHealth = false;
    InOutProjection.HealthFraction = 0.0f;
    InOutProjection.bHealthPercentEligible = false;
    InOutProjection.bCombatCapable = false;
    InOutProjection.PresentedCombatRank =
        EMythicPresentedCombatRank::Unknown;
    InOutProjection.ThreatBand = EMythicThreatBand::Unknown;
    InOutProjection.bShowExactLevel = false;
    InOutProjection.CombatLevel = 0;
    InOutProjection.ResolvedLevelText = FText::GetEmpty();
    InOutProjection.Statuses.Reset();
    InOutProjection.StatusOverflowCount = 0;
}

bool FMythicNameplateRules::AreProjectionsEquivalent(
    const FMythicNameplateProjection &Left,
    const FMythicNameplateProjection &Right) {
    if (Left.Instance != Right.Instance
        || Left.DisclosureTier != Right.DisclosureTier
        || Left.VisualFamily != Right.VisualFamily
        || Left.AttentionState != Right.AttentionState
        || Left.Lane != Right.Lane
        || !Left.ResolvedName.EqualTo(Right.ResolvedName)
        || Left.PrimaryCue != Right.PrimaryCue
        || !Left.ResolvedSubtitle.EqualTo(Right.ResolvedSubtitle)
        || Left.bShowHealth != Right.bShowHealth
        || !FMath::IsNearlyEqual(Left.HealthFraction,
                                 Right.HealthFraction)
        || Left.bHealthPercentEligible != Right.bHealthPercentEligible
        || Left.bCombatCapable != Right.bCombatCapable
        || Left.PresentedCombatRank != Right.PresentedCombatRank
        || Left.ThreatBand != Right.ThreatBand
        || Left.bShowExactLevel != Right.bShowExactLevel
        || Left.CombatLevel != Right.CombatLevel
        || !Left.ResolvedLevelText.EqualTo(Right.ResolvedLevelText)
        || Left.StatusOverflowCount != Right.StatusOverflowCount
        || Left.Statuses.Num() != Right.Statuses.Num()) {
        return false;
    }

    for (int32 Index = 0; Index < Left.Statuses.Num(); ++Index) {
        const FMythicNameplateStatusCandidate &A = Left.Statuses[Index];
        const FMythicNameplateStatusCandidate &B = Right.Statuses[Index];
        if (A.StatusType != B.StatusType
            || !A.ResolvedLabel.EqualTo(B.ResolvedLabel)
            || A.Icon != B.Icon
            || !A.DisplayColor.Equals(B.DisplayColor)
            || A.Urgency != B.Urgency
            || A.bPresentationPermitted != B.bPresentationPermitted
            || A.bAppliedByViewerOrParty != B.bAppliedByViewerOrParty
            || A.AuthoredPriority != B.AuthoredPriority
            || A.StableTieBreak != B.StableTieBreak
            || A.StackCount != B.StackCount
            || !FMath::IsNearlyEqual(A.ServerEndTimeSeconds,
                                     B.ServerEndTimeSeconds)) {
            return false;
        }
    }

    return true;
}

bool FMythicNameplateRules::AreActionRailProjectionsEquivalent(
    const FMythicNameplateActionRailProjection &Left,
    const FMythicNameplateActionRailProjection &Right) {
    if (Left.Instance != Right.Instance
        || Left.bInspectAvailable != Right.bInspectAvailable
        || Left.InspectInputActionTag != Right.InspectInputActionTag
        || !Left.ResolvedInspectLabel.EqualTo(Right.ResolvedInspectLabel)
        || Left.Actions.Num() != Right.Actions.Num()) {
        return false;
    }
    for (int32 Index = 0; Index < Left.Actions.Num(); ++Index) {
        const FMythicNameplateActionProjection &A = Left.Actions[Index];
        const FMythicNameplateActionProjection &B = Right.Actions[Index];
        if (A.ActionTag != B.ActionTag
            || !A.ResolvedLabel.EqualTo(B.ResolvedLabel)
            || A.Icon != B.Icon
            || A.InputActionTag != B.InputActionTag
            || A.OfferRevision != B.OfferRevision
            || !FMath::IsNearlyEqual(A.HoldDurationSeconds,
                                     B.HoldDurationSeconds)) {
            return false;
        }
    }
    return true;
}

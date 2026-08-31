#pragma once

#include "CoreMinimal.h"
#include "UI/Nameplate/MythicNameplatePolicy.h"
#include "UI/Nameplate/MythicNameplateTypes.h"

enum class EMythicNameplatePresentationMode : uint8;

/** Allocation-free local render result for one passive distance profile. */
struct MYTHIC_API FMythicNameplateDistancePresentation {
    /** Distance-controlled alpha before temporal transition and collision suppression. */
    float Alpha = 0.0f;

    /** Distance-controlled scale before local accessibility scale. */
    float Scale = 1.0f;

    /** True at or beyond the profile's release edge, where the surface must relinquish its claim. */
    bool bBeyondRelease = true;
};

/**
 * Deterministic contextual-nameplate policy kernel. Callers supply already-redacted facts; these rules never inspect
 * actors, ability systems, widgets, local players, replication state, or private LivingWorld simulation data.
 */
struct MYTHIC_API FMythicNameplateRules {
    /** Resolves the richest currently eligible surface before dwell and hysteresis are applied. */
    static EMythicNameplateDisclosureTier ResolveDesiredDisclosure(
        const FMythicNameplateDisclosureEvidence &Evidence);

    /**
     * Applies the local presentation-mode preference after entitlement. Minimal may suppress optional surfaces;
     * Expanded never upgrades the tier because its extra density is applied separately after sanitization.
     */
    static EMythicNameplateDisclosureTier ApplyPresentationMode(
        EMythicNameplateDisclosureTier EntitledTier,
        EMythicNameplatePresentationMode Mode,
        bool bMandatorySafetyOrCurrentTarget);

    /**
     * Applies the passive acquire/release hysteresis gate. A new surface admits through AcquireDistance; an existing
     * passive incumbent remains admitted only while strictly inside ReleaseDistance.
     */
    static bool ShouldAdmitPassiveSurface(float DistanceCentimeters,
                                          float AcquireDistanceCentimeters,
                                          float ReleaseDistanceCentimeters,
                                          bool bPassiveIncumbent);

    /**
     * Resolves the smoothstep distance alpha and scale for an admitted passive surface. High Contrast or Reduced
     * Motion passes bSnapDistance=true to retain full alpha and unit scale until the unchanged release boundary.
     */
    static FMythicNameplateDistancePresentation ResolvePassiveDistancePresentation(
        float DistanceCentimeters, float FullDistanceCentimeters,
        float ReleaseDistanceCentimeters, float FullAlpha,
        float ReleaseScale, bool bSnapDistance);

    /** Resolves a local eased acquire/release alpha from a retained start alpha and transition start time. */
    static float ResolveTemporalAlpha(float StartAlpha, bool bReleasing,
                                      double TransitionStartSeconds,
                                      double NowSeconds,
                                      float AcquireTransitionSeconds,
                                      float ReleaseTransitionSeconds,
                                      bool bReducedMotion);

    /** Returns the binding semantic precedence of a mutually exclusive primary cue. */
    static int32 GetCuePrecedence(EMythicNameplatePrimaryCue Cue);

    /** Selects one headline from any candidate ordering using precedence and a stable enum tie-break. */
    static EMythicNameplatePrimaryCue SelectPrimaryCue(
        TConstArrayView<EMythicNameplatePrimaryCue> Candidates);

    /**
     * Resolves an observable fighting fact without treating viewer-outgoing combat as an attack on the viewer.
     * Incoming evidence must also be part of the current generic combat lease before hostility is inferred.
     */
    static EMythicNameplatePrimaryCue ResolveObservedFightingCue(
        bool bRecentCombatSignal, bool bRecentIncomingCombatSignal);

    /** Maps a selected cue to its reserved density lane, with deliberate focus always owning the Focus lane. */
    static EMythicNameplateLane ResolveLane(EMythicNameplatePrimaryCue Cue, bool bIsFocused);

    /** Applies the binding matrix that suppresses ambient health while retaining required combat and rescue reads. */
    static bool ShouldShowHealth(EMythicNameplateDisclosureTier Tier,
                                 const FMythicNameplateHealthContext &Context);

    /** Restricts exact combat level to permitted combatants on Focus/Inspect or the current Context combat target. */
    static bool ShouldShowExactLevel(EMythicNameplateDisclosureTier Tier,
                                     const FMythicNameplateLevelContext &Context);

    /** Returns the continuous evidence time required to promote into DesiredTier. */
    static float GetAcquireDwellSeconds(EMythicNameplateDisclosureTier DesiredTier,
                                        const FMythicEntityAttentionConfig &Attention);

    /** Returns the continuous loss time required to demote from CurrentTier. */
    static float GetReleaseGraceSeconds(EMythicNameplateDisclosureTier CurrentTier,
                                        const FMythicEntityAttentionConfig &Attention);

    /** Reports whether a richer desired tier has satisfied its acquisition dwell. */
    static bool ShouldPromote(EMythicNameplateDisclosureTier CurrentTier,
                              EMythicNameplateDisclosureTier DesiredTier,
                              float EvidenceHeldSeconds, const FMythicEntityAttentionConfig &Attention);

    /** Reports whether a poorer desired tier has satisfied the incumbent's release grace. */
    static bool ShouldDemote(EMythicNameplateDisclosureTier CurrentTier,
                             EMythicNameplateDisclosureTier DesiredTier,
                             float EvidenceLostSeconds, const FMythicEntityAttentionConfig &Attention);

    /** Returns whether a same-lane challenger currently holds the authored score advantage over an incumbent. */
    static bool HasReplacementScoreLead(
        float IncumbentScore, float ChallengerScore,
        const FMythicEntityAttentionConfig &Attention);

    /**
     * Reports whether a challenger may replace an occupied slot. A valid higher-lane challenger wins immediately;
     * a same-lane challenger must hold the configured multiplicative score advantage for the configured dwell.
     */
    static bool ShouldReplaceIncumbent(float IncumbentScore, float ChallengerScore,
                                       float ChallengerLeadSeconds, bool bChallengerFromHigherLane,
                                       const FMythicEntityAttentionConfig &Attention);

    /** Returns the configured hard reservation for Lane. */
    static int32 GetLaneCapacity(EMythicNameplateLane Lane, const FMythicNameplateCapacityPolicy &Capacity);

    /**
     * Returns the ambient identity budget after deliberate attention is known. A focused surface consumes the
     * viewer's identity attention budget, while tactical Safety and Opportunity surfaces remain unaffected.
     */
    static int32 GetAmbientWhisperCapacity(
        bool bHasFocusedSurface, EMythicNameplatePresentationMode Mode,
        const FMythicNameplateCapacityPolicy &Capacity);

    /** Returns the bounded status-icon capacity for Tier; Silent and Whisper intentionally return zero. */
    static int32 GetStatusIconCap(EMythicNameplateDisclosureTier Tier,
                                  const FMythicNameplateStatusPolicy &Statuses);

    /**
     * Resolves one priority-ordered screen-space surface against bounds already claimed by richer surfaces.
     * Awareness yields on collision; tactical lanes try a small bounded upward stack before yielding. The returned
     * bounds are unpadded authored bounds so padding is applied exactly once by each subsequent candidate.
     */
    static bool ResolveDeclutteredPlacement(
        FVector2D DesiredCenter, FVector2D LogicalFootprint,
        EMythicNameplateLane Lane, bool bWasSuppressed,
        float ScreenPixelsPerLogicalPixel,
        TConstArrayView<FBox2D> HigherPriorityBounds,
        const FMythicNameplateDeclutterPolicy &Declutter,
        FVector2D &OutResolvedCenter, FBox2D &OutResolvedBounds);

    /** Derives the only legal fixed layout family from already-redacted tactical and safety facts. */
    static EMythicNameplateVisualFamily ResolveVisualFamily(
        EMythicPresentedCombatRank PresentedRank,
        bool bCombatPresentationEarned,
        bool bUrgentProtectedAlly);

    /**
     * Filters unauthorized or invalid statuses, applies a total deterministic order, copies at most MaxIcons, and
     * reports how many other eligible statuses collapsed into the overflow count.
     */
    static void SelectStatusCandidates(TConstArrayView<FMythicNameplateStatusCandidate> Candidates,
                                       int32 MaxIcons,
                                       TArray<FMythicNameplateStatusCandidate> &OutSelected,
                                       int32 &OutOverflowCount);

    /**
     * Enforces terminal-life-state and disclosure boundaries after every producer has contributed. Dead projections
     * fail closed to Silent; a separately-built corpse action context must replace the Dead cue before sanitization.
     */
    static void SanitizeProjectionForPresentation(
        FMythicNameplateProjection &InOutProjection);

    /** Compares every widget-visible or executable projection field, ignoring placement and internal ranking state. */
    static bool AreProjectionsEquivalent(
        const FMythicNameplateProjection &Left,
        const FMythicNameplateProjection &Right);

    /** Compares every renderable or executable field on two exact-subject action-rail projections. */
    static bool AreActionRailProjectionsEquivalent(
        const FMythicNameplateActionRailProjection &Left,
        const FMythicNameplateActionRailProjection &Right);
};

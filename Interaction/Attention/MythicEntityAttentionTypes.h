#pragma once

#include "CoreMinimal.h"
#include "World/Entity/MythicEntityPresentationTypes.h"

#include "MythicEntityAttentionTypes.generated.h"

class AActor;
class UMythicEntityPresentationComponent;

/** Semantic urgency observed by attention; final UI lane and disclosure policy remain downstream decisions. */
UENUM(BlueprintType)
enum class EMythicEntityAttentionPriorityClass : uint8 {
    /** Incidental proximity or gaze with no event-backed relevance. */
    Ambient,

    /** An observable world-state change worth low-pressure awareness. */
    Awareness,

    /** A viewer-relevant interaction, dialogue, quest, or service opportunity. */
    Opportunity,

    /** Immediate combat, rescue, or other survival relevance. */
    Safety,
};

/** Short-lived event edge accepted by the attention service without interpreting private simulation state. */
UENUM(BlueprintType)
enum class EMythicEntityAttentionSignalKind : uint8 {
    /** Publicly observable activity or state change. */
    Awareness,

    /** A viewer-specific interaction or service became actionable. */
    Opportunity,

    /** A survival-relevant state such as danger, downing, or rescue. */
    Safety,

    /** Recent combat relevance; contributes to Safety and is retained as explicit evidence. */
    Combat,

    /** The subject acted against this viewer; this directional edge may support an attacking-viewer cue. */
    CombatFromSubjectToViewer,

    /** Recent contextual-action relevance; contributes to Opportunity and is retained as explicit evidence. */
    Action,
};

/** Central runtime tuning for one LocalPlayer attention service. A policy/bootstrap owner may replace it atomically. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicEntityAttentionConfig {
    GENERATED_BODY()

    /** Frequency of scheduled candidate decisions; event edges may request an earlier bounded pass. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attention|Cadence", meta = (ClampMin = "1.0", ClampMax = "10.0", Units = "Hz"))
    float DecisionRateHz = 10.0f;

    /** Minimum spacing between event-driven passes so an event storm cannot turn attention into a per-frame scan. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attention|Cadence", meta = (ClampMin = "0.0", ClampMax = "0.1", Units = "s"))
    float MinimumImmediatePassIntervalSeconds = 0.033f;

    /** Maximum cheap candidates retained for screen and visibility evaluation after the registry census. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attention|Budgets", meta = (ClampMin = "16", ClampMax = "128"))
    int32 MaxEvaluatedCandidates = 32;

    /** Maximum observations published to consumers; sixteen matches the shared presentation pool. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attention|Budgets", meta = (ClampMin = "1", ClampMax = "16"))
    int32 MaxPublishedObservations = 16;

    /** Hard upper bound on expensive line-of-sight traces during one decision pass. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attention|Budgets", meta = (ClampMin = "1", ClampMax = "8"))
    int32 MaxLineOfSightTracesPerPass = 8;

    /** Maximum presentation actors consumed from the gaze/personal-space collision broadphase in one pass. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attention|Budgets", meta = (ClampMin = "8", ClampMax = "256"))
    int32 MaxSpatialCandidatesPerPass = 64;

    /** Maximum active event subjects retained per viewer; lower-priority signals fail closed when this budget is full. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attention|Budgets", meta = (ClampMin = "8", ClampMax = "128"))
    int32 MaxRetainedEventSubjects = 64;

    /** Registered entries missed by spatial probes checked per pass as a bounded compatibility seed, never a full census. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attention|Budgets", meta = (ClampMin = "0", ClampMax = "64"))
    int32 MaxRegistryFallbackChecksPerPass = 8;

    /** Furthest ordinary entity considered by the local attention service. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attention|Range", meta = (ClampMin = "100.0", Units = "cm"))
    float MaximumAttentionDistanceCentimeters = 6000.0f;

    /** Personal-space radius exposed as ambient evidence, not automatic presentation permission. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attention|Range", meta = (ClampMin = "0.0", Units = "cm"))
    float AmbientPersonalSpaceDistanceCentimeters = 400.0f;

    /** Radius of the camera-forward collision sweep that discovers deliberate gaze targets without a world census. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attention|Range", meta = (ClampMin = "1.0", ClampMax = "300.0", Units = "cm"))
    float GazeProbeRadiusCentimeters = 90.0f;

    /** Minimum camera alignment for stable gaze evidence used by identity-only presentation policy. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attention|Focus", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    float GazeMinimumViewDot = 0.35f;

    /** Wider retention cone used only after a subject has entered recent gaze residency, preventing threshold chatter. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attention|Focus", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    float GazeReleaseMinimumViewDot = 0.30f;

    /** Minimum camera alignment for an ordinary subject to challenge for deliberate focus. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attention|Focus", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    float FocusMinimumViewDot = 0.92f;

    /** Stable ordinary attention required to acquire the single focused entity. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attention|Focus", meta = (ClampMin = "0.0", Units = "s"))
    float FocusAcquireDwellSeconds = 0.12f;

    /** Stable gaze required before downstream policy may disclose an identity-only Whisper. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attention|Focus", meta = (ClampMin = "0.0", Units = "s"))
    float WhisperAcquireDwellSeconds = 0.12f;

    /** Time an existing focus survives a transient loss of ordinary focus eligibility. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attention|Focus", meta = (ClampMin = "0.0", Units = "s"))
    float FocusReleaseGraceSeconds = 0.14f;

    /** Time a stronger ordinary challenger must remain stable before replacing the incumbent focus. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attention|Focus", meta = (ClampMin = "0.0", Units = "s"))
    float ReplacementDwellSeconds = 0.09f;

    /** Score multiplier an ordinary challenger must exceed before replacement dwell begins. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attention|Focus", meta = (ClampMin = "1.0"))
    float ReplacementScoreMultiplier = 1.15f;

    /** Grace before a non-Focus presentation slot is released after eligibility disappears. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attention|Focus", meta = (ClampMin = "0.0", Units = "s"))
    float SlotReleaseGraceSeconds = 0.40f;

    /** Symmetric world-distance deadband used by downstream presentation thresholds. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attention|Range", meta = (ClampMin = "0.0", Units = "cm"))
    float DistanceDeadbandCentimeters = 200.0f;

    /** Maximum age of a cached positive or negative line-of-sight result. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attention|Visibility", meta = (ClampMin = "0.0", Units = "s"))
    float LineOfSightCacheLifetimeSeconds = 0.25f;

    /** Viewer or subject movement beyond this distance invalidates a cached line-of-sight result early. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attention|Visibility", meta = (ClampMin = "0.0", Units = "cm"))
    float LineOfSightMovementInvalidationCentimeters = 75.0f;

    /** Continuous occlusion required before a previously visible ordinary presentation is hidden. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attention|Visibility", meta = (ClampMin = "0.0", Units = "s"))
    float OcclusionHideGraceSeconds = 0.12f;

    /** Continuous visibility required before a previously occluded presentation is revealed. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attention|Visibility", meta = (ClampMin = "0.0", Units = "s"))
    float OcclusionRevealGraceSeconds = 0.08f;

    /** Maximum dedicated edge indicators downstream policy may reserve for critical offscreen exceptions. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attention|Visibility", meta = (ClampMin = "0", ClampMax = "8"))
    int32 MaxCriticalEdgeIndicators = 4;

    /** Extra player-viewport pixels tolerated before a projected anchor is classified as offscreen. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attention|Visibility", meta = (ClampMin = "0.0"))
    float ScreenEdgePaddingPixels = 24.0f;

    /** Score contribution of camera alignment after normalization to the visible forward hemisphere. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attention|Scoring", meta = (ClampMin = "0.0"))
    float ViewAlignmentWeight = 1000.0f;

    /** Score contribution of proximity after normalization by MaximumAttentionDistanceCentimeters. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attention|Scoring", meta = (ClampMin = "0.0"))
    float ProximityWeight = 450.0f;

    /** Bonus for an anchor projected inside this LocalPlayer's own viewport. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attention|Scoring", meta = (ClampMin = "0.0"))
    float OnScreenBonus = 250.0f;

    /** Bonus for a current positive visibility result. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attention|Scoring", meta = (ClampMin = "0.0"))
    float LineOfSightBonus = 300.0f;

    /** Base score for awareness-class event evidence. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attention|Scoring", meta = (ClampMin = "0.0"))
    float AwarenessPriorityBonus = 1800.0f;

    /** Base score for opportunity-class event evidence. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attention|Scoring", meta = (ClampMin = "0.0"))
    float OpportunityPriorityBonus = 3600.0f;

    /** Base score for safety-class event evidence. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attention|Scoring", meta = (ClampMin = "0.0"))
    float SafetyPriorityBonus = 6000.0f;

    /** Dominant score bonus for a validated interaction-target override. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attention|Scoring", meta = (ClampMin = "0.0"))
    float InteractionOverrideBonus = 8000.0f;

    /** Dominant score bonus for a current hard-combat-target override. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attention|Scoring", meta = (ClampMin = "0.0"))
    float HardTargetOverrideBonus = 10000.0f;

    /** Highest score bonus for an explicit inspect target. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attention|Scoring", meta = (ClampMin = "0.0"))
    float InspectOverrideBonus = 12000.0f;
};

/** One bounded, viewer-local observation consumed by nameplates, interaction, targeting, and accessibility layers. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicEntityAttentionObservation {
    GENERATED_BODY()

    /** Exact opaque embodiment handle; delayed consumers must reject it after generation changes. */
    UPROPERTY(BlueprintReadOnly, Category = "Attention")
    FMythicEntityPresentationInstance Instance;

    /** Highest semantic urgency currently evidenced for this viewer, before final UI policy. */
    UPROPERTY(BlueprintReadOnly, Category = "Attention")
    EMythicEntityAttentionPriorityClass PriorityClass = EMythicEntityAttentionPriorityClass::Ambient;

    /** Stable within-pass ordering score; consumers should still apply their own lane capacities and policy. */
    UPROPERTY(BlueprintReadOnly, Category = "Attention")
    float Score = 0.0f;

    /** Distance from the local camera to the presentation anchor in centimeters. */
    UPROPERTY(BlueprintReadOnly, Category = "Attention", meta = (Units = "cm"))
    float DistanceCentimeters = 0.0f;

    /** Camera-forward alignment in [-1,1], where one is centered directly ahead. */
    UPROPERTY(BlueprintReadOnly, Category = "Attention", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    float ViewAlignment = -1.0f;

    /** Anchor position relative to this LocalPlayer's viewport, suitable for a pooled world-overlay projection. */
    UPROPERTY(BlueprintReadOnly, Category = "Attention")
    FVector2D ScreenPosition = FVector2D::ZeroVector;

    /** World position sampled for scoring, visibility, and projection during this decision pass. */
    UPROPERTY(BlueprintReadOnly, Category = "Attention")
    FVector AnchorWorldLocation = FVector::ZeroVector;

    /** Continuous eligible gaze duration for downstream whisper/focus dwell decisions. */
    UPROPERTY(BlueprintReadOnly, Category = "Attention", meta = (ClampMin = "0.0", Units = "s"))
    float StableGazeSeconds = 0.0f;

    /** Whether the anchor currently projects inside this LocalPlayer's own split-screen viewport. */
    UPROPERTY(BlueprintReadOnly, Category = "Attention")
    bool bOnScreen = false;

    /** Whether the latest bounded visibility sample reaches the subject without obstruction. */
    UPROPERTY(BlueprintReadOnly, Category = "Attention")
    bool bHasLineOfSight = false;

    /** Whether this is the service's one stable focused subject. */
    UPROPERTY(BlueprintReadOnly, Category = "Attention")
    bool bFocused = false;

    /** Whether current gaze evidence meets the configured ordinary gaze threshold. */
    UPROPERTY(BlueprintReadOnly, Category = "Attention")
    bool bGazeCandidate = false;

    /** Whether distance is inside the configured personal-space radius. */
    UPROPERTY(BlueprintReadOnly, Category = "Attention")
    bool bWithinAmbientRange = false;

    /** Whether the legacy/shared interaction selector explicitly owns this subject. */
    UPROPERTY(BlueprintReadOnly, Category = "Attention")
    bool bInteractionTarget = false;

    /** Whether the combat targeting layer explicitly owns this subject. */
    UPROPERTY(BlueprintReadOnly, Category = "Attention")
    bool bHardTarget = false;

    /** Whether the viewer deliberately selected this subject for dossier inspection. */
    UPROPERTY(BlueprintReadOnly, Category = "Attention")
    bool bInspectTarget = false;

    /** Whether a nonexpired Safety signal is present. */
    UPROPERTY(BlueprintReadOnly, Category = "Attention")
    bool bSafetySignal = false;

    /** Whether a nonexpired Opportunity or Action signal is present. */
    UPROPERTY(BlueprintReadOnly, Category = "Attention")
    bool bOpportunitySignal = false;

    /** Whether a nonexpired Awareness signal is present. */
    UPROPERTY(BlueprintReadOnly, Category = "Attention")
    bool bAwarenessSignal = false;

    /** Whether combat relevance was refreshed within its supplied event lifetime. */
    UPROPERTY(BlueprintReadOnly, Category = "Attention")
    bool bRecentCombatSignal = false;

    /** Whether recent directional evidence says this subject acted against this viewer. */
    UPROPERTY(BlueprintReadOnly, Category = "Attention")
    bool bRecentIncomingCombatSignal = false;

    /** Whether contextual-action relevance was refreshed within its supplied event lifetime. */
    UPROPERTY(BlueprintReadOnly, Category = "Attention")
    bool bRecentActionSignal = false;

    /** Whether an executed public behavior fact supplies awareness evidence without revealing private intent. */
    UPROPERTY(BlueprintReadOnly, Category = "Attention")
    bool bPublicAwarenessEvidence = false;

    /** Whether a replicated downed, dying, or dead fact supplies safety evidence. */
    UPROPERTY(BlueprintReadOnly, Category = "Attention")
    bool bPublicSafetyEvidence = false;

    /** Whether a canonical publicly projected status is authored to promote observed safety context. */
    UPROPERTY(BlueprintReadOnly, Category = "Attention")
    bool bSafetyCriticalStatus = false;

    // Non-owning native resolution only. These handles are intentionally not reflected, replicated, or persisted.
    TWeakObjectPtr<AActor> Actor;
    TWeakObjectPtr<UMythicEntityPresentationComponent> Component;
};

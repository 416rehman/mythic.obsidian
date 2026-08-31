#pragma once

#include "CoreMinimal.h"
#include "GAS/Combat/MythicCombatThreatAssessment.h"
#include "GAS/Combat/MythicEntityCombatPresentationComponent.h"
#include "GameplayTagContainer.h"
#include "World/Entity/MythicEntityPresentationTypes.h"
#include "MythicNameplateTypes.generated.h"

class UTexture2D;

/** Progressive-disclosure surface currently earned by an entity for one local viewer. */
UENUM(BlueprintType)
enum class EMythicNameplateDisclosureTier : uint8 {
    /** No world-space presentation is eligible. */
    Silent,

    /** Identity only: a known name, species, or knowledge-safe fallback. */
    Whisper,

    /** A compact plate carrying one primary contextual cue and conditional combat information. */
    Context,

    /** The single deliberate target read with richer, still-bounded detail. */
    Focus,
};

/** Small fixed layout family derived after disclosure and privacy resolution; it is never authored per entity. */
UENUM(BlueprintType)
enum class EMythicNameplateVisualFamily : uint8 {
    /** Peaceful or ambient identity with no tactical resource read. */
    Identity,

    /** Standard through Champion tactical presentation. */
    Combat,

    /** Urgent protected-ally presentation such as a valid revive or rescue. */
    AllySafety,

    /** Authority-presented Boss or WorldBoss tactical presentation. */
    Boss,
};

/** Viewer-local attention channel used only to emphasize an already-entitled projection. */
UENUM(BlueprintType)
enum class EMythicNameplateAttentionState : uint8 {
    /** No deliberate target emphasis. */
    None,

    /** Visible ambient observation without deliberate focus. */
    Observed,

    /** Stable cursor, reticle, or controller focus. */
    Focused,

    /** Explicit interaction target; acquisition is immediate. */
    InteractionTarget,

    /** Soft combat selection supplied by targeting policy. */
    SoftCombatTarget,

    /** Explicit hard combat target; acquisition is immediate. */
    HardCombatTarget,

    /** Server-supported combat lock target. */
    LockedCombatTarget,

    /** Active dialogue target; dialogue normally suppresses the overhead surface. */
    DialogueTarget,
};

/** Reserved screen-density lane used before score ordering so safety cannot be evicted by ambient awareness. */
UENUM(BlueprintType)
enum class EMythicNameplateLane : uint8 {
    /** The one deliberate focus target. */
    Focus,

    /** Immediate survival information such as a downed ally or an entity attacking the viewer. */
    Safety,

    /** Actionable dialogue, quest, service, or other interaction opportunity. */
    Opportunity,

    /** Low-pressure identity and observable world-state information. */
    Awareness,
};

/** Mutually exclusive headline selected for a compact plate; numeric order is not the precedence contract. */
UENUM(BlueprintType)
enum class EMythicNameplatePrimaryCue : uint8 {
    /** No contextual headline is available. */
    None,

    /** Known public faction or affiliation. */
    Faction,

    /** Public or learned role such as guard, healer, or craftsperson. */
    Role,

    /** Observable activity that has no more urgent semantic meaning. */
    ObservableActivity,

    /** A contextual action not covered by dialogue, quest, or service semantics. */
    OtherAction,

    /** A currently available service such as trade, repair, or training. */
    Service,

    /** A quest offer that is currently assignable to this viewer. */
    QuestOffer,

    /** A quest step that this viewer can currently turn in. */
    QuestTurnIn,

    /** An outward, directed request to speak with this viewer. */
    DirectedTalk,

    /** An observable surrender already being performed. */
    Surrendering,

    /** Observable flight already being performed, not a private intention to flee. */
    Fleeing,

    /** The entity has publicly committed an attack against this viewer. */
    AttackingViewer,

    /** The entity is in an observable dying state. */
    Dying,

    /** The entity is downed and may require immediate intervention. */
    Downed,

    /** The entity is dead; corpse interaction may subsequently own presentation. */
    Dead,
};

/** Tactical class used as the first deterministic key when bounded status icons are selected. */
UENUM(BlueprintType)
enum class EMythicNameplateStatusUrgency : uint8 {
    /** Cosmetic or low-value state that is normally visible only on deliberate focus. */
    Other,

    /** Movement impairment or soft control. */
    MovementControl,

    /** Non-lethal damaging status. */
    Damaging,

    /** Viewer-relevant setup or exploit window. */
    ViewerExploitable,

    /** Damage over time assessed as immediately lethal or critical. */
    LethalDamageOverTime,

    /** Incapacitation or other hard crowd control. */
    HardCrowdControl,
};

/**
 * Local accessibility and skin preferences applied after gameplay disclosure has produced a viewer-safe projection.
 * These values never grant recognition, status visibility, health visibility, or any other information entitlement.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicNameplateRenderPreferences {
    GENERATED_BODY()

    /** Uniform local scale for overhead plates and the contextual action rail, clamped to 0.75..1.5. */
    UPROPERTY(BlueprintReadOnly, Category = "Nameplate|Accessibility",
              meta = (ClampMin = "0.75", ClampMax = "1.5"))
    float Scale = 1.0f;

    /** Whether an already-entitled Focus/current-target health bar may add a rounded percentage label. */
    UPROPERTY(BlueprintReadOnly, Category = "Nameplate|Accessibility")
    bool bShowHealthPercent = false;

    /** Whether visible status icons/shapes may add their short localized labels. */
    UPROPERTY(BlueprintReadOnly, Category = "Nameplate|Accessibility")
    bool bShowStatusText = false;

    /** Whether already-entitled plate content uses the stronger content-local accessibility scrim. */
    UPROPERTY(BlueprintReadOnly, Category = "Nameplate|Accessibility")
    bool bHighContrast = false;

    /** Whether cosmetic interpolation should snap while semantic dwell, leases, and authority timing remain unchanged. */
    UPROPERTY(BlueprintReadOnly, Category = "Nameplate|Accessibility")
    bool bReducedMotion = false;

    bool operator==(const FMythicNameplateRenderPreferences &Other) const {
        return FMath::IsNearlyEqual(Scale, Other.Scale)
            && bShowHealthPercent == Other.bShowHealthPercent
            && bShowStatusText == Other.bShowStatusText
            && bHighContrast == Other.bHighContrast
            && bReducedMotion == Other.bReducedMotion;
    }

    bool operator!=(const FMythicNameplateRenderPreferences &Other) const {
        return !(*this == Other);
    }
};

/** Already-redacted attention evidence consumed by the pure disclosure resolver. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicNameplateDisclosureEvidence {
    GENERATED_BODY()

    /** Whether this viewer may receive any presentation for the subject. */
    UPROPERTY(BlueprintReadWrite, Category = "Nameplate|Attention")
    bool bPresentationPermitted = false;

    /** Whether an unobstructed presentation trace currently reaches the subject. */
    UPROPERTY(BlueprintReadWrite, Category = "Nameplate|Attention")
    bool bHasLineOfSight = false;

    /** Whether the subject is the viewer's current hard combat target. */
    UPROPERTY(BlueprintReadWrite, Category = "Nameplate|Attention")
    bool bHardTarget = false;

    /** Whether the shared targeting service selected the subject for interaction. */
    UPROPERTY(BlueprintReadWrite, Category = "Nameplate|Attention")
    bool bInteractionTarget = false;

    /** Whether stable reticle, cursor, or controller attention earned a deliberate focus read. */
    UPROPERTY(BlueprintReadWrite, Category = "Nameplate|Attention")
    bool bFocusAttention = false;

    /** Whether a combat, communication, objective, or observable-world event earned contextual presentation. */
    UPROPERTY(BlueprintReadWrite, Category = "Nameplate|Attention")
    bool bContextSignal = false;

    /** Whether a stable visible glance earned identity-only presentation. */
    UPROPERTY(BlueprintReadWrite, Category = "Nameplate|Attention")
    bool bGazeAttention = false;

    /** Whether the one constrained personal-space candidate earned identity-only presentation. */
    UPROPERTY(BlueprintReadWrite, Category = "Nameplate|Attention")
    bool bPersonalSpaceAttention = false;
};

/** Redacted facts used by the health-visibility matrix; no actor or attribute lookup occurs in the rule. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicNameplateHealthContext {
    GENERATED_BODY()

    /** Whether this viewer is allowed to receive the subject's health presentation. */
    UPROPERTY(BlueprintReadWrite, Category = "Nameplate|Health")
    bool bHealthPresentationPermitted = false;

    /** Whether the subject is the viewer's current hard or soft combat target. */
    UPROPERTY(BlueprintReadWrite, Category = "Nameplate|Health")
    bool bCurrentCombatTarget = false;

    /** Whether a dedicated encounter policy identifies the subject as a boss. */
    UPROPERTY(BlueprintReadWrite, Category = "Nameplate|Health")
    bool bBoss = false;

    /** Whether the subject is downed or dying. */
    UPROPERTY(BlueprintReadWrite, Category = "Nameplate|Health")
    bool bDownedOrDying = false;

    /** Whether the subject is currently relevant to this viewer's combat. */
    UPROPERTY(BlueprintReadWrite, Category = "Nameplate|Health")
    bool bCombatRelevant = false;

    /** Whether current health is below maximum after authoritative tolerance rules. */
    UPROPERTY(BlueprintReadWrite, Category = "Nameplate|Health")
    bool bInjured = false;

    /** Whether the subject is a party member, companion, escort, or other protected ally. */
    UPROPERTY(BlueprintReadWrite, Category = "Nameplate|Health")
    bool bPartyOrCompanion = false;

    /** Whether this viewer currently has a validated assist, heal, rescue, or revive action. */
    UPROPERTY(BlueprintReadWrite, Category = "Nameplate|Health")
    bool bCanAssist = false;
};

/** Permission and relevance facts used to prevent exact combat levels from becoming ambient world labels. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicNameplateLevelContext {
    GENERATED_BODY()

    /** Whether this viewer may receive exact combat-level information for the subject. */
    UPROPERTY(BlueprintReadWrite, Category = "Nameplate|Level")
    bool bExactLevelPermitted = false;

    /** Whether the subject is actually combat-capable rather than a territory-scaled civilian. */
    UPROPERTY(BlueprintReadWrite, Category = "Nameplate|Level")
    bool bCombatCapable = false;

    /** Whether the subject is the viewer's current hard or soft combat target. */
    UPROPERTY(BlueprintReadWrite, Category = "Nameplate|Level")
    bool bCurrentCombatTarget = false;
};

/** Fully redacted status candidate supplied to deterministic bounded selection. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicNameplateStatusCandidate {
    GENERATED_BODY()

    /** Canonical Status.Type.* identity used only as a stable semantic tie-break and renderer lookup key. */
    UPROPERTY(BlueprintReadWrite, Category = "Nameplate|Status", meta = (Categories = "Status.Type"))
    FGameplayTag StatusType;

    /** Localized canonical status label resolved before the immutable projection reaches a widget. */
    UPROPERTY(BlueprintReadWrite, Category = "Nameplate|Status")
    FText ResolvedLabel;

    /** Resident canonical status icon resolved locally from its one definition; projections never carry asset paths. */
    UPROPERTY(BlueprintReadWrite, Category = "Nameplate|Status")
    TObjectPtr<UTexture2D> Icon;

    /** Canonical authored status glyph tint used by badges without another registry lookup. */
    UPROPERTY(BlueprintReadWrite, Category = "Nameplate|Status")
    FLinearColor DisplayColor = FLinearColor::White;

    /** Tactical urgency already resolved from the canonical status definition and viewer context. */
    UPROPERTY(BlueprintReadWrite, Category = "Nameplate|Status")
    EMythicNameplateStatusUrgency Urgency = EMythicNameplateStatusUrgency::Other;

    /** Whether this viewer is permitted to see the status on the current presentation surface. */
    UPROPERTY(BlueprintReadWrite, Category = "Nameplate|Status")
    bool bPresentationPermitted = false;

    /** Whether the status was applied by this viewer or their party. */
    UPROPERTY(BlueprintReadWrite, Category = "Nameplate|Status")
    bool bAppliedByViewerOrParty = false;

    /** Designer-authored priority within the same tactical urgency class; larger values win. */
    UPROPERTY(BlueprintReadWrite, Category = "Nameplate|Status")
    int32 AuthoredPriority = 0;

    /** Stable nonnegative order supplied by the status source before selection; smaller values win exact ties. */
    UPROPERTY(BlueprintReadWrite, Category = "Nameplate|Status", meta = (ClampMin = "0"))
    int32 StableTieBreak = 0;

    /** Bounded public stack count for rendering; zero means the canonical status definition hides stacks. */
    UPROPERTY(BlueprintReadWrite, Category = "Nameplate|Status", meta = (ClampMin = "0", ClampMax = "255"))
    int32 StackCount = 0;

    /** Synchronized server-time deadline; zero means duration is unknown or intentionally hidden. */
    UPROPERTY(BlueprintReadWrite, Category = "Nameplate|Status", meta = (Units = "s"))
    double ServerEndTimeSeconds = 0.0;
};

/** One available viewer-safe action rendered by the focused entity's separate action rail. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicNameplateActionProjection {
    GENERATED_BODY()

    /** Canonical Context.Action.* identity used for definition lookup and execution. */
    UPROPERTY(BlueprintReadOnly, Category = "Nameplate|Action", meta = (Categories = "Context.Action"))
    FGameplayTag ActionTag;

    /** Localized label resolved from the canonical context-action definition without a raw-tag fallback. */
    UPROPERTY(BlueprintReadOnly, Category = "Nameplate|Action")
    FText ResolvedLabel;

    /** Resident canonical action icon resolved locally from its one definition; projections never carry asset paths. */
    UPROPERTY(BlueprintReadOnly, Category = "Nameplate|Action")
    TObjectPtr<UTexture2D> Icon;

    /** CommonUI UI.Action.* key used to resolve the current device glyph. */
    UPROPERTY(BlueprintReadOnly, Category = "Nameplate|Action", meta = (Categories = "UI.Action"))
    FGameplayTag InputActionTag;

    /** Opaque provider revision echoed by native execution so authority can reject a stale offer. */
    UPROPERTY()
    uint32 OfferRevision = 0;

    /** Continuous input hold required by the canonical action definition; zero means tap. */
    UPROPERTY(BlueprintReadOnly, Category = "Nameplate|Action", meta = (ClampMin = "0.0", Units = "s"))
    float HoldDurationSeconds = 0.0f;
};

/** Exact-subject, available-only action projection consumed by the one prewarmed contextual action rail. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicNameplateActionRailProjection {
    GENERATED_BODY()

    /** Exact opaque handle-generation pair; a focus or embodiment change invalidates the entire rail. */
    UPROPERTY(BlueprintReadOnly, Category = "Nameplate|Action Rail")
    FMythicEntityPresentationInstance Instance;

    /** Available actions only, priority sorted and hard-capped to two by native policy. */
    UPROPERTY(BlueprintReadOnly, Category = "Nameplate|Action Rail")
    TArray<FMythicNameplateActionProjection> Actions;

    /** Whether the learned-knowledge dossier may be opened for this exact subject. */
    UPROPERTY(BlueprintReadOnly, Category = "Nameplate|Action Rail")
    bool bInspectAvailable = false;

    /** Current-device CommonUI action used for the Inspect hold glyph. */
    UPROPERTY(BlueprintReadOnly, Category = "Nameplate|Action Rail", meta = (Categories = "UI.Action"))
    FGameplayTag InspectInputActionTag;

    /** Localized Inspect verb resolved before the rail reaches the widget. */
    UPROPERTY(BlueprintReadOnly, Category = "Nameplate|Action Rail")
    FText ResolvedInspectLabel;

    /** Returns true only when an exact subject has at least one available action or an entitled Inspect action. */
    bool IsPresentable() const {
        return Instance.IsValid() && (!Actions.IsEmpty() || bInspectAvailable);
    }
};

/**
 * Immutable local projection consumed by pooled nameplate widgets. It contains viewer-safe categorical and formatted
 * output only: no canonical entity ID, raw health, raw combat pressure, private LivingWorld state, or object pointer.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicNameplateProjection {
    GENERATED_BODY()

    /** Exact opaque handle-generation pair that makes delayed work and pooled-actor reuse rejectable. */
    UPROPERTY(BlueprintReadOnly, Category = "Nameplate")
    FMythicEntityPresentationInstance Instance;

    /** Maximum viewer-safe disclosure currently earned by this viewer. */
    UPROPERTY(BlueprintReadOnly, Category = "Nameplate")
    EMythicNameplateDisclosureTier DisclosureTier =
        EMythicNameplateDisclosureTier::Silent;

    /** Native-derived fixed layout family; Blueprint may skin it but cannot choose it. */
    UPROPERTY(BlueprintReadOnly, Category = "Nameplate")
    EMythicNameplateVisualFamily VisualFamily =
        EMythicNameplateVisualFamily::Identity;

    /** Viewer-local attention/target emphasis independent of disclosure and combat rank. */
    UPROPERTY(BlueprintReadOnly, Category = "Nameplate")
    EMythicNameplateAttentionState AttentionState =
        EMythicNameplateAttentionState::None;

    /** Reserved crowd-density lane assigned before within-lane scoring. */
    UPROPERTY(BlueprintReadOnly, Category = "Nameplate")
    EMythicNameplateLane Lane = EMythicNameplateLane::Awareness;

    /** Viewer-safe localized name, species, or learned fallback; empty suppresses identity text. */
    UPROPERTY(BlueprintReadOnly, Category = "Nameplate|Identity")
    FText ResolvedName;

    /** One mutually exclusive semantic headline selected by binding cue precedence. */
    UPROPERTY(BlueprintReadOnly, Category = "Nameplate|Context")
    EMythicNameplatePrimaryCue PrimaryCue = EMythicNameplatePrimaryCue::None;

    /** Single learned peaceful subtitle such as role plus faction; combat families always suppress it. */
    UPROPERTY(BlueprintReadOnly, Category = "Nameplate|Identity")
    FText ResolvedSubtitle;

    /** Whether the widget may draw HealthFraction on the current tier. */
    UPROPERTY(BlueprintReadOnly, Category = "Nameplate|Health")
    bool bShowHealth = false;

    /** Sanitized current-health fraction in [0,1]; exact current and maximum health never enter the projection. */
    UPROPERTY(BlueprintReadOnly, Category = "Nameplate|Health", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float HealthFraction = 0.0f;

    /** Whether local accessibility settings may add a rounded percent to this already-entitled health bar. */
    UPROPERTY(BlueprintReadOnly, Category = "Nameplate|Health")
    bool bHealthPercentEligible = false;

    /** Whether authority permits this viewer to know the subject is combat-capable; civilians remain unlabelled. */
    UPROPERTY(BlueprintReadOnly, Category = "Nameplate|Threat")
    bool bCombatCapable = false;

    /** Authority-redacted combat rank; Unknown prevents any rank claim or styling. */
    UPROPERTY(BlueprintReadOnly, Category = "Nameplate|Threat")
    EMythicPresentedCombatRank PresentedCombatRank =
        EMythicPresentedCombatRank::Unknown;

    /** Combat-owned viewer-relative danger category; no raw combat pressure is retained. */
    UPROPERTY(BlueprintReadOnly, Category = "Nameplate|Threat")
    EMythicThreatBand ThreatBand = EMythicThreatBand::Unknown;

    /** Whether CombatLevel is permitted on this tier for this combat-capable subject. */
    UPROPERTY(BlueprintReadOnly, Category = "Nameplate|Threat")
    bool bShowExactLevel = false;

    /** Exact combat level used only when bShowExactLevel is true; noncombatants must leave it at zero. */
    UPROPERTY(BlueprintReadOnly, Category = "Nameplate|Threat", meta = (ClampMin = "0"))
    int32 CombatLevel = 0;

    /** Localized compact level text resolved by the director; empty suppresses the level slot. */
    UPROPERTY(BlueprintReadOnly, Category = "Nameplate|Threat")
    FText ResolvedLevelText;

    /** Already-filtered and deterministically sorted status rows bounded for the current tier. */
    UPROPERTY(BlueprintReadOnly, Category = "Nameplate|Status")
    TArray<FMythicNameplateStatusCandidate> Statuses;

    /** Eligible status rows omitted by the tier cap and represented by the optional +N label. */
    UPROPERTY(BlueprintReadOnly, Category = "Nameplate|Status", meta = (ClampMin = "0"))
    int32 StatusOverflowCount = 0;

    /** Returns true only for a current subject instance with an earned non-Silent tier. */
    bool IsPresentable() const {
        return Instance.IsValid()
            && DisclosureTier != EMythicNameplateDisclosureTier::Silent;
    }
};

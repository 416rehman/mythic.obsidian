#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Interaction/Attention/MythicEntityAttentionTypes.h"
#include "MythicNameplatePolicy.generated.h"

class UMythicEntityInspectPage;
class UMythicNameplateVisualStyle;

/** Fixed pool and on-screen lane reservations used to degrade predictably during crowd spikes. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicNameplateCapacityPolicy {
    GENERATED_BODY()

    /** Widgets prewarmed at HUD construction; runtime presentation must never exceed this pool. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capacity", meta = (ClampMin = "1"))
    int32 PoolSize = 16;

    /** Maximum simultaneously drawn plates across all lanes; overflow is dropped rather than shrunk. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capacity", meta = (ClampMin = "1"))
    int32 MaxDrawnPlates = 12;

    /** Pool entries reserved for fade-out leases so new claims do not force visible pops. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capacity", meta = (ClampMin = "0"))
    int32 FadeReserveSlots = 4;

    /** Reserved Focus-lane capacity; the contextual design supports exactly one deliberate focused plate. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capacity|Lanes", meta = (ClampMin = "1", ClampMax = "1"))
    int32 FocusLaneSlots = 1;

    /** Reserved capacity for immediate survival information. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capacity|Lanes", meta = (ClampMin = "0"))
    int32 SafetyLaneSlots = 6;

    /** Reserved capacity for actionable dialogue, quest, service, and world-event opportunities. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capacity|Lanes", meta = (ClampMin = "0"))
    int32 OpportunityLaneSlots = 3;

    /** Reserved capacity for low-pressure identity and observable ambient information. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capacity|Lanes", meta = (ClampMin = "0"))
    int32 AwarenessLaneSlots = 2;
};

/** Screen-space collision policy that preserves semantic priority without turning the HUD into a label cloud. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicNameplateDeclutterPolicy {
    GENERATED_BODY()

    /** Empty logical pixels required between the conservative authored bounds of two visible surfaces. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Declutter",
              meta = (ClampMin = "0.0", ClampMax = "32.0"))
    float CollisionPaddingPixels = 6.0f;

    /** Extra separation a suppressed surface must regain before it can reappear, preventing edge flicker. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Declutter",
              meta = (ClampMin = "0.0", ClampMax = "48.0"))
    float ReleaseHysteresisPixels = 10.0f;

    /** Logical-pixel increment used to displace tactical surfaces vertically around a higher-priority surface. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Declutter",
              meta = (ClampMin = "4.0", ClampMax = "64.0"))
    float VerticalStepPixels = 24.0f;

    /** Bounded upward conflict-resolution passes for tactical surfaces before the lower-priority one hides. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Declutter",
              meta = (ClampMin = "0", ClampMax = "4"))
    int32 MaxVerticalSteps = 3;

    /** When enabled, identity-only Awareness surfaces yield instead of sliding around focused or tactical surfaces. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Declutter")
    bool bSuppressAwarenessOnCollision = true;
};

/** Hard readability caps for status badges on compact and deliberate-focus surfaces. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicNameplateStatusPolicy {
    GENERATED_BODY()

    /** Maximum status icons on a compact Context plate. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Statuses", meta = (ClampMin = "0", ClampMax = "4"))
    int32 ContextIconCap = 2;

    /** Maximum status icons on the single Focus surface; additional eligible statuses collapse into +N. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Statuses", meta = (ClampMin = "0", ClampMax = "4"))
    int32 FocusIconCap = 3;
};

/** Readability limits for viewer-specific actions on the separate focused action rail. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicNameplateActionPolicy {
    GENERATED_BODY()

    /** Maximum available glyph-and-verb entries on the one-line rail; the hard shipping cap is two. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actions", meta = (ClampMin = "1", ClampMax = "2"))
    int32 FocusActionCap = 2;
};

/** Local-only distance and transition policy for passive neutral identity surfaces. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicNameplatePassiveIdentityPolicy {
    GENERATED_BODY()

    /** Distance through which a passive Whisper remains at full presentation, in centimetres. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passive Identity|Whisper",
              meta = (ClampMin = "0.0", Units = "cm"))
    float WhisperFullDistanceCentimeters = 300.0f;

    /** Maximum distance at which a new passive Whisper may acquire, in centimetres. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passive Identity|Whisper",
              meta = (ClampMin = "0.0", Units = "cm"))
    float WhisperAcquireDistanceCentimeters = 400.0f;

    /** Distance at or beyond which an incumbent passive Whisper releases, in centimetres. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passive Identity|Whisper",
              meta = (ClampMin = "0.0", Units = "cm"))
    float WhisperReleaseDistanceCentimeters = 500.0f;

    /** Full steady-state opacity for an admitted passive Whisper, in the inclusive range 0..1. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passive Identity|Whisper",
              meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float WhisperFullAlpha = 0.90f;

    /** Local render scale reached at the Whisper release edge before the surface collapses. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passive Identity|Whisper",
              meta = (ClampMin = "0.75", ClampMax = "1.0"))
    float WhisperReleaseScale = 0.90f;

    /** Distance through which an ordinary neutral Focus remains at full presentation, in centimetres. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passive Identity|Focus",
              meta = (ClampMin = "0.0", Units = "cm"))
    float FocusFullDistanceCentimeters = 800.0f;

    /** Maximum distance at which a new ordinary neutral Focus may acquire, in centimetres. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passive Identity|Focus",
              meta = (ClampMin = "0.0", Units = "cm"))
    float FocusAcquireDistanceCentimeters = 1000.0f;

    /** Distance at or beyond which an incumbent ordinary neutral Focus releases, in centimetres. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passive Identity|Focus",
              meta = (ClampMin = "0.0", Units = "cm"))
    float FocusReleaseDistanceCentimeters = 1200.0f;

    /** Full steady-state opacity for an admitted ordinary neutral Focus, in the inclusive range 0..1. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passive Identity|Focus",
              meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float FocusFullAlpha = 1.0f;

    /** Local render scale reached at the ordinary Focus release edge before the surface collapses. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passive Identity|Focus",
              meta = (ClampMin = "0.75", ClampMax = "1.0"))
    float FocusReleaseScale = 0.85f;

    /** Local opacity/emphasis acquire transition duration; Reduced Motion snaps it while retaining admission gates. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passive Identity|Transitions",
              meta = (ClampMin = "0.0", ClampMax = "0.5", Units = "s"))
    float AcquireTransitionSeconds = 0.10f;

    /** Local whole-surface release duration, including authoritative death; Reduced Motion snaps it. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passive Identity|Transitions",
              meta = (ClampMin = "0.0", ClampMax = "0.5", Units = "s"))
    float ReleaseTransitionSeconds = 0.14f;
};

/** Density and screen-class policy for the deliberate, learned-knowledge Inspect surface. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicNameplateInspectPolicy {
    GENERATED_BODY()

    /** Deliberate CommonUI hold required before the learned dossier opens. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inspect",
              meta = (ClampMin = "0.20", ClampMax = "1.50", Units = "s"))
    float HoldDurationSeconds = 0.55f;

    /** Maximum resolved rows in any one learned-knowledge section before lower-priority facts are omitted. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inspect",
              meta = (ClampMin = "1", ClampMax = "16"))
    int32 MaxFactsPerSection = 8;

    /** Maximum resolved rows across the whole dossier, preserving a readable controller-first screen. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inspect",
              meta = (ClampMin = "1", ClampMax = "40"))
    int32 MaxTotalFacts = 24;

    /** CommonUI page pushed only after the configured inspect input's hold completes; null disables the page safely. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inspect")
    TSubclassOf<UMythicEntityInspectPage> InspectPageClass;
};

/** Primary Data Asset owning contextual-nameplate density, timing, visibility, and status budgets. */
UCLASS(BlueprintType)
class MYTHIC_API UMythicNameplatePolicy : public UPrimaryDataAsset {
    GENERATED_BODY()

public:
    /** Fixed pool and screen-lane budgets. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nameplate Policy")
    FMythicNameplateCapacityPolicy Capacity;

    /** Shared candidate, focus, visibility, distance, scoring, and hysteresis policy for all entity-attention consumers. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nameplate Policy")
    FMythicEntityAttentionConfig Attention;

    /** Deterministic screen-space collision, displacement, and reappearance hysteresis policy. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nameplate Policy")
    FMythicNameplateDeclutterPolicy Declutter;

    /** Context and Focus status-icon readability caps. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nameplate Policy")
    FMythicNameplateStatusPolicy Statuses;

    /** Separate one-line action-rail readability cap. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nameplate Policy")
    FMythicNameplateActionPolicy Actions;

    /** Local passive-neutral range gates, distance falloff, and whole-surface transition timings. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nameplate Policy")
    FMythicNameplatePassiveIdentityPolicy PassiveIdentity;

    /** Deliberate learned-dossier density and CommonUI surface. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nameplate Policy")
    FMythicNameplateInspectPolicy Inspect;

    /** Project visual tokens; this asset cannot change disclosure, timing, capacity, or gameplay truth. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nameplate Policy")
    TObjectPtr<UMythicNameplateVisualStyle> VisualStyle;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext &Context) const override;
#endif
};

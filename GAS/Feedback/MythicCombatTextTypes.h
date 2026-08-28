#pragma once

#include "CoreMinimal.h"
#include "MythicCombatTextTypes.generated.h"

class UMythicStatusEffectDefinition;

/**
 * Why a resolved combat-text event exists. This is presentation metadata only: it must never be used to drive
 * damage, status application, procs, threat, or kill credit.
 */
UENUM(BlueprintType)
enum class EMythicCombatTextOrigin : uint8 {
    /** Damage resolved from an immediate hit or ability execution. */
    DirectDamage,

    /** Damage resolved from a periodic status whose exact definition is carried by StatusDefinition. */
    StatusTick,

    /** Damage resolved without a player-owned source, such as a hazard or world rule. */
    EnvironmentalDamage,

    /** Reserved for damage authored as a status reaction rather than a normal hit or tick. */
    ReactionDamage,

    /** Incoming damage absorbed by an energy shield before health is changed. */
    ShieldAbsorption,

    /** Restored health. Included so healing can use the same transport when that path is unified. */
    Healing,
};

/**
 * Server-authored, client-presented result of one combat resolution. The magnitude is the health actually removed,
 * shield actually absorbed, or healing actually restored after gameplay resolution, never an authored estimate.
 * Actor and Data Asset references are intentional typed references; combat text does not reconstruct gameplay
 * identity from strings or duplicate IDs.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicResolvedCombatTextEvent {
    GENERATED_BODY()

    /** Actor credited as the source when one still exists. */
    UPROPERTY(BlueprintReadOnly, Category = "Combat Text")
    TObjectPtr<AActor> SourceActor = nullptr;

    /** Actor whose health was resolved. */
    UPROPERTY(BlueprintReadOnly, Category = "Combat Text")
    TObjectPtr<AActor> TargetActor = nullptr;

    /** Fallback world position used when the target is no longer available on the receiving client. */
    UPROPERTY(BlueprintReadOnly, Category = "Combat Text")
    FVector_NetQuantize10 WorldLocation = FVector::ZeroVector;

    /** Positive resolved amount whose meaning is defined by Origin. */
    UPROPERTY(BlueprintReadOnly, Category = "Combat Text")
    float Magnitude = 0.0f;

    /** Exact canonical status definition for StatusTick events; null for non-status damage. */
    UPROPERTY(BlueprintReadOnly, Category = "Combat Text")
    TObjectPtr<UMythicStatusEffectDefinition> StatusDefinition = nullptr;

    /** Presentation origin, kept orthogonal to critical state and status identity. */
    UPROPERTY(BlueprintReadOnly, Category = "Combat Text")
    EMythicCombatTextOrigin Origin = EMythicCombatTextOrigin::DirectDamage;

    /** Whether this direct damage resolution was critical. Status ticks do not inherit application-hit crits. */
    UPROPERTY(BlueprintReadOnly, Category = "Combat Text")
    bool bCritical = false;

    /** True when this event was routed to the player who dealt it; false for incoming presentation. */
    UPROPERTY(BlueprintReadOnly, Category = "Combat Text")
    bool bOutgoingForViewer = true;
};

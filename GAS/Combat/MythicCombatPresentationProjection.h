#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "GAS/Combat/MythicCombatThreatAssessment.h"

#include "MythicCombatPresentationProjection.generated.h"

class AActor;
class AMythicNPCCharacter;
class APawn;
class UAbilitySystemComponent;
enum class EMythicPresentedCombatRank : uint8;

/**
 * Server-side security and cadence policy for projecting one focused entity's combat read to its owning viewer.
 *
 * The client nominates only an opaque presentation instance. Authority independently proves range, aim, line of
 * sight, current generation, and combat facts before the owner-only transport receives a short-lived lease.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicCombatPresentationProjectionPolicy {
    GENERATED_BODY()

    /** Furthest pawn-to-subject distance at which a focused combat assessment may be disclosed, in centimeters. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat Presentation|Security",
              meta = (ClampMin = "100.0", ClampMax = "20000.0", Units = "cm"))
    float MaximumFocusRangeCentimeters = 6000.0f;

    /** Minimum server camera alignment required for the nominated subject; 0.92 is roughly a 23-degree cone. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat Presentation|Security",
              meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    float MinimumFocusViewDot = 0.92f;

    /** Collision channel used by the authority line-of-sight proof from the viewer camera to the subject anchor. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat Presentation|Security")
    TEnumAsByte<ECollisionChannel> LineOfSightTraceChannel = ECC_Visibility;

    /** Whether the authority focus line-of-sight proof should test complex collision geometry. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat Presentation|Security")
    bool bTraceComplex = false;

    /** Smallest accepted interval between client focus nominations; later nominations replace earlier deferred work. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat Presentation|Cadence",
              meta = (ClampMin = "0.02", ClampMax = "0.5", Units = "s"))
    float MinimumClientRequestIntervalSeconds = 0.08f;

    /** Authority refresh cadence while one exact subject remains focused; only that subject is sampled. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat Presentation|Cadence",
              meta = (ClampMin = "0.1", ClampMax = "2.0", Units = "s"))
    float AuthorityRefreshIntervalSeconds = 0.50f;

    /** Owner-only lease lifetime; must exceed the authority refresh interval so packet jitter cannot flicker the read. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat Presentation|Cadence",
              meta = (ClampMin = "0.2", ClampMax = "10.0", Units = "s"))
    float PresentationLeaseDurationSeconds = 1.50f;

    /** Whether a server-validated focused combatant may carry its exact combat level into the owner-only channel. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat Presentation|Disclosure")
    bool bPermitExactCombatLevelForFocus = true;

    /** Returns false for unsafe or internally contradictory authored values. */
    bool IsValid() const;
};

/**
 * Authority-only capability sample used to derive a dimensionless effective combat pressure.
 *
 * The sample is deliberately native and non-reflected: raw offensive and defensive values never enter replication,
 * Blueprint, saves, or the nameplate projection. Inputs describe sustained baseline capability rather than current
 * health, preventing an injured boss from being relabeled as harmless.
 */
struct MYTHIC_API FMythicCombatPressureSnapshot {
    bool bCombatCapable = false;
    float ExpectedDamagePerHit = 0.0f;
    float AttacksPerSecond = 0.0f;
    float CriticalHitChance = 0.0f;
    float CriticalDamageMultiplier = 1.0f;
    float OutgoingDamageMultiplier = 1.0f;
    float MaximumHealth = 0.0f;
    float MaximumShield = 0.0f;
    float ArmorMitigationFraction = 0.0f;
    float DodgeChance = 0.0f;
};

/** Pure rules shared by the authoritative producer and focused automation tests. */
struct MYTHIC_API FMythicCombatPresentationProjectionRules {
    /** Resolves an actor's authority GAS component without accepting any client-authored combat values. */
    static UAbilitySystemComponent *ResolveAbilitySystem(AActor *Actor);

    /**
     * Resolves an authority NPC's canonical AI tier into a player-facing rank. Unknown, invalid, and non-authority
     * NPCs fail closed; WorldBoss is intentionally never inferred from the ordinary AI tier ladder.
     */
    static EMythicPresentedCombatRank ResolveAuthorityNpcPresentedCombatRank(
        const AMythicNPCCharacter *NPC);

    /**
     * Returns whether the subject has made a publicly observable combat commitment: its combat state is active, or
     * an authority NPC is explicitly engaged with this viewer pawn. This does not inspect private AI intent.
     */
    static bool HasPublicCombatCommitment(
        const AActor *SubjectActor,
        const UAbilitySystemComponent *SubjectAbilitySystem,
        const APawn *ViewerPawn);

    /**
     * Samples the combat-owned baseline rating from live authority GAS and the exact actor's attack source. Weapon
     * cadence comes from the same AttackFragment montage cycle used at runtime; other attacks use the authored
     * UMythicCombatSettings fallback until their combat domain supplies a canonical cadence provider.
     */
    static FMythicCombatPressureSnapshot BuildAuthorityPressureSnapshot(
        const UObject *WorldContext, AActor *Actor,
        UAbilitySystemComponent *AbilitySystem);

    /** Samples the authority combat level for an exact actor; disclosure remains a separate caller permission. */
    static int32 ResolveAuthorityCombatLevel(
        AActor *Actor, const UAbilitySystemComponent *AbilitySystem);

    /** Computes sustained-offense times effective-survivability pressure, or zero for an incomplete sample. */
    static double ComputeEffectivePressure(const FMythicCombatPressureSnapshot &Snapshot);

    /** Applies the bounded server range, aim, and visibility gates to an already exact-resolved subject. */
    static bool IsSpatiallyEligible(float DistanceSquared, float ViewDot, bool bHasLineOfSight,
                                    const FMythicCombatPresentationProjectionPolicy &Policy);

    /** Returns the remaining request throttle delay, zero when work may execute, or the largest double for malformed input. */
    static double GetRequestThrottleDelaySeconds(double NowSeconds, double LastAcceptedSeconds,
                                                 float MinimumIntervalSeconds);

    /** Advances a producer revision while reserving zero as invalid for transport writes. */
    static uint32 AdvanceNonzeroRevision(uint32 CurrentRevision);
};

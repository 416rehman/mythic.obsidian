
#pragma once

#include "CoreMinimal.h"
#include "Engine/CurveTable.h"
#include "Engine/DeveloperSettings.h"
#include "World/LivingWorld/Territory/MythicDanger.h"
#include "GAS/Executions/MythicDamageCompose.h"
#include "GAS/MythicStatContribution.h"
#include "GAS/MythicHealthBands.h"
#include "GAS/MythicStatDiminishing.h"
#include "GAS/MythicWeatherCombatRules.h"
#include "GAS/Effects/MythicCrowdControl.h"
#include "GAS/Combat/MythicCombatThreatAssessment.h"
#include "MythicCombatSettings.generated.h"

class UGameplayEffect;

/**
 * The whole Resolve to MaxStamina derivation, as three numbers instead of three literals buried in an attribute
 * callback. MaxStamina = Base + BonusCeiling * Resolve / (Resolve + HalfPoint): a hyperbola that pays half the
 * ceiling at HalfPoint Resolve and approaches but never reaches Base + BonusCeiling, so stacking Resolve always
 * buys something and never buys an unbounded pool.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicResolveStaminaConfig {
    GENERATED_BODY()

    /** Stamina a character carries at zero Resolve, in stamina points. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stamina", meta = (ClampMin = "0.0"))
    float BaseMaxStamina = 100.0f;

    /** Stamina the Resolve curve asymptotically approaches on top of the base, in stamina points. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stamina", meta = (ClampMin = "0.0"))
    float ResolveBonusCeiling = 150.0f;

    /** Resolve at which half the bonus ceiling is paid; larger values push the whole curve later. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stamina", meta = (ClampMin = "0.01"))
    float ResolveHalfPoint = 40.0f;
};

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Mythic Combat"))
class MYTHIC_API UMythicCombatSettings : public UDeveloperSettings {
    GENERATED_BODY()

public:
    UMythicCombatSettings();

    virtual FName GetCategoryName() const override { return FName("Game"); }

    // Increased-vs-More damage bucket configuration. Empty buckets (default) = the compose layer is inert.
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Damage Compose")
    FMythicDamageComposeConfig DamageCompose;

    /**
     * Canonical subject-to-viewer combat-pressure boundaries used by every authority nameplate assessment. Keeping
     * these global combat balance values here prevents PlayerState classes and UI assets from drifting by viewer.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Combat Presentation",
              meta = (ShowOnlyInnerProperties))
    FMythicCombatThreatThresholds CombatPresentationThreatThresholds;

    /**
     * Authored basic attacks per second used only when a combatant has no exact live weapon montage cycle to rate.
     * It is a fail-closed baseline for NPC/native attacks, not a UI approximation or a client-provided value.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Combat Presentation",
              meta = (ClampMin = "0.01", ClampMax = "20.0", Units = "Hz"))
    float CombatRatingFallbackAttacksPerSecond = 1.0f;

    /**
     * Upper endpoint of the uniform basic-weapon damage roll, expressed as a multiplier of DamagePerHit. The shared
     * combat roll and item DPS projection both consume this value, so the displayed range cannot drift from hits.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Weapon Damage",
              meta = (ClampMin = "1.0"))
    float WeaponDamageMaximumMultiplier = 1.5f;

    /**
     * Which primary stat feeds which derived value, and by how much. Empty means primaries contribute nothing,
     * which is inert but honest - the damage path then uses the weapon roll alone rather than a hidden constant.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Primary Stats")
    FMythicStatContributionConfig StatContributions;

    // Weather×combat elemental coupling (J1). Empty Mods (default) = weather never touches damage (byte-identical
    // pipeline). Authored rows couple the live Environment.Weather.* state to damage: e.g. rain smothers fire hits
    // (×0.75) while conducting shock (×1.25 + bonus Slowed buildup) — see MythicWeatherCombatRules.h for the documented
    // sample set. Resolved ONCE per damage application (a cheap subsystem getter — no Tick, no polling).
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Weather Combat")
    FMythicWeatherCombatConfig WeatherCombat;

    /**
     * Slices of the health bar an entity advertises as GAS.State.Health.* tags while inside them. This is the whole
     * mechanism behind "hits harder when the target is nearly dead": the band tag is a normal gameplay tag, so a
     * gameplay effect gates a modifier on it with the tag requirements GAS already has, and no C++ knows which
     * talent is asking. Bands nest deliberately - an entity at 10% carries Critical, Low and Wounded at once, so a
     * two-tier talent is two modifiers rather than a special case.
     *
     * Empty (never, unless deliberately cleared) = no band tags are published and every effect gated on one is inert.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Health Bands")
    FMythicHealthBandConfig HealthBands;

    /**
     * Diminishing returns per stat. Gear stacks additively, so without a curve a deep enough stash makes any one
     * stat unbounded; with one, stacking always pays something and never pays everything.
     *
     * Unnamed stats are uncurved by default, so this is an opt-in list rather than a silent global cap.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Diminishing")
    FMythicStatDiminishingConfig StatDiminishing;

    /**
     * What a status does before anything on the applier touches it. A status definition that authors its own band
     * overrides the base; one that does not falls back to it, so a new status is playable before it is tuned.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Status Baseline", meta = (ClampMin = "0.0"))
    float StatusBaseDamagePerTick = 3.0f;

    /** Default status lifetime used when a status definition does not author its own duration band. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Status Baseline", meta = (ClampMin = "0.0"))
    float StatusBaseDurationSeconds = 5.0f;

    /**
     * Global scales applied to every status on top of its own band. These are the one place to answer "statuses
     * last too long" or "damage over time hits too hard" without opening eight assets.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Status Baseline", meta = (ClampMin = "0.0"))
    float StatusDamageScale = 1.0f;

    /** Global multiplier applied to the resolved lifetime of every status effect. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Status Baseline", meta = (ClampMin = "0.0"))
    float StatusDurationScale = 1.0f;

    /**
     * Buildup a single landed proc contributes, before the source's StatusBuildupMultiplier. Against the
     * threshold below this decides how many procs an unmodified attacker needs to land a status.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Status Baseline", meta = (ClampMin = "0.0"))
    float StatusBuildupPerProc;

    /**
     * Buildup a target must accumulate before a status lands. Resistance deliberately does not move this: it
     * already gates every proc through 1 - Resist, so at full resistance no buildup accrues at all, and bending
     * the threshold as well would pay the same stat twice.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Status Baseline", meta = (ClampMin = "1.0"))
    float StatusBuildupThreshold = 100.0f;

    /**
     * Most of the threshold an attacker's StatusThresholdReduction may remove. Without a ceiling a stacked build
     * reaches zero and every status lands on the first proc.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Status Baseline", meta = (ClampMin = "0.0", ClampMax = "0.95"))
    float MaxStatusThresholdReduction = 0.6f;

    /**
     * Ceiling on any one status resistance, as a [0,1] fraction. Every status proc is gated through 1 - Resist,
     * so at 1.0 a stacked resistance build is permanently and totally immune to that status with no counterplay.
     * Below 1.0 resistance stays a strong defence that a determined attacker can still beat.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Status Baseline", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MaxStatusResistance;

    /**
     * How hard each enemy tier resists repeat hard crowd control. One row per AI tier (1..5); a tier with no row
     * falls back to the gentlest defaults. This is the retune knob for CC feel, kept beside the stun-duration curve
     * in StatDiminishing so both halves of the mechanic live in one place.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Crowd Control")
    TArray<FMythicCcTierEscalation> CcEscalationByTier;

    /**
     * Base combat level a combat-capable entity spawns at per territory danger tier at its spawn site. The world already computes
     * danger from distance to civilisation and military strength; this is the one table that turns that danger into
     * a number the scaling curves can eat. A tier with no row spawns at level 1.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Combatant Level")
    TMap<EMythicDangerTier, int32> CombatantLevelByDangerTier;

    /**
     * Per-level growth applied beyond the last authored key of the combatant scaling curves. Keyed curves terminate
     * and levels do not; past the final key the sampled value keeps compounding at this rate, so the curves stay
     * a readable table while the progression stays open-ended.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Combatant Level", meta = (ClampMin = "1.0"))
    float CombatantHealthTailGrowth = 1.05f;

    /** Per-level damage multiplier used beyond the final authored combatant scaling key. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Combatant Level", meta = (ClampMin = "1.0"))
    float CombatantDamageTailGrowth = 1.04f;

    /**
     * The authored growth of each primary with character level - the whole "primaries rise with level"
     * model in two curves a designer can read. Consumed by MythicGE_PrimaryGrowth through non-snapshot
     * magnitudes, so a level-up moves the primary and its derived values the same frame.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Primary Stats")
    FCurveTableRowHandle PlayerPowerCurve;

    /** Authored player Strength progression sampled by the primary-stat growth effect. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Primary Stats")
    FCurveTableRowHandle PlayerStrengthCurve;

    /** Per-level multiplier used for player primary stats beyond their final authored curve keys. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Primary Stats", meta = (ClampMin = "1.0"))
    float PlayerPrimaryTailGrowth = 1.01f;

    /**
     * What sprinting is worth, multiplied onto the character's speed attribute while GAS.State.Sprinting is held.
     * Sprint used to be a gear-rolled fraction, which meant a character with no boots sprinted at walking pace;
     * it is a property of the act now, so every character gets the same differential and gear moves the whole
     * speed instead.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "1.0"))
    float SprintSpeedMultiplier = 1.5f;

    /**
     * The floor under the speed attribute and under the composed walk-speed scale. Stopping a character dead is
     * the crowd-control path's job - Stunned and Frozen disable movement outright - so no stack of slows,
     * encumbrance and debuffs is allowed to reach a standstill by accident, and none may invert movement.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.01", ClampMax = "1.0"))
    float MinSpeedScale = 0.1f;

    /**
     * How often the server samples a player pawn for the moving signal, in seconds. Movement runes read
     * GAS.State.Moving and GAS.Event.Moved instead of ticking, so this one timer is the whole cost of the feature.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.05", Units = "s"))
    float MovementSampleIntervalSeconds;

    /** Ground speed a pawn must exceed to count as moving, in cm/s. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0"))
    float MovingSpeedThresholdCmPerSec;

    /**
     * How long a pawn stays under the threshold before GAS.State.Moving drops and the distance odometer resets.
     * A pause shorter than this is still one journey, so a stutter-step never resets a distance rune.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0.0", Units = "s"))
    float StillGraceSeconds;

    /** How long a rune's callout stays above the pawn, in seconds. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Runes", meta = (ClampMin = "0.1", Units = "s"))
    float RuneCalloutLifetimeSeconds;

    /**
     * Multiple of MaxHealth a rune guard raises Shield by while it holds. Sized so no single hit can exceed it; the
     * number is never shown to a player.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Runes", meta = (ClampMin = "1.0"))
    float RuneGuardShieldMultiple;

    /** Most metres of fall past its threshold a slam rune is paid for, so a cliff dive has a ceiling. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Runes", meta = (ClampMin = "0.0"))
    float RuneSlamMaxBonusMetres;

    /** Least time between two plays of the same cue from one rune, in seconds, so a clink can never spam. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Runes", meta = (ClampMin = "0.0", Units = "s"))
    float RuneCueThrottleSeconds;

    /** How long a rune's success flash holds its badge lit, in seconds. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Runes", meta = (ClampMin = "0.0", Units = "s"))
    float RuneFlashSeconds;

    /** How long a rune's miss readout dims its badge, in seconds. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Runes", meta = (ClampMin = "0.0", Units = "s"))
    float RuneWhiffFlashSeconds;

    /** Fraction of MaxHealth a prevented fall must have been worth before Featherfall calls it out. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Runes", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float FeatherfallCalloutFraction;

    /**
     * Duration effect that adds SetByCaller.Generic to MaxShield for SetByCaller.Duration seconds. A rune guard applies
     * it before the Shield effect so the Shield add is not clamped against the old ceiling.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Runes")
    TSoftClassPtr<UGameplayEffect> RuneGuardMaxShieldEffect;

    /** Duration effect that adds SetByCaller.Generic to Shield for SetByCaller.Duration seconds. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Runes")
    TSoftClassPtr<UGameplayEffect> RuneGuardShieldEffect;

    /** Instant effect that adds SetByCaller.Generic to the Damage meta attribute, so a rune's self-wound runs the pipeline. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Runes")
    TSoftClassPtr<UGameplayEffect> RuneSelfDamageEffect;

    /** Instant effect that adds SetByCaller.Generic to the Healing meta attribute. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Runes")
    TSoftClassPtr<UGameplayEffect> RuneHealEffect;

    /**
     * Post-mitigation floor under any hit that was not negated outright, in damage points. It is what stops a
     * fully-armoured target reading as invulnerable to a weak attacker: the swing always says something.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Damage", meta = (ClampMin = "0.0"))
    float MinChipDamage;

    /** Pre-mitigation damage the Rage buff adds, as an additive fraction of the hit. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Damage", meta = (ClampMin = "0.0"))
    float RageDamageBonus;

    /** Pre-mitigation damage the Weakened debuff removes from its victim's hits, as an additive fraction. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Damage", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float WeakenedDamagePenalty;

    /** Extra pre-mitigation damage a Terrified target takes, as an additive fraction of the hit. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Damage", meta = (ClampMin = "0.0"))
    float TerrifiedDamageBonus;

    /** Pre-mitigation damage the Fortify buff removes from incoming hits, as an additive fraction. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Damage", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float FortifyDamageReduction;

    /**
     * Slowest an attack montage may play, as a unitless rate multiplier. Below this an attack stops reading as a
     * swing and starts reading as a hitch, however far a build sinks its attack speed.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Attack Speed", meta = (ClampMin = "0.01"))
    float MinAttackSpeedPlayRate;

    /**
     * Fastest an attack montage may play, as a unitless rate multiplier. Exactly zero means no ceiling, so attack
     * speed keeps paying however far a build stacks it; any positive value is the reachable ceiling item validation
     * projects attack-speed affix rolls against, so raising it widens what a weapon roll can be worth.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Attack Speed", meta = (ClampMin = "0.0"))
    float MaxAttackSpeedPlayRate;

    /**
     * Ceiling on dodge chance, however much an entity stacks. At 1.0 a build reaching 100% dodge is literally
     * invulnerable, so this must stay below 1 for stacked dodge to remain a trade rather than an exploit.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Probability", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MaxDodgeChance;

    /**
     * Where on-hit chances stop being worth their face value, as a [0,1] probability. Below this a chance is
     * exactly what it says; above it each further point buys less than the last, approaching certainty without
     * reaching it. Raise it to let gear carry more before the curve bites; lower it to make specialising bite sooner.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Probability", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ProbabilitySoftCap;

    /**
     * Most of a cooldown that stacked cooldown reduction may remove, as a [0,1] fraction. It is the safety ceiling
     * that keeps a deep reduction build off a degenerate zero-duration cooldown.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Cooldown", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MaxCooldownReduction;

    /**
     * Most of a stamina cost that stacked stamina-cost reduction may remove, as a [0,1] fraction. At 1.0 a
     * specialised build acts for free and stamina stops being a resource, so this is the knob that keeps it one.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Stamina", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MaxStaminaCostReduction;

    /** How Resolve buys MaxStamina; see the struct for the curve it describes. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Stamina")
    FMythicResolveStaminaConfig ResolveStamina;

    /** Extra proficiency XP the Enlighten buff grants, as an additive fraction of the award. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Progression", meta = (ClampMin = "0.0"))
    float EnlightenProficiencyBonus;
};

namespace MythicCombat {
/**
 * Sample a level-keyed curve with an open-ended geometric tail: inside the authored range the curve answers
 * directly; beyond its last key the final value compounds by TailGrowth per level. An unset handle reads 1.0,
 * so an unauthored curve scales nothing rather than zeroing what it multiplies.
 */
MYTHIC_API float SampleOpenEnded(const FCurveTableRowHandle &Handle, float Level, float TailGrowth);

/**
 * The combat level an entity standing here should carry: the authored level for the territory danger tier at
 * the position, lifted by the world tier's ItemLevelBase. Both actor spawn paths (NPC manager, Mass
 * embodiment) resolve through this one function.
 */
MYTHIC_API int32 ResolveCombatLevelAt(const UWorld *World, const FVector &Location);

/** The authored floor under any speed value. */
MYTHIC_API float GetMinSpeedScale();

/**
 * The MaxStamina a given Resolve is worth under the authored curve. Negative or non-finite Resolve reads as zero,
 * so a bad attribute value falls back to the base pool rather than producing a NaN one.
 */
MYTHIC_API float ResolveMaxStamina(float Resolve);

/**
 * The walk-speed scale a character should be moving at: its one speed attribute, the authored sprint multiplier
 * while sprinting, and the situational scales already composed into SituationalScale (slows, haste, encumbrance).
 * Never below the authored floor and never negative.
 */
MYTHIC_API float ComposeSpeedScale(float SpeedMultiplier, float SituationalScale, bool bSprinting);

/**
 * Resolves the global uniform basic-weapon damage band and its expected value from one composed DamagePerHit value.
 * Returns false and zeroes every output for invalid input instead of allowing combat and presentation to diverge.
 */
MYTHIC_API bool ResolveWeaponDamageRange(float DamagePerHit,
                                         float &OutMinimumDamage,
                                         float &OutMaximumDamage,
                                         float &OutAverageDamage);
}

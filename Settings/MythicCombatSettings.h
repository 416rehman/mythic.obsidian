
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
#include "MythicCombatSettings.generated.h"

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

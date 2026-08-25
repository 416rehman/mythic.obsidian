
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

UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Mythic Combat"))
class MYTHIC_API UMythicCombatSettings : public UDeveloperSettings {
    GENERATED_BODY()

public:
    UMythicCombatSettings();

    virtual FName GetCategoryName() const override { return FName("Game"); }

    // Increased-vs-More damage bucket configuration. Empty buckets (default) = the compose layer is inert.
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Damage Compose")
    FMythicDamageComposeConfig DamageCompose;

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

    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Status Baseline", meta = (ClampMin = "0.0"))
    float StatusBaseDurationSeconds = 5.0f;

    /**
     * Global scales applied to every status on top of its own band. These are the one place to answer "statuses
     * last too long" or "damage over time hits too hard" without opening eight assets.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Status Baseline", meta = (ClampMin = "0.0"))
    float StatusDamageScale = 1.0f;

    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Status Baseline", meta = (ClampMin = "0.0"))
    float StatusDurationScale = 1.0f;

    /**
     * How hard each enemy tier resists repeat hard crowd control. One row per AI tier (1..5); a tier with no row
     * falls back to the gentlest defaults. This is the retune knob for CC feel, kept beside the stun-duration curve
     * in StatDiminishing so both halves of the mechanic live in one place.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Crowd Control")
    TArray<FMythicCcTierEscalation> CcEscalationByTier;

    /**
     * Base combat level an enemy spawns at per territory danger tier at its spawn site. The world already computes
     * danger from distance to civilisation and military strength; this is the one table that turns that danger into
     * a number the scaling curves can eat. A tier with no row spawns at level 1.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Enemy Level")
    TMap<EMythicDangerTier, int32> EnemyLevelByDangerTier;

    /**
     * Per-level growth applied beyond the last authored key of the enemy scaling curves. Keyed curves terminate
     * and levels do not; past the final key the sampled value keeps compounding at this rate, so the curves stay
     * a readable table while the progression stays open-ended.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Enemy Level", meta = (ClampMin = "1.0"))
    float EnemyHealthTailGrowth = 1.05f;

    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Enemy Level", meta = (ClampMin = "1.0"))
    float EnemyDamageTailGrowth = 1.04f;
};

namespace MythicCombat {
/**
 * Sample a level-keyed curve with an open-ended geometric tail: inside the authored range the curve answers
 * directly; beyond its last key the final value compounds by TailGrowth per level. An unset handle reads 1.0,
 * so an unauthored curve scales nothing rather than zeroing what it multiplies.
 */
MYTHIC_API float SampleOpenEnded(const FCurveTableRowHandle &Handle, float Level, float TailGrowth);
}


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

/** The central band a core affix rolls from: a level-1 base scaled by the shared level curve. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicCoreAffixScaling {
    GENERATED_BODY()

    // The band at level 1. Units match the attribute (fractions for percentage affixes, flat for Armor).
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Core Affix")
    float BaseMin = 0.0f;

    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Core Affix")
    float BaseMax = 0.0f;
};

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

    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Combatant Level", meta = (ClampMin = "1.0"))
    float CombatantDamageTailGrowth = 1.04f;

    /**
     * The one place "how hard should a core affix of this level hit" is expressed. A core affix whose attribute
     * has a row here derives its min/max from BaseMin/BaseMax scaled by the level curve (with the same open-ended
     * tail as combatant scaling), instead of the numbers authored on the item. An item that authors a non-zero band
     * keeps it as its own level-1 identity - a chestplate outweighs boots - but the LEVEL dependence still comes
     * from here alone. An item that authors zeros needs no damage authoring at all.
     *
     * Rarity never touches these bands: it drives affix COUNT (AffixCountByRarity), never affix size.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Core Affix Scaling")
    TMap<FGameplayAttribute, FMythicCoreAffixScaling> CoreAffixScaling;

    // Level curve + compounding tail shared by every core affix row that does not author its own.
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Core Affix Scaling")
    FCurveTableRowHandle CoreAffixLevelCurve;

    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Core Affix Scaling", meta = (ClampMin = "1.0"))
    float CoreAffixTailGrowth = 1.05f;

    /**
     * The authored growth of each primary with character level - the whole "primaries rise with level"
     * model in two curves a designer can read. Consumed by MythicGE_PrimaryGrowth through non-snapshot
     * magnitudes, so a level-up moves the primary and its derived values the same frame.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Primary Stats")
    FCurveTableRowHandle PlayerPowerCurve;

    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Primary Stats")
    FCurveTableRowHandle PlayerStrengthCurve;

    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Primary Stats", meta = (ClampMin = "1.0"))
    float PlayerPrimaryTailGrowth = 1.01f;
};

namespace MythicCombat {
/**
 * Sample a level-keyed curve with an open-ended geometric tail: inside the authored range the curve answers
 * directly; beyond its last key the final value compounds by TailGrowth per level. An unset handle reads 1.0,
 * so an unauthored curve scales nothing rather than zeroing what it multiplies.
 */
MYTHIC_API float SampleOpenEnded(const FCurveTableRowHandle &Handle, float Level, float TailGrowth);

/**
 * The centrally scaled min/max a core affix of this attribute rolls at this item level, or false when the
 * attribute has no central row (the caller keeps its authored numbers). The out-band is the settings base -
 * or the item's own non-zero authored band, its deliberate identity - scaled by the shared level curve.
 */
/**
 * The combat level an entity standing here should carry: the authored level for the territory danger tier at
 * the position, lifted by the world tier's ItemLevelBase. Both actor spawn paths (NPC manager, Mass
 * embodiment) resolve through this one function.
 */
MYTHIC_API int32 ResolveCombatLevelAt(const UWorld *World, const FVector &Location);

MYTHIC_API bool ResolveCoreAffixBand(const FGameplayAttribute &Attribute, float AuthoredMin, float AuthoredMax,
                                     float ItemLevel, float &OutMin, float &OutMax);
}

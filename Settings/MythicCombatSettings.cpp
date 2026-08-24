
#include "Settings/MythicCombatSettings.h"

#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/MythicTags_GAS.h"

namespace {
FMythicStatDiminishing MakeCurve(const FGameplayAttribute &Attribute, float SoftCap, float Ceiling) {
    FMythicStatDiminishing Curve;
    Curve.Attribute = Attribute;
    Curve.SoftCapBonus = SoftCap;
    Curve.CeilingBonus = Ceiling;
    return Curve;
}

FMythicHealthBand MakeBand(const FGameplayTag &Tag, float Min, float Max) {
    FMythicHealthBand Band;
    Band.Tag = Tag;
    Band.MinFraction = Min;
    Band.MaxFraction = Max;
    return Band;
}

FMythicCcTierEscalation MakeCcTier(int32 Tier, float Step, int32 ImmunityAt, float Window, float Immune) {
    FMythicCcTierEscalation Row;
    Row.Tier = Tier;
    Row.Config.ThresholdEscalationStep = Step;
    Row.Config.ImmunityTriggerCount = ImmunityAt;
    Row.Config.RollingWindowSeconds = Window;
    Row.Config.ImmuneSeconds = Immune;
    return Row;
}
}

UMythicCombatSettings::UMythicCombatSettings() {
    HealthBands.Bands = {
        MakeBand(GAS_STATE_HEALTH_CRITICAL, 0.0f, 0.20f),
        MakeBand(GAS_STATE_HEALTH_LOW, 0.0f, 0.50f),
        MakeBand(GAS_STATE_HEALTH_WOUNDED, 0.0f, 0.90f),
        MakeBand(GAS_STATE_HEALTH_UNHURT, 0.90f, 1.0f),
    };

    using Off = UMythicAttributeSet_Offense;
    StatDiminishing.Stats = {
        // Damage over time is where a specialised build should be able to go furthest, so it gets the longest
        // runway: face value to +100%, then bending, approaching but never reaching +400%.
        MakeCurve(Off::GetBurnBonusDamageAttribute(), 1.0f, 4.0f),
        MakeCurve(Off::GetBleedBonusDamageAttribute(), 1.0f, 4.0f),
        MakeCurve(Off::GetPoisonBonusDamageAttribute(), 1.0f, 4.0f),

        // Duration compounds with damage - a longer burn is also a bigger burn - so it bends sooner and lower.
        MakeCurve(Off::GetBurnDurationMultiplierAttribute(), 0.5f, 1.5f),
        MakeCurve(Off::GetBleedDurationMultiplierAttribute(), 0.5f, 1.5f),
        MakeCurve(Off::GetPoisonDurationMultiplierAttribute(), 0.5f, 1.5f),
        MakeCurve(Off::GetSlowDurationMultiplierAttribute(), 0.5f, 1.5f),
        MakeCurve(Off::GetWeakenDurationMultiplierAttribute(), 0.5f, 1.5f),
        MakeCurve(Off::GetTerrifyDurationMultiplierAttribute(), 0.5f, 1.5f),

        // Freeze and stun take control away from a player entirely, and the escalation rules already fight
        // repeat application. Their duration gets the tightest curve in the game: a hard stun build is possible,
        // an unbreakable one is not.
        MakeCurve(Off::GetFreezeDurationMultiplierAttribute(), 0.2f, 0.5f),
        MakeCurve(Off::GetStunDurationMultiplierAttribute(), 0.2f, 0.5f),

        // Weapon-class bonus is the primary damage of a weapon-specialised build, and the narrowest, most
        // stackable damage stat in the game - one family, so every roll concentrates. It gets the specialist's
        // runway (face value to +100%, bending toward but never reaching +400%), never an unbounded stack.
        MakeCurve(Off::GetBonusSwordDamageAttribute(), 1.0f, 4.0f),
        MakeCurve(Off::GetBonusAxeDamageAttribute(), 1.0f, 4.0f),
        MakeCurve(Off::GetBonusDaggerDamageAttribute(), 1.0f, 4.0f),
        MakeCurve(Off::GetBonusSickleDamageAttribute(), 1.0f, 4.0f),
        MakeCurve(Off::GetBonusSpearDamageAttribute(), 1.0f, 4.0f),
        MakeCurve(Off::GetBonusHammerDamageAttribute(), 1.0f, 4.0f),

        // Skill damage and the two conditional damage bonuses ride the same specialist runway as weapon class.
        MakeCurve(Off::GetBonusSkillDamageAttribute(), 1.0f, 4.0f),
        MakeCurve(Off::GetControlPotencyAttribute(), 1.0f, 4.0f),
        MakeCurve(Off::GetIncreasedDamageToEnemiesUnderStatusEffectsAttribute(), 1.0f, 4.0f),
        MakeCurve(Off::GetBonusDamageToSuperiorEnemiesAttribute(), 1.0f, 4.0f),

        // Critical damage is its own build archetype and gets the longest runway of any stat - face value to
        // +300%, approaching but never reaching +1000% - so a crit build stays strong while stacking still bends.
        MakeCurve(Off::GetCriticalHitDamageAttribute(), 3.0f, 10.0f),
    };

    // Higher tiers shrug off repeat crowd control faster: a boss escalates hard and goes immune after two stuns,
    // a trash mob barely resists. One row per AI tier (1..5); an unlisted tier uses the struct's gentle defaults.
    CcEscalationByTier = {
        MakeCcTier(1, 0.25f, 8, 6.0f, 2.0f),
        MakeCcTier(2, 0.35f, 6, 8.0f, 3.5f),
        MakeCcTier(3, 0.5f, 4, 10.0f, 5.0f),
        MakeCcTier(4, 0.75f, 3, 10.0f, 6.0f),
        MakeCcTier(5, 1.0f, 2, 12.0f, 8.0f),
    };
}

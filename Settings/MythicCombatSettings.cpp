
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
        MakeCurve(Off::GetBurnDamageMultiplierAttribute(), 1.0f, 4.0f),
        MakeCurve(Off::GetBleedDamageMultiplierAttribute(), 1.0f, 4.0f),
        MakeCurve(Off::GetPoisonDamageMultiplierAttribute(), 1.0f, 4.0f),

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
    };
}

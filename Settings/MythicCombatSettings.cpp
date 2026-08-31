
#include "Settings/MythicCombatSettings.h"

#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/MythicTags_GAS.h"
#include "GameModes/Attributes/WorldAttributes.h"
#include "GameModes/GameState/MythicGameState.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"

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

    // Safe stays a tutorial-grade threat; each danger band opens a clear gap over the last so walking toward the
    // frontier reads as walking into a harder world.
    CombatantLevelByDangerTier = {
        {EMythicDangerTier::Safe, 1},
        {EMythicDangerTier::Low, 5},
        {EMythicDangerTier::Moderate, 12},
        {EMythicDangerTier::High, 20},
        {EMythicDangerTier::Extreme, 30},
    };

    MinChipDamage = 1.0f;
    RageDamageBonus = 0.25f;
    WeakenedDamagePenalty = 0.25f;
    TerrifiedDamageBonus = 0.25f;
    FortifyDamageReduction = 0.25f;

    MinAttackSpeedPlayRate = 0.8f;
    MaxAttackSpeedPlayRate = 1.4f;

    StatusBuildupPerProc = 25.0f;
    MaxStatusResistance = 1.0f;

    MaxDodgeChance = 0.75f;
    ProbabilitySoftCap = 0.5f;

    MaxCooldownReduction = 0.8f;

    MaxStaminaCostReduction = 1.0f;
    ResolveStamina.BaseMaxStamina = 100.0f;
    ResolveStamina.ResolveBonusCeiling = 150.0f;
    ResolveStamina.ResolveHalfPoint = 40.0f;

    EnlightenProficiencyBonus = 0.5f;
}

namespace MythicCombat {
int32 ResolveCombatLevelAt(const UWorld *World, const FVector &Location) {
    EMythicDangerTier Tier = EMythicDangerTier::Safe;
    if (World) {
        if (const UGameInstance *GI = World->GetGameInstance()) {
            if (const UMythicLivingWorldSubsystem *LWS = GI->GetSubsystem<UMythicLivingWorldSubsystem>()) {
                if (const UMythicTerritoryGrid *Grid = LWS->GetTerritoryGrid()) {
                    Tier = Grid->GetCellDangerTier(Grid->WorldToCell(Location));
                }
            }
        }
    }

    int32 Level = 1;
    if (const UMythicCombatSettings *Settings = GetDefault<UMythicCombatSettings>()) {
        if (const int32 *Base = Settings->CombatantLevelByDangerTier.Find(Tier)) {
            Level = FMath::Max(1, *Base);
        }
    }

    if (World) {
        if (const AMythicGameState *GS = World->GetGameState<AMythicGameState>()) {
            if (const UWorldTierAttributes *WTA = GS->WorldTierAttributes) {
                // ItemLevelBase is the world tier's floor for dropped gear; combatants stand on the same
                // floor so a higher world raises the fight and the reward together.
                Level += FMath::Max(0, FMath::RoundToInt(WTA->GetItemLevelBase()) - 1);
            }
        }
    }
    return Level;
}

float GetMinSpeedScale() {
    const UMythicCombatSettings *Settings = GetDefault<UMythicCombatSettings>();
    return Settings ? FMath::Clamp(Settings->MinSpeedScale, 0.01f, 1.0f) : 0.1f;
}

float ResolveMaxStamina(const float Resolve) {
    const FMythicResolveStaminaConfig &Config = GetDefault<UMythicCombatSettings>()->ResolveStamina;
    const float ClampedResolve = FMath::IsFinite(Resolve) ? FMath::Max(0.0f, Resolve) : 0.0f;
    const float HalfPoint = FMath::Max(KINDA_SMALL_NUMBER, Config.ResolveHalfPoint);
    const float Scalar = ClampedResolve / (ClampedResolve + HalfPoint);
    return Config.BaseMaxStamina + Config.ResolveBonusCeiling * Scalar;
}

float ComposeSpeedScale(const float SpeedMultiplier, const float SituationalScale, const bool bSprinting) {
    const UMythicCombatSettings *Settings = GetDefault<UMythicCombatSettings>();
    const float Sprint = bSprinting ? FMath::Max(1.0f, Settings ? Settings->SprintSpeedMultiplier : 1.5f) : 1.0f;
    return FMath::Max(GetMinSpeedScale(), SpeedMultiplier * SituationalScale * Sprint);
}

bool ResolveWeaponDamageRange(const float DamagePerHit,
                              float &OutMinimumDamage,
                              float &OutMaximumDamage,
                              float &OutAverageDamage) {
    OutMinimumDamage = 0.0f;
    OutMaximumDamage = 0.0f;
    OutAverageDamage = 0.0f;
    if (!FMath::IsFinite(DamagePerHit) || DamagePerHit < 0.0f) {
        return false;
    }

    const UMythicCombatSettings *Settings = GetDefault<UMythicCombatSettings>();
    const float AuthoredMaximumMultiplier = Settings
        ? Settings->WeaponDamageMaximumMultiplier
        : 1.5f;
    if (!FMath::IsFinite(AuthoredMaximumMultiplier)) {
        return false;
    }

    const float MaximumMultiplier = FMath::Max(1.0f, AuthoredMaximumMultiplier);
    const float MaximumDamage = DamagePerHit * MaximumMultiplier;
    const float AverageDamage = (DamagePerHit + MaximumDamage) * 0.5f;
    if (!FMath::IsFinite(MaximumDamage) || !FMath::IsFinite(AverageDamage)) {
        return false;
    }

    OutMinimumDamage = DamagePerHit;
    OutMaximumDamage = MaximumDamage;
    OutAverageDamage = AverageDamage;
    return true;
}

float SampleOpenEnded(const FCurveTableRowHandle &Handle, const float Level, const float TailGrowth) {
    if (Handle.IsNull()) {
        return 1.0f;
    }
    const FRealCurve *Curve = Handle.GetCurve(TEXT("MythicCombat::SampleOpenEnded"));
    if (!Curve || Curve->GetNumKeys() == 0) {
        return 1.0f;
    }

    float MinTime = 0.0f;
    float MaxTime = 0.0f;
    Curve->GetTimeRange(MinTime, MaxTime);
    const float Clamped = FMath::Clamp(Level, MinTime, MaxTime);
    const float Base = Curve->Eval(Clamped);
    if (Level <= MaxTime) {
        return Base;
    }
    return Base * FMath::Pow(FMath::Max(TailGrowth, 1.0f), Level - MaxTime);
}
}

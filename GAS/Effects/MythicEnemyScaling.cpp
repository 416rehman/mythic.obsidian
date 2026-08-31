
#include "MythicEnemyScaling.h"

#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "AI/MythicTags_AI.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "Settings/MythicDeveloperSettings.h"

UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_SETBYCALLER_SCALING_HEALTH, "SetByCaller.Scaling.Health",
                               "Runtime health multiplier fed to the CombatScaling GE");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_SETBYCALLER_SCALING_DAMAGE, "SetByCaller.Scaling.Damage",
                               "Runtime damage multiplier fed to the CombatScaling GE");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_STATE_COMBATSCALING, "GAS.State.CombatScaling",
                               "Granted by the infinite CombatScaling GE so pool-return can strip it (else it accumulates)");

FVector2D FMythicEnemyScaling::ComputeStatMultiplier(int32 PartySize, float PerExtraMemberHealth, float PerExtraMemberDamage,
                                                     float WorldHealthMultiplier, float WorldDamageMultiplier) {
    const int32 ExtraMembers = FMath::Max(PartySize, 1) - 1;

    const float PEMH = FMath::Max(PerExtraMemberHealth, 0.0f);
    const float PEMD = FMath::Max(PerExtraMemberDamage, 0.0f);

    // A world multiplier that has not replicated yet reads as zero. Treat anything non-positive as "no tier bonus"
    // so a late attribute leaves the enemy at its authored strength instead of erasing it.
    const float WorldH = WorldHealthMultiplier > 0.0f ? WorldHealthMultiplier : 1.0f;
    const float WorldD = WorldDamageMultiplier > 0.0f ? WorldDamageMultiplier : 1.0f;

    return FVector2D((1.0f + ExtraMembers * PEMH) * WorldH, (1.0f + ExtraMembers * PEMD) * WorldD);
}

FMythicTierScaling FMythicEnemyScaling::GetTierScaling(const FGameplayTag &TierTag) {
    if (const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>()) {
        const int32 Wanted = GetAITierInt(TierTag);
        for (const FMythicEnemyTierScaling &Row : Settings->EnemyTierScaling) {
            if (GetAITierInt(Row.Tier) == Wanted) {
                return FMythicTierScaling{Row.HealthMultiplier, Row.DamageMultiplier, Row.XpMultiplier,
                                          Row.ItemLevelBonus};
            }
        }
    }
    // An unconfigured tier leaves the enemy at its authored strength rather than erasing it.
    return FMythicTierScaling{1.0f, 1.0f, 1.0f, 0};
}

int32 FMythicEnemyScaling::ComputeDropItemLevel(float WorldItemLevelBase, const FGameplayTag &TierTag) {
    const int32 Base = FMath::RoundToInt(FMath::Max(WorldItemLevelBase, 0.0f));
    return FMath::Max(1, Base + GetTierScaling(TierTag).ItemLevelBonus);
}

UMythicGE_CombatScaling::UMythicGE_CombatScaling() {
    DurationPolicy = EGameplayEffectDurationType::Infinite;

    {
        FGameplayModifierInfo HealthMod;
        HealthMod.Attribute = UMythicAttributeSet_Life::GetMaxHealthAttribute();
        // Encounter/world/tier scaling is independent from primary-stat derivation, so it compounds with the
        // derived factor instead of joining GAS's additive multiplier bucket.
        HealthMod.ModifierOp = EGameplayModOp::MultiplyCompound;
        FSetByCallerFloat HealthSBC;
        HealthSBC.DataTag = GAS_SETBYCALLER_SCALING_HEALTH;
        HealthMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(HealthSBC);
        Modifiers.Add(HealthMod);
    }

    {
        FGameplayModifierInfo DamageMod;
        DamageMod.Attribute = UMythicAttributeSet_Offense::GetDamagePerHitAttribute();
        DamageMod.ModifierOp = EGameplayModOp::MultiplyCompound;
        FSetByCallerFloat DamageSBC;
        DamageSBC.DataTag = GAS_SETBYCALLER_SCALING_DAMAGE;
        DamageMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(DamageSBC);
        Modifiers.Add(DamageMod);
    }
}

void UMythicGE_CombatScaling::PostInitProperties() {
    Super::PostInitProperties();

    UTargetTagsGameplayEffectComponent &TagComp = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
    FInheritedTagContainer Container;
    Container.AddTag(GAS_STATE_COMBATSCALING);
    TagComp.SetAndApplyTargetTagChanges(Container);
}

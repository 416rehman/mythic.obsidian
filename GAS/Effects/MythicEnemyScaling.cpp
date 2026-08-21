
#include "MythicEnemyScaling.h"

#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "AI/MythicTags_AI.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"

UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_SETBYCALLER_SCALING_HEALTH, "SetByCaller.Scaling.Health",
                               "Runtime health multiplier fed to the CombatScaling GE");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_SETBYCALLER_SCALING_DAMAGE, "SetByCaller.Scaling.Damage",
                               "Runtime damage multiplier fed to the CombatScaling GE");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_STATE_COMBATSCALING, "GAS.State.CombatScaling",
                               "Granted by the infinite CombatScaling GE so pool-return can strip it (else it accumulates)");

FVector2D FMythicEnemyScaling::ComputeStatMultiplier(int32 PartySize, int32 WorldTier,
                                                     float PerExtraMemberHealth, float PerExtraMemberDamage,
                                                     float PerTierHealth, float PerTierDamage) {
    const int32 ClampedParty = FMath::Max(PartySize, 1);
    const int32 ClampedTier = FMath::Max(WorldTier, 0);
    const int32 ExtraMembers = ClampedParty - 1;

    const float PEMH = FMath::Max(PerExtraMemberHealth, 0.0f);
    const float PEMD = FMath::Max(PerExtraMemberDamage, 0.0f);
    const float PTH = FMath::Max(PerTierHealth, 0.0f);
    const float PTD = FMath::Max(PerTierDamage, 0.0f);

    const float HealthMult = 1.0f + ExtraMembers * PEMH + ClampedTier * PTH;
    const float DamageMult = 1.0f + ExtraMembers * PEMD + ClampedTier * PTD;
    return FVector2D(HealthMult, DamageMult);
}

FMythicTierScaling FMythicEnemyScaling::GetTierScaling(const FGameplayTag &TierTag) {
    switch (GetAITierInt(TierTag)) {
        case 2: return FMythicTierScaling{1.5f, 1.3f, 2.0f};
        case 3: return FMythicTierScaling{2.5f, 1.8f, 4.0f};
        case 4: return FMythicTierScaling{5.0f, 2.5f, 8.0f};
        case 5: return FMythicTierScaling{12.0f, 4.0f, 20.0f};
        case 1:
        default:
            return FMythicTierScaling{1.0f, 1.0f, 1.0f};
    }
}

UMythicGE_CombatScaling::UMythicGE_CombatScaling() {
    DurationPolicy = EGameplayEffectDurationType::Infinite;

    {
        FGameplayModifierInfo HealthMod;
        HealthMod.Attribute = UMythicAttributeSet_Life::GetMaxHealthAttribute();
        HealthMod.ModifierOp = EGameplayModOp::MultiplyAdditive;
        FSetByCallerFloat HealthSBC;
        HealthSBC.DataTag = GAS_SETBYCALLER_SCALING_HEALTH;
        HealthMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(HealthSBC);
        Modifiers.Add(HealthMod);
    }

    {
        FGameplayModifierInfo DamageMod;
        DamageMod.Attribute = UMythicAttributeSet_Offense::GetDamagePerHitAttribute();
        DamageMod.ModifierOp = EGameplayModOp::MultiplyAdditive;
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

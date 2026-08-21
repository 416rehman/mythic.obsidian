
#include "Misc/AutomationTest.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "GAS/Effects/MythicEnemyScaling.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "AI/MythicTags_AI.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicEnemyScalingTest,
    "Mythic.Combat.EnemyScaling",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicEnemyScalingTest::RunTest(const FString &Parameters) {
    using ES = FMythicEnemyScaling;

    const float PEMH = 0.15f, PEMD = 0.10f, PTH = 0.25f, PTD = 0.15f;

    {
        const FVector2D M = ES::ComputeStatMultiplier(1, 0, PEMH, PEMD, PTH, PTD);
        TestEqual(TEXT("baseline health mult == 1"), static_cast<float>(M.X), 1.0f);
        TestEqual(TEXT("baseline damage mult == 1"), static_cast<float>(M.Y), 1.0f);
    }

    {
        const FVector2D MParty = ES::ComputeStatMultiplier(0, 0, PEMH, PEMD, PTH, PTD);
        TestEqual(TEXT("PartySize 0 clamps to solo (health 1)"), static_cast<float>(MParty.X), 1.0f);
        TestEqual(TEXT("PartySize 0 clamps to solo (damage 1)"), static_cast<float>(MParty.Y), 1.0f);

        const FVector2D MTier = ES::ComputeStatMultiplier(1, -5, PEMH, PEMD, PTH, PTD);
        TestEqual(TEXT("WorldTier -5 clamps to 0 (health 1)"), static_cast<float>(MTier.X), 1.0f);
        TestEqual(TEXT("WorldTier -5 clamps to 0 (damage 1)"), static_cast<float>(MTier.Y), 1.0f);
    }

    {
        const FVector2D M = ES::ComputeStatMultiplier(3, 0, PEMH, PEMD, PTH, PTD);
        TestEqual(TEXT("party 3 health = 1 + 2*0.15"), static_cast<float>(M.X), 1.30f);
        TestEqual(TEXT("party 3 damage = 1 + 2*0.10"), static_cast<float>(M.Y), 1.20f);
        TestTrue(TEXT("party scales health above baseline"), M.X > 1.0f);
        TestTrue(TEXT("party scales damage above baseline"), M.Y > 1.0f);
    }

    {
        const FVector2D M = ES::ComputeStatMultiplier(1, 2, PEMH, PEMD, PTH, PTD);
        TestEqual(TEXT("world tier 2 health = 1 + 2*0.25"), static_cast<float>(M.X), 1.50f);
        TestEqual(TEXT("world tier 2 damage = 1 + 2*0.15"), static_cast<float>(M.Y), 1.30f);
    }

    {
        const FVector2D M = ES::ComputeStatMultiplier(2, 1, PEMH, PEMD, PTH, PTD);
        TestEqual(TEXT("party2+tier1 health = 1 + 0.15 + 0.25"), static_cast<float>(M.X), 1.40f);
        TestEqual(TEXT("party2+tier1 damage = 1 + 0.10 + 0.15"), static_cast<float>(M.Y), 1.25f);
    }

    {
        const FVector2D M = ES::ComputeStatMultiplier(5, 3, -1.0f, -1.0f, -1.0f, -1.0f);
        TestTrue(TEXT("negative tunables: health >= 1"), M.X >= 1.0f);
        TestTrue(TEXT("negative tunables: damage >= 1"), M.Y >= 1.0f);
    }

    {
        const FMythicTierScaling N = ES::GetTierScaling(AI_TIER_NORMAL);
        TestEqual(TEXT("Normal health == 1"), N.HealthMult, 1.0f);
        TestEqual(TEXT("Normal damage == 1"), N.DamageMult, 1.0f);
        TestEqual(TEXT("Normal xp == 1"), N.XpMult, 1.0f);
    }

    {
        const FMythicTierScaling Empty = ES::GetTierScaling(FGameplayTag());
        TestEqual(TEXT("empty tag health fallback == 1"), Empty.HealthMult, 1.0f);
        TestEqual(TEXT("empty tag damage fallback == 1"), Empty.DamageMult, 1.0f);
        TestEqual(TEXT("empty tag xp fallback == 1"), Empty.XpMult, 1.0f);

        const FMythicTierScaling NonTier = ES::GetTierScaling(AI_AFFILIATION_HUMAN);
        TestEqual(TEXT("non-tier tag health fallback == 1"), NonTier.HealthMult, 1.0f);
        TestEqual(TEXT("non-tier tag damage fallback == 1"), NonTier.DamageMult, 1.0f);
        TestEqual(TEXT("non-tier tag xp fallback == 1"), NonTier.XpMult, 1.0f);
    }

    {
        const FMythicTierScaling N = ES::GetTierScaling(AI_TIER_NORMAL);
        const FMythicTierScaling S = ES::GetTierScaling(AI_TIER_SUPERIOR);
        const FMythicTierScaling E = ES::GetTierScaling(AI_TIER_ELITE);
        const FMythicTierScaling C = ES::GetTierScaling(AI_TIER_CHAMPION);
        const FMythicTierScaling B = ES::GetTierScaling(AI_TIER_BOSS);

        TestTrue(TEXT("health monotonic N<S<E<C<B"),
                 N.HealthMult < S.HealthMult && S.HealthMult < E.HealthMult &&
                 E.HealthMult < C.HealthMult && C.HealthMult < B.HealthMult);
        TestTrue(TEXT("damage monotonic N<S<E<C<B"),
                 N.DamageMult < S.DamageMult && S.DamageMult < E.DamageMult &&
                 E.DamageMult < C.DamageMult && C.DamageMult < B.DamageMult);
        TestTrue(TEXT("xp monotonic N<S<E<C<B"),
                 N.XpMult < S.XpMult && S.XpMult < E.XpMult &&
                 E.XpMult < C.XpMult && C.XpMult < B.XpMult);
        TestTrue(TEXT("boss mults strictly positive"),
                 B.HealthMult > 0.0f && B.DamageMult > 0.0f && B.XpMult > 0.0f);
    }

    {
        const FVector2D PartyWorld = ES::ComputeStatMultiplier(3, 1, PEMH, PEMD, PTH, PTD);
        const FMythicTierScaling Elite = ES::GetTierScaling(AI_TIER_ELITE);
        const float CombinedHealth = static_cast<float>(PartyWorld.X) * Elite.HealthMult;
        const float CombinedDamage = static_cast<float>(PartyWorld.Y) * Elite.DamageMult;
        TestEqual(TEXT("combined health == 1.55 * 2.5"), CombinedHealth, 3.875f);
        TestEqual(TEXT("combined damage == 1.35 * 1.8"), CombinedDamage, 2.43f);
        TestTrue(TEXT("combined health exceeds each factor"),
                 CombinedHealth > static_cast<float>(PartyWorld.X) && CombinedHealth > Elite.HealthMult);
    }

    {
        const UMythicGE_CombatScaling *CDO = GetDefault<UMythicGE_CombatScaling>();
        TestNotNull(TEXT("CombatScaling CDO resolves"), CDO);
        if (CDO) {
            TestTrue(TEXT("DurationPolicy == Infinite"),
                     CDO->DurationPolicy == EGameplayEffectDurationType::Infinite);
            TestEqual(TEXT("exactly 2 modifiers"), CDO->Modifiers.Num(), 2);
            if (CDO->Modifiers.Num() == 2) {
                for (const FGameplayModifierInfo &Mod : CDO->Modifiers) {
                    TestTrue(TEXT("modifier op is Multiply(Additive)"),
                             Mod.ModifierOp == EGameplayModOp::MultiplyAdditive);
                    TestTrue(TEXT("modifier magnitude is SetByCaller"),
                             Mod.ModifierMagnitude.GetMagnitudeCalculationType() ==
                                 EGameplayEffectMagnitudeCalculation::SetByCaller);
                }
                const FGameplayAttribute HealthAttr = UMythicAttributeSet_Life::GetMaxHealthAttribute();
                const FGameplayAttribute DamageAttr = UMythicAttributeSet_Offense::GetDamagePerHitAttribute();
                bool bHasHealth = false, bHasDamage = false;
                for (const FGameplayModifierInfo &Mod : CDO->Modifiers) {
                    if (Mod.Attribute == HealthAttr &&
                        Mod.ModifierMagnitude.GetSetByCallerFloat().DataTag == GAS_SETBYCALLER_SCALING_HEALTH) {
                        bHasHealth = true;
                    }
                    if (Mod.Attribute == DamageAttr &&
                        Mod.ModifierMagnitude.GetSetByCallerFloat().DataTag == GAS_SETBYCALLER_SCALING_DAMAGE) {
                        bHasDamage = true;
                    }
                }
                TestTrue(TEXT("MaxHealth modifier keyed by health SetByCaller tag"), bHasHealth);
                TestTrue(TEXT("DamagePerHit modifier keyed by damage SetByCaller tag"), bHasDamage);
            }
            const UTargetTagsGameplayEffectComponent *TagComp =
                CDO->FindComponent<UTargetTagsGameplayEffectComponent>();
            TestNotNull(TEXT("has a TargetTags component"), TagComp);
            if (TagComp) {
                TestTrue(TEXT("grants GAS.State.CombatScaling cleanup tag"),
                         TagComp->GetConfiguredTargetTagChanges().Added.HasTagExact(GAS_STATE_COMBATSCALING));
            }
        }
    }

    return true;
}

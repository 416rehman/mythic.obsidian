
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

    const float PEMH = 0.15f, PEMD = 0.10f;

    {
        const FVector2D M = ES::ComputeStatMultiplier(1, PEMH, PEMD, 1.0f, 1.0f);
        TestEqual(TEXT("baseline health mult == 1"), static_cast<float>(M.X), 1.0f);
        TestEqual(TEXT("baseline damage mult == 1"), static_cast<float>(M.Y), 1.0f);
    }

    {
        const FVector2D M = ES::ComputeStatMultiplier(0, PEMH, PEMD, 1.0f, 1.0f);
        TestEqual(TEXT("PartySize 0 clamps to solo (health 1)"), static_cast<float>(M.X), 1.0f);
        TestEqual(TEXT("PartySize 0 clamps to solo (damage 1)"), static_cast<float>(M.Y), 1.0f);
    }

    {
        const FVector2D M = ES::ComputeStatMultiplier(3, PEMH, PEMD, 1.0f, 1.0f);
        TestEqual(TEXT("party 3 health = 1 + 2*0.15"), static_cast<float>(M.X), 1.30f);
        TestEqual(TEXT("party 3 damage = 1 + 2*0.10"), static_cast<float>(M.Y), 1.20f);
    }

    // The whole point of #101: the tier ladder is the authored curve, so changing the attribute must change the
    // scaled result. Curve_WorldTiers rows EnemyHealthMultiplier/EnemyDamageMultiplier read 1, 1.5, 2, 3.
    {
        const FVector2D M = ES::ComputeStatMultiplier(1, PEMH, PEMD, 3.0f, 3.0f);
        TestEqual(TEXT("world tier 4 health takes the curve value"), static_cast<float>(M.X), 3.0f);
        TestEqual(TEXT("world tier 4 damage takes the curve value"), static_cast<float>(M.Y), 3.0f);
    }

    {
        // Party and world tier are separate axes and compose multiplicatively.
        const FVector2D M = ES::ComputeStatMultiplier(3, PEMH, PEMD, 1.5f, 1.5f);
        TestEqual(TEXT("party 3 at tier 2 health = 1.30 * 1.5"), static_cast<float>(M.X), 1.95f);
        TestEqual(TEXT("party 3 at tier 2 damage = 1.20 * 1.5"), static_cast<float>(M.Y), 1.80f);
    }

    {
        // The attributes replicate, so an NPC can spawn before they arrive and read zero. That must leave the
        // enemy at its authored strength, never erase it.
        const FVector2D M = ES::ComputeStatMultiplier(3, PEMH, PEMD, 0.0f, 0.0f);
        TestEqual(TEXT("unreplicated world health falls back to no tier bonus"), static_cast<float>(M.X), 1.30f);
        TestEqual(TEXT("unreplicated world damage falls back to no tier bonus"), static_cast<float>(M.Y), 1.20f);
    }

    {
        const FVector2D M = ES::ComputeStatMultiplier(5, -1.0f, -1.0f, -1.0f, -1.0f);
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
        // Party, world tier and NPC rank are three independent axes; ApplyCombatScaling multiplies all three.
        const FVector2D PartyWorld = ES::ComputeStatMultiplier(3, PEMH, PEMD, 1.5f, 1.5f);
        const FMythicTierScaling Elite = ES::GetTierScaling(AI_TIER_ELITE);
        const float CombinedHealth = static_cast<float>(PartyWorld.X) * Elite.HealthMult;
        const float CombinedDamage = static_cast<float>(PartyWorld.Y) * Elite.DamageMult;
        TestEqual(TEXT("combined health == 1.30 * 1.5 * 2.5"), CombinedHealth, 4.875f);
        TestEqual(TEXT("combined damage == 1.20 * 1.5 * 1.8"), CombinedDamage, 3.24f);
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
                    TestTrue(TEXT("independent scaling uses Multiply(Compound)"),
                             Mod.ModifierOp == EGameplayModOp::MultiplyCompound);
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

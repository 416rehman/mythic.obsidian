#include "Misc/AutomationTest.h"

#include "AI/MythicTags_AI.h"
#include "AI/NPCs/MythicNPCCharacter.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/Effects/MythicEnemyScaling.h"
#include "GameplayEffect.h"

namespace {
const TCHAR *TierPawns[] = {
    TEXT("/Game/Mythic/AI/NPCs/Tiers/Pawn_Humanoid_Superior.Pawn_Humanoid_Superior_C"),
    TEXT("/Game/Mythic/AI/NPCs/Tiers/Pawn_Humanoid_Elite.Pawn_Humanoid_Elite_C"),
    TEXT("/Game/Mythic/AI/NPCs/Tiers/Pawn_Humanoid_Champion.Pawn_Humanoid_Champion_C"),
    TEXT("/Game/Mythic/AI/NPCs/Tiers/Pawn_Humanoid_Boss.Pawn_Humanoid_Boss_C"),
};

// Named rather than indexed into TierPawns: an index silently points at a different pawn the moment
// someone adds one, and the test would still pass while asserting nothing about a boss.
const TCHAR *BossPawn = TEXT("/Game/Mythic/AI/NPCs/Tiers/Pawn_Humanoid_Boss.Pawn_Humanoid_Boss_C");

const AMythicNPCCharacter *LoadPawn(const TCHAR *Path) {
    const UClass *Loaded = LoadClass<AActor>(nullptr, Path);
    return Loaded ? Cast<AMythicNPCCharacter>(Loaded->GetDefaultObject()) : nullptr;
}

/** Highest Override magnitude an effect applies to one attribute, or -1 when it never touches it. */
float FindOverride(const UGameplayEffect *Effect, const FGameplayAttribute &Attribute) {
    if (!Effect) {
        return -1.0f;
    }
    for (const FGameplayModifierInfo &Mod : Effect->Modifiers) {
        if (Mod.Attribute == Attribute) {
            float Magnitude = 0.0f;
            Mod.ModifierMagnitude.GetStaticMagnitudeIfPossible(1.0f, Magnitude);
            return Magnitude;
        }
    }
    return -1.0f;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicNPCsCanFightTest,
    "Mythic.Combat.NPCsCanFight",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicNPCsCanFightTest::RunTest(const FString &Parameters) {
    // THE REGRESSION THIS EXISTS FOR: no NPC in the project could deal damage. Every tier pawn had a null
    // AttackAbility, and the only default effect any of them carried set MaxHealth and Health and nothing
    // else - so DamagePerHit, Power and Armor all sat at the C++ default of 0. The tier multiplier is
    // MultiplyAdditive, and 0 x 4 for a Boss is still 0. Every defensive stat in the game was therefore
    // unmeasurable in play, and it looked like broken AI rather than empty data.
    for (const TCHAR *Path : TierPawns) {
        const AMythicNPCCharacter *Pawn = LoadPawn(Path);
        const FString Name = FPaths::GetBaseFilename(FString(Path));
        if (!TestNotNull(*FString::Printf(TEXT("%s loads"), *Name), Pawn)) {
            continue;
        }

        TestNotNull(*FString::Printf(TEXT("%s carries an attack ability"), *Name),
                    Pawn->GetAttackAbility().Get());
        TestTrue(*FString::Printf(TEXT("%s carries a stat baseline"), *Name),
                 Pawn->GetDefaultGameplayEffects().Num() > 0);
        TestTrue(*FString::Printf(TEXT("%s declares a tier"), *Name),
                 Pawn->GetEnemyTier().IsValid());

        // A baseline that never sets DamagePerHit leaves it at zero, which is the whole bug: the pawn
        // has an ability, swings it, and applies nothing.
        bool bGrantsDamage = false;
        bool bGrantsArmor = false;
        for (const TSubclassOf<UGameplayEffect> &EffectClass : Pawn->GetDefaultGameplayEffects()) {
            const UGameplayEffect *Effect = EffectClass ? EffectClass->GetDefaultObject<UGameplayEffect>() : nullptr;
            if (FindOverride(Effect, UMythicAttributeSet_Offense::GetDamagePerHitAttribute()) > 0.0f) {
                bGrantsDamage = true;
            }
            if (FindOverride(Effect, UMythicAttributeSet_Defense::GetArmorAttribute()) > 0.0f) {
                bGrantsArmor = true;
            }
        }
        TestTrue(*FString::Printf(TEXT("%s is given non-zero DamagePerHit"), *Name), bGrantsDamage);
        // Armor on the attacker matters too: it is what makes the mitigation curve observable from a fight.
        TestTrue(*FString::Printf(TEXT("%s is given non-zero Armor"), *Name), bGrantsArmor);
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicNPCTierDamageTest,
    "Mythic.Combat.NPCTierDamage",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicNPCTierDamageTest::RunTest(const FString &Parameters) {
    // A shared baseline plus a per-tier multiplier is the only reason a Boss hits harder than a Superior.
    // Assert the product, not the multiplier alone: a baseline of zero multiplies to zero at every tier,
    // which is exactly how this shipped.
    const AMythicNPCCharacter *Boss = LoadPawn(BossPawn);
    if (!TestNotNull(TEXT("the boss pawn loads"), Boss)) {
        return false;
    }

    float Baseline = 0.0f;
    for (const TSubclassOf<UGameplayEffect> &EffectClass : Boss->GetDefaultGameplayEffects()) {
        const UGameplayEffect *Effect = EffectClass ? EffectClass->GetDefaultObject<UGameplayEffect>() : nullptr;
        Baseline = FMath::Max(Baseline, FindOverride(Effect, UMythicAttributeSet_Offense::GetDamagePerHitAttribute()));
    }
    TestTrue(TEXT("the boss has a non-zero damage baseline to scale"), Baseline > 0.0f);

    const float SuperiorHit = Baseline * FMythicEnemyScaling::GetTierScaling(AI_TIER_SUPERIOR).DamageMult;
    const float BossHit = Baseline * FMythicEnemyScaling::GetTierScaling(AI_TIER_BOSS).DamageMult;

    TestTrue(TEXT("a superior actually hits for something"), SuperiorHit > 0.0f);
    TestTrue(TEXT("a boss hits harder than a superior"), BossHit > SuperiorHit);

    return true;
}

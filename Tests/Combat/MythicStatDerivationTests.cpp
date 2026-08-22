#include "Misc/AutomationTest.h"

#include "AI/NPCs/MythicNPCCharacter.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/MythicStatContribution.h"
#include "GameplayEffect.h"
#include "Settings/MythicCombatSettings.h"

namespace {
// Prefixed because UE unity builds merge translation units, so an anonymous namespace is no protection
// against a name another test file already uses.
const TCHAR *DerivationBossPawn = TEXT("/Game/Mythic/AI/NPCs/Tiers/Pawn_Humanoid_Boss.Pawn_Humanoid_Boss_C");

/**
 * True when this effect derives Target from something else rather than setting it to a fixed number.
 *
 * A custom calculation cannot resolve to a static magnitude, so GetStaticMagnitudeIfPossible failing is
 * precisely what distinguishes "computed from a stat" from "authored constant".
 */
bool DerivationHasComputedModifier(const UGameplayEffect *Effect, const FGameplayAttribute &Target) {
    if (!Effect) {
        return false;
    }
    for (const FGameplayModifierInfo &Mod : Effect->Modifiers) {
        if (Mod.Attribute != Target || Mod.ModifierOp != EGameplayModOp::MultiplyAdditive) {
            continue;
        }
        float Unused = 0.0f;
        if (!Mod.ModifierMagnitude.GetStaticMagnitudeIfPossible(1.0f, Unused)) {
            return true;
        }
    }
    return false;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicStrengthDerivesStatsTest,
    "Mythic.Combat.StrengthDerivesStats",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicStrengthDerivesStatsTest::RunTest(const FString &Parameters) {
    // THE REGRESSION THIS EXISTS FOR: the Strength mappings were authored, resolved correctly in isolation,
    // and nothing applied them. Two of the four mappings were live data feeding nothing - which reads as a
    // finished system right up until someone checks whether MaxHealth ever moves.
    const UClass *Loaded = LoadClass<AActor>(nullptr, DerivationBossPawn);
    const AMythicNPCCharacter *Pawn = Loaded ? Cast<AMythicNPCCharacter>(Loaded->GetDefaultObject()) : nullptr;
    if (!TestNotNull(TEXT("a tier pawn loads"), Pawn)) {
        return false;
    }

    bool bDerivesHealth = false;
    bool bDerivesArmor = false;
    for (const TSubclassOf<UGameplayEffect> &EffectClass : Pawn->GetDefaultGameplayEffects()) {
        const UGameplayEffect *Effect = EffectClass ? EffectClass->GetDefaultObject<UGameplayEffect>() : nullptr;
        bDerivesHealth |= DerivationHasComputedModifier(Effect, UMythicAttributeSet_Life::GetMaxHealthAttribute());
        bDerivesArmor |= DerivationHasComputedModifier(Effect, UMythicAttributeSet_Defense::GetArmorAttribute());
    }

    TestTrue(TEXT("something actually derives MaxHealth, not just an authored row that could"), bDerivesHealth);
    TestTrue(TEXT("something actually derives Armor"), bDerivesArmor);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicDerivedValuesMoveTest,
    "Mythic.Combat.DerivedValuesMove",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicDerivedValuesMoveTest::RunTest(const FString &Parameters) {
    const UMythicCombatSettings *Settings = GetDefault<UMythicCombatSettings>();
    if (!Settings || Settings->StatContributions.Contributions.Num() == 0) {
        AddError(TEXT("the primary stat model is not authored"));
        return false;
    }
    const TArray<FMythicStatContribution> &Rows = Settings->StatContributions.Contributions;

    auto WithStrength = [&Rows](const FGameplayAttribute &Target, float Base, float Strength) {
        return FMythicStatContributionRules::ApplyToBase(
            Rows, Target, Base, [Strength](const FGameplayAttribute &) -> float { return Strength; });
    };

    const FGameplayAttribute Health = UMythicAttributeSet_Life::GetMaxHealthAttribute();
    const FGameplayAttribute Armor = UMythicAttributeSet_Defense::GetArmorAttribute();

    TestTrue(TEXT("Strength raises max health above the flat baseline"),
             WithStrength(Health, 500.0f, 10.0f) > 500.0f);
    TestTrue(TEXT("more Strength means more health"),
             WithStrength(Health, 500.0f, 50.0f) > WithStrength(Health, 500.0f, 10.0f));
    TestTrue(TEXT("Strength raises armor too"), WithStrength(Armor, 10.0f, 10.0f) > 10.0f);

    // MaxHealth is deliberately uncapped (CeilingBonus 0) because health must keep pace with level forever,
    // while Armor bends. Assert the two really are configured differently, or the "independent scaling"
    // requirement has quietly collapsed into one shared curve.
    const float HealthGain = WithStrength(Health, 100.0f, 400.0f) / 100.0f;
    const float ArmorGain = WithStrength(Armor, 100.0f, 400.0f) / 100.0f;
    TestNotEqual(TEXT("health and armor scale off Strength differently"), HealthGain, ArmorGain);
    TestTrue(TEXT("armor is the one that bends"), ArmorGain < HealthGain);

    return true;
}

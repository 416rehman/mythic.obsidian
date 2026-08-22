#include "Misc/AutomationTest.h"

#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/MythicStatContribution.h"
#include "Settings/MythicCombatSettings.h"

namespace {
/** What a character actually hits for: the weapon roll lifted by Power's authored, diminished contribution. */
float WeaponDamageAt(TConstArrayView<FMythicStatContribution> Rows, float WeaponBase, float Power) {
    // The same function the damage execution calls, so this cannot pass while the real path is broken.
    return FMythicStatContributionRules::ApplyToBase(
        Rows, UMythicAttributeSet_Offense::GetDamagePerHitAttribute(), WeaponBase,
        [Power](const FGameplayAttribute &Attr) -> float {
            return Attr == UMythicAttributeSet_Offense::GetPowerAttribute() ? Power : 0.0f;
        });
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicPowerNotQuadraticTest,
    "Mythic.Combat.PowerNotQuadratic",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicPowerNotQuadraticTest::RunTest(const FString &Parameters) {
    // THE REGRESSION THIS EXISTS FOR: damage was Max(1.0f, Power) * WeaponRoll. Weapon damage rises with item
    // level and Power rises with character level, so the two multiplied: a character at twice the level with
    // twice the weapon dealt FOUR times the damage. No tuning pass fixes a quadratic - content is either
    // trivial or a wall, and the gap widens the further you get from wherever it was last tuned.
    const UMythicCombatSettings *Settings = GetDefault<UMythicCombatSettings>();
    if (!TestNotNull(TEXT("combat settings resolve"), Settings)) {
        return false;
    }
    const TArray<FMythicStatContribution> &Rows = Settings->StatContributions.Contributions;
    TestTrue(TEXT("the primary stat model is authored, not empty"), Rows.Num() > 0);

    // Double both axes at once. Quadratic would be 4x; the model must come in well under that.
    const float Base = WeaponDamageAt(Rows, 100.0f, 10.0f);
    const float Doubled = WeaponDamageAt(Rows, 200.0f, 20.0f);
    if (Base > KINDA_SMALL_NUMBER) {
        const float Ratio = Doubled / Base;
        TestTrue(FString::Printf(TEXT("doubling weapon and Power is sub-quadratic (was %.2fx)"), Ratio),
                 Ratio < 4.0f);
        TestTrue(TEXT("but doubling still meaningfully increases damage"), Ratio > 2.0f);
    }

    /**
     * Measured against the quadratic it replaced, at several levels.
     *
     * Each step below quadruples BOTH weapon and Power, so the old formula would return 16x per step. The
     * check is that each step stays far under that.
     *
     * Deliberately NOT asserting that the step ratio falls monotonically. It does not, and that is correct:
     * the contribution itself decelerates, but damage is (1 + f), and the +1 damps a small f's ratio more than
     * a large one's, so the total ratio can still creep up while f is genuinely bending. Asserting monotonic
     * deceleration here would be asserting a property the model does not have and should not need.
     */
    const float Steps[][2] = {{100.0f, 10.0f}, {400.0f, 40.0f}, {1600.0f, 160.0f}};
    const float QuadraticStep = 16.0f;
    for (int32 i = 1; i < UE_ARRAY_COUNT(Steps); ++i) {
        const float Prev = WeaponDamageAt(Rows, Steps[i - 1][0], Steps[i - 1][1]);
        const float Curr = WeaponDamageAt(Rows, Steps[i][0], Steps[i][1]);
        if (Prev <= KINDA_SMALL_NUMBER) {
            continue;
        }
        const float Ratio = Curr / Prev;
        TestTrue(FString::Printf(TEXT("step %d grows %.2fx, well under the quadratic %.0fx"), i, Ratio, QuadraticStep),
                 Ratio < QuadraticStep * 0.6f);
    }

    // And the contribution itself must genuinely decelerate, which is the diminishing claim.
    auto Contribution = [&Rows](float Power) {
        return FMythicStatContributionRules::ResolveTarget(
            Rows, UMythicAttributeSet_Offense::GetDamagePerHitAttribute(),
            [Power](const FGameplayAttribute &Attr) -> float {
                return Attr == UMythicAttributeSet_Offense::GetPowerAttribute() ? Power : 0.0f;
            });
    };
    const float F1 = Contribution(10.0f), F2 = Contribution(40.0f), F3 = Contribution(160.0f);
    if (F1 > KINDA_SMALL_NUMBER && F2 > KINDA_SMALL_NUMBER) {
        TestTrue(TEXT("Power's contribution decelerates as it stacks"), (F3 / F2) < (F2 / F1));
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicPowerDeadZoneTest,
    "Mythic.Combat.PowerDeadZone",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicPowerDeadZoneTest::RunTest(const FString &Parameters) {
    const UMythicCombatSettings *Settings = GetDefault<UMythicCombatSettings>();
    if (!Settings || Settings->StatContributions.Contributions.Num() == 0) {
        AddError(TEXT("the primary stat model is not authored"));
        return false;
    }
    const TArray<FMythicStatContribution> &Rows = Settings->StatContributions.Contributions;

    // Max(1.0f, Power) made every Power from 0 to 1 produce identical damage, so early investment did nothing
    // and any debuff pushing Power below 1 was silently inert.
    const float AtZero = WeaponDamageAt(Rows, 100.0f, 0.0f);
    const float AtHalf = WeaponDamageAt(Rows, 100.0f, 0.5f);
    const float AtOne = WeaponDamageAt(Rows, 100.0f, 1.0f);

    TestTrue(TEXT("half a point of Power beats none"), AtHalf > AtZero);
    TestTrue(TEXT("a full point beats half"), AtOne > AtHalf);

    // Zero Power must still swing for the weapon's own damage, not for nothing - that is what the old clamp
    // was there to prevent, and the fix must not reintroduce the problem it was guarding against.
    TestTrue(TEXT("zero Power still deals the weapon's damage"), AtZero > 0.0f);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicOnePointBoundTest,
    "Mythic.Combat.OnePointBound",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicOnePointBoundTest::RunTest(const FString &Parameters) {
    const UMythicCombatSettings *Settings = GetDefault<UMythicCombatSettings>();
    if (!Settings || Settings->StatContributions.Contributions.Num() == 0) {
        AddError(TEXT("the primary stat model is not authored"));
        return false;
    }
    const TArray<FMythicStatContribution> &Rows = Settings->StatContributions.Contributions;

    // The house rule: one point of a primary must never come close to doubling output. Asserted at both ends
    // of the range, because a curve can be well-behaved high up and still be absurd at level 1.
    const float MaxSingleStep = 0.25f;
    for (const float Power : {1.0f, 10.0f, 200.0f}) {
        const float Before = WeaponDamageAt(Rows, 100.0f, Power);
        const float After = WeaponDamageAt(Rows, 100.0f, Power + 1.0f);
        if (Before <= KINDA_SMALL_NUMBER) {
            continue;
        }
        const float Step = (After - Before) / Before;
        TestTrue(FString::Printf(TEXT("one point at Power %.0f adds %.1f%%, under the %.0f%% bound"),
                                 Power, Step * 100.0f, MaxSingleStep * 100.0f),
                 Step < MaxSingleStep);
        TestTrue(FString::Printf(TEXT("one point at Power %.0f is still worth something"), Power), Step > 0.0f);
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicIndependentMappingsTest,
    "Mythic.Combat.IndependentMappings",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicIndependentMappingsTest::RunTest(const FString &Parameters) {
    const UMythicCombatSettings *Settings = GetDefault<UMythicCombatSettings>();
    if (!Settings) {
        AddError(TEXT("combat settings did not resolve"));
        return false;
    }
    const TArray<FMythicStatContribution> &Rows = Settings->StatContributions.Contributions;

    // The central requirement: Power to weapon damage and Power to skill damage are two rows with their own
    // scaling, not one coefficient applied twice. If they ever produce the same number for the same input,
    // the independence has been lost and a designer can no longer tune skills apart from weapons.
    auto Read = [](float Value) {
        return [Value](const FGameplayAttribute &) -> float { return Value; };
    };
    const float Weapon = FMythicStatContributionRules::ResolveTarget(
        Rows, UMythicAttributeSet_Offense::GetDamagePerHitAttribute(), Read(50.0f));
    const float Skill = FMythicStatContributionRules::ResolveTarget(
        Rows, UMythicAttributeSet_Offense::GetBonusSkillDamageAttribute(), Read(50.0f));

    TestTrue(TEXT("Power feeds weapon damage"), Weapon > 0.0f);
    TestTrue(TEXT("Power feeds skill damage"), Skill > 0.0f);
    TestNotEqual(TEXT("and the two scale independently"), Weapon, Skill);

    // Strength drives survivability, and its two mappings are likewise separate.
    const float Health = FMythicStatContributionRules::ResolveTarget(
        Rows, UMythicAttributeSet_Life::GetMaxHealthAttribute(), Read(50.0f));
    const float Armor = FMythicStatContributionRules::ResolveTarget(
        Rows, UMythicAttributeSet_Defense::GetArmorAttribute(), Read(50.0f));

    TestTrue(TEXT("Strength feeds max health"), Health > 0.0f);
    TestTrue(TEXT("Strength feeds armor"), Armor > 0.0f);
    TestNotEqual(TEXT("and those two scale independently too"), Health, Armor);

    return true;
}

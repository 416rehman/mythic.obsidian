
#include "Misc/AutomationTest.h"

#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/MythicStatContribution.h"

namespace {
FMythicStatContribution Row(const FGameplayAttribute &Source, const FGameplayAttribute &Target, float PerPoint,
                            float SoftCap = 1.0f, float Ceiling = 0.0f) {
    FMythicStatContribution R;
    R.SourceStat = Source;
    R.TargetAttribute = Target;
    R.PerPoint = PerPoint;
    R.SoftCapBonus = SoftCap;
    R.CeilingBonus = Ceiling;
    return R;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicStatContributionTest,
    "Mythic.Combat.StatContribution",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicStatContributionTest::RunTest(const FString &Parameters) {
    using Rules = FMythicStatContributionRules;

    const FGameplayAttribute Power = UMythicAttributeSet_Offense::GetPowerAttribute();
    const FGameplayAttribute Weapon = UMythicAttributeSet_Offense::GetDamagePerHitAttribute();
    const FGameplayAttribute Health = UMythicAttributeSet_Life::GetMaxHealthAttribute();

    // A row missing either end, or worth nothing per point, is authored and inert. Returning a contribution for
    // it would make a half-authored row look like it works.
    TestFalse(TEXT("a row with no source is dead"), Rules::IsRowLive(Row(FGameplayAttribute(), Weapon, 0.01f)));
    TestFalse(TEXT("a row with no target is dead"), Rules::IsRowLive(Row(Power, FGameplayAttribute(), 0.01f)));
    TestFalse(TEXT("a row worth nothing per point is dead"), Rules::IsRowLive(Row(Power, Weapon, 0.0f)));
    TestTrue(TEXT("a complete row is live"), Rules::IsRowLive(Row(Power, Weapon, 0.01f)));

    // Uncurved: correct for something that must keep pace with level forever, like health.
    const FMythicStatContribution Linear = Row(Power, Health, 0.01f);
    TestEqual(TEXT("no stat, no contribution"), Rules::ResolveRow(Linear, 0.0f), 0.0f);
    TestEqual(TEXT("ten points is ten percent"), Rules::ResolveRow(Linear, 10.0f), 0.1f);
    TestEqual(TEXT("a thousand points keeps growing"), Rules::ResolveRow(Linear, 1000.0f), 10.0f);

    // Curved: correct for a percentage bonus that must not run away.
    const FMythicStatContribution Curved = Row(Power, Weapon, 0.01f, 1.0f, 4.0f);
    TestEqual(TEXT("below the soft cap a curved row is face value"), Rules::ResolveRow(Curved, 50.0f), 0.5f);
    TestEqual(TEXT("at the soft cap it is still face value"), Rules::ResolveRow(Curved, 100.0f), 1.0f);
    TestTrue(TEXT("past it the row bends"), Rules::ResolveRow(Curved, 300.0f) < 3.0f);
    TestTrue(TEXT("but still beats the soft cap"), Rules::ResolveRow(Curved, 300.0f) > 1.0f);
    TestTrue(TEXT("and never reaches the ceiling"), Rules::ResolveRow(Curved, 1.0e6f) < 4.0f);

    // The owner's concern: one point must never come close to doubling output. At the very bottom of the curve,
    // where a single point is worth the largest share of a small total, it is still one percent.
    const float AtOne = Rules::ResolveRow(Curved, 1.0f);
    const float AtTwo = Rules::ResolveRow(Curved, 2.0f);
    TestTrue(TEXT("one point at level one is a small step"), (AtTwo - AtOne) < 0.02f);

    // Two stats feeding one target must SUM, not multiply. Multiplying is how growth goes quadratic.
    const FGameplayAttribute Strength = UMythicAttributeSet_Offense::GetPowerAttribute();
    TArray<FMythicStatContribution> Rows;
    Rows.Add(Row(Power, Weapon, 0.01f));
    Rows.Add(Row(Strength, Weapon, 0.02f));
    Rows.Add(Row(Power, Health, 0.05f));

    auto ReadFifty = [](const FGameplayAttribute &) { return 50.0f; };
    TestEqual(TEXT("rows feeding one target sum"), Rules::ResolveTarget(Rows, Weapon, ReadFifty), 1.5f);
    TestEqual(TEXT("a different target sums only its own rows"), Rules::ResolveTarget(Rows, Health, ReadFifty), 2.5f);

    // A target nothing feeds contributes nothing rather than defaulting to something.
    TestEqual(TEXT("an unfed target contributes nothing"),
              Rules::ResolveTarget(Rows, UMythicAttributeSet_Offense::GetAttackSpeedAttribute(), ReadFifty), 0.0f);

    TArray<FGameplayAttribute> Targets;
    Rules::GatherTargets(Rows, Targets);
    TestEqual(TEXT("every fed target is enumerated once"), Targets.Num(), 2);

    Rows.Add(Row(Power, FGameplayAttribute(), 0.5f));
    Rules::GatherTargets(Rows, Targets);
    TestEqual(TEXT("a dead row is not enumerated"), Targets.Num(), 2);

    return true;
}

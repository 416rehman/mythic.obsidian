#include "Misc/AutomationTest.h"

#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/MythicStatContribution.h"
#include "Settings/MythicCombatSettings.h"
#include "UI/ViewModels/MythicStatDisplay.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicPrimaryStatTierTest,
    "Mythic.UI.PrimaryStatTier",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicPrimaryStatTierTest::RunTest(const FString &Parameters) {
    // The panel has a causal structure - Primary feeds Derived feeds Summarized - and that causality is the
    // mental model. A primary listed among its own outputs hides it.
    const FMythicStatRule PowerRule = MythicStatDisplay::GetRule(UMythicAttributeSet_Offense::GetPowerAttribute());
    const FMythicStatRule StrengthRule = MythicStatDisplay::GetRule(UMythicAttributeSet_Defense::GetStrengthAttribute());

    TestEqual(TEXT("Power is a primary, not an offense stat among its own outputs"),
              PowerRule.Category, EMythicStatCategory::Primary);
    TestEqual(TEXT("Strength is a primary"), StrengthRule.Category, EMythicStatCategory::Primary);

    // Things a primary derives must NOT themselves be primary, or the tier stops meaning anything.
    const FMythicStatRule DamageRule =
        MythicStatDisplay::GetRule(UMythicAttributeSet_Offense::GetDamagePerHitAttribute());
    const FMythicStatRule ArmorRule = MythicStatDisplay::GetRule(UMythicAttributeSet_Defense::GetArmorAttribute());
    TestNotEqual(TEXT("damage per hit is derived, not primary"), DamageRule.Category, EMythicStatCategory::Primary);
    TestNotEqual(TEXT("armor is derived, not primary"), ArmorRule.Category, EMythicStatCategory::Primary);

    TestFalse(TEXT("the primary tier has a heading of its own"),
              MythicStatDisplay::GetCategoryLabel(EMythicStatCategory::Primary).IsEmpty());

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicTooltipMatchesGameplayTest,
    "Mythic.UI.TooltipMatchesGameplay",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicTooltipMatchesGameplayTest::RunTest(const FString &Parameters) {
    // THE DEFECT THIS EXISTS FOR: a hand-written tooltip drifts silently the moment a designer retunes a
    // coefficient. The figure a player reads has to be produced by the same rows that decide what hits.
    const UMythicCombatSettings *Settings = GetDefault<UMythicCombatSettings>();
    if (!Settings || Settings->StatContributions.Contributions.Num() == 0) {
        AddError(TEXT("the primary stat model is not authored"));
        return false;
    }
    const TArray<FMythicStatContribution> &Rows = Settings->StatContributions.Contributions;
    const FGameplayAttribute Power = UMythicAttributeSet_Offense::GetPowerAttribute();

    // Every row the tooltip would show for Power must resolve to what gameplay resolves for the same value.
    const float PowerValue = 42.0f;
    int32 Shown = 0;
    for (const FMythicStatContribution &Row : Rows) {
        if (Row.SourceStat != Power || !FMythicStatContributionRules::IsRowLive(Row)) {
            continue;
        }
        ++Shown;

        const float TooltipFraction = FMythicStatContributionRules::ResolveRow(Row, PowerValue);
        const float GameplayFraction = FMythicStatContributionRules::ResolveTarget(
            Rows, Row.TargetAttribute,
            [PowerValue, &Power](const FGameplayAttribute &Attr) -> float {
                return Attr == Power ? PowerValue : 0.0f;
            });
        TestEqual(TEXT("the tooltip figure is the gameplay figure"), TooltipFraction, GameplayFraction);

        // And it must be a real number a player can act on, not a silent zero.
        TestTrue(TEXT("a primary at 42 is contributing something"), TooltipFraction > 0.0f);

        // Named by the shared rule table rather than a string typed into the tooltip.
        const FString Label = MythicStatDisplay::GetRule(Row.TargetAttribute).Label.IsEmpty()
                                  ? MythicStatDisplay::MakeFriendlyLabel(Row.TargetAttribute.GetName())
                                  : MythicStatDisplay::GetRule(Row.TargetAttribute).Label;
        TestFalse(TEXT("every contribution line has a name"), Label.IsEmpty());
    }

    TestTrue(TEXT("Power has contributions to show at all"), Shown > 0);

    return true;
}

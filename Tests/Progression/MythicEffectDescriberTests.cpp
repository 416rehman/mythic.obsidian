
#include "Misc/AutomationTest.h"

#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "Itemization/Inventory/Fragments/FragmentTypes.h"
#include "UI/ViewModels/MythicEffectDescriber.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicEffectDescriberTest,
    "Mythic.UI.EffectDescriber",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicEffectDescriberTest::RunTest(const FString &Parameters) {
    const FGameplayAttribute Armor = UMythicAttributeSet_Defense::GetArmorAttribute();
    const FGameplayAttribute Crit = UMythicAttributeSet_Offense::GetCriticalHitChanceAttribute();

    {
        const FMythicEffectLine Line = MythicEffectDescriber::DescribeModifier(Armor, 12.0f);
        TestEqual(TEXT("armour is named, not printed as a property"), Line.Label.ToString(), FString(TEXT("Armor")));
        TestTrue(TEXT("a positive armour change reads as good"), Line.bPositive);
        TestTrue(TEXT("the value carries its sign"), Line.Value.ToString().StartsWith(TEXT("+")));
        TestTrue(TEXT("the line is wrapped in the project's Roll markup"),
                 Line.RichText.ToString().StartsWith(TEXT("<Roll>")));
        TestFalse(TEXT("a fixed value shows no invented range"), Line.RichText.ToString().Contains(TEXT("<Context>")));
    }

    {
        const FMythicEffectLine Line = MythicEffectDescriber::DescribeModifier(Crit, 0.05f);
        TestTrue(TEXT("a chance attribute reads as a percentage"), Line.Value.ToString().Contains(TEXT("%")));
        TestFalse(TEXT("a chance attribute does not read as a raw decimal"), Line.Value.ToString().Contains(TEXT("0.05")));
        TestEqual(TEXT("crit is named properly"), Line.Label.ToString(), FString(TEXT("Critical Hit Chance")));
    }

    {
        FRollDefinition Roll;
        Roll.Min = 0.10f;
        Roll.Max = 0.25f;
        Roll.bIsPercentage = true;
        Roll.Modifier = EGameplayModOp::Additive;

        const FMythicEffectLine Line = MythicEffectDescriber::DescribeRolledModifier(Crit, 0.15f, Roll);
        TestFalse(TEXT("a rolled affix shows its range"), Line.Range.IsEmpty());
        TestTrue(TEXT("the range is wrapped in Context markup"), Line.RichText.ToString().Contains(TEXT("<Context>")));
        TestTrue(TEXT("the range names both ends"), Line.Range.ToString().Contains(TEXT("-")));
    }

    {
        FRollDefinition Fixed;
        Fixed.Min = 0.20f;
        Fixed.Max = 0.20f;
        Fixed.bIsPercentage = true;
        const FMythicEffectLine Line = MythicEffectDescriber::DescribeRolledModifier(Crit, 0.20f, Fixed);
        TestTrue(TEXT("a fixed roll has no range"), Line.Range.IsEmpty());
        TestFalse(TEXT("and no Context markup"), Line.RichText.ToString().Contains(TEXT("<Context>")));
    }

    {
        FRollDefinition Cooldown;
        Cooldown.Min = -0.10f;
        Cooldown.Max = -0.02f;
        Cooldown.bLowerIsBetter = true;
        const FMythicEffectLine Line = MythicEffectDescriber::DescribeRolledModifier(Crit, -0.05f, Cooldown);
        TestTrue(TEXT("a reduction on a lower-is-better roll reads as good"), Line.bPositive);
    }

    {
        const FMythicEffectLine Line = MythicEffectDescriber::DescribeModifier(FGameplayAttribute(), 5.0f);
        TestTrue(TEXT("an invalid attribute has no label"), Line.Label.IsEmpty());
        TestTrue(TEXT("an invalid attribute has no markup"), Line.RichText.IsEmpty());
    }

    {
        TestEqual(TEXT("a null effect has no lines"), MythicEffectDescriber::DescribeEffect(nullptr).Num(), 0);
        TestTrue(TEXT("a null effect summarises to nothing"),
                 MythicEffectDescriber::SummariseEffect(nullptr).IsEmpty());
    }

    return true;
}

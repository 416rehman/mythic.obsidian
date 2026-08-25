
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
        const FGameplayAttribute Power = UMythicAttributeSet_Offense::GetPowerAttribute();
        FRollDefinition Roll;
        Roll.Min = 4.0f;
        Roll.Max = 9.0f;
        Roll.bWholeNumber = true;
        const FMythicEffectLine Line = MythicEffectDescriber::DescribeRolledModifier(Power, 8.0f, Roll);
        TestFalse(TEXT("a flat point stat carries no percent sign"), Line.Value.ToString().Contains(TEXT("%")));
        TestEqual(TEXT("power grants whole points"), Line.Value.ToString(), FString(TEXT("+8")));
        TestFalse(TEXT("and its range is not a percentage either"), Line.Range.ToString().Contains(TEXT("%")));
    }

    {
        const FGameplayAttribute Dur = UMythicAttributeSet_Offense::GetBurnDurationMultiplierAttribute();
        FRollDefinition Roll;
        Roll.Min = 0.06f;
        Roll.Max = 0.16f;
        Roll.Modifier = EGameplayModOp::Additive;

        const FMythicEffectLine Line = MythicEffectDescriber::DescribeRolledModifier(Dur, 0.10f, Roll);
        TestEqual(TEXT("an additive roll on a multiplier stat reads as percentage points"),
                  Line.Value.ToString(), FString(TEXT("+10%")));
        TestFalse(TEXT("it never subtracts a whole multiplier from the delta"),
                  Line.Range.ToString().Contains(TEXT("-9")));
        TestEqual(TEXT("and its range agrees with the value"), Line.Range.ToString(), FString(TEXT("[6%-16%]")));
    }

    {
        const FGameplayAttribute Dmg = UMythicAttributeSet_Offense::GetDamagePerHitAttribute();
        FRollDefinition Roll;
        Roll.Min = 6.0f;
        Roll.Max = 11.0f;
        Roll.LevelScaling = 1.0f;
        Roll.bWholeNumber = true;

        const FMythicEffectLine AtLevel = MythicEffectDescriber::DescribeRolledModifier(Dmg, 60.0f, Roll, 50);
        TestEqual(TEXT("the range scales with item level, like the value it sits beside"),
                  AtLevel.Range.ToString(), FString(TEXT("[56-61]")));

        const FMythicEffectLine Unscaled = MythicEffectDescriber::DescribeRolledModifier(Dmg, 8.0f, Roll);
        TestEqual(TEXT("and an unscaled roll still prints its authored band"),
                  Unscaled.Range.ToString(), FString(TEXT("[6-11]")));
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

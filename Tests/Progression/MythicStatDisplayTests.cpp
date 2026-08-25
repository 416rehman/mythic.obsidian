
#include "Misc/AutomationTest.h"

#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Utility.h"
#include "UI/ViewModels/MythicStatDisplay.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicStatDisplayFormatsTest,
    "Mythic.UI.StatDisplayFormats",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicStatDisplayFormatsTest::RunTest(const FString &Parameters) {
    using U = UMythicAttributeSet_Utility;
    using O = UMythicAttributeSet_Offense;

    {
        const FMythicStatRule Rule = MythicStatDisplay::GetRule(U::GetItemQuantityFindAttribute());
        TestEqual(TEXT("quantity find is an expected multiplier, so a percentage"),
                  static_cast<uint8>(Rule.Format), static_cast<uint8>(EMythicStatFormat::Percent));
        TestEqual(TEXT("quantity find says what it does"), Rule.Label, FString(TEXT("Extra Item Drops")));

        const FString Shown = MythicStatDisplay::FormatValue(0.6f, Rule.Format).ToString();
        TestFalse(TEXT("a working quantity-find never prints as bare zero"), Shown == TEXT("0"));
        TestTrue(TEXT("quantity find reads as a percentage"), Shown.Contains(TEXT("%")));
    }

    {
        // The owner's rule for the one speed stat: 100% is default speed, 200% is double. The Multiplier format
        // would print the default 1.0 as "+0%", which reads as the stat being absent.
        const FMythicStatRule Rule = MythicStatDisplay::GetRule(U::GetMovementSpeedMultiplierAttribute());
        TestEqual(TEXT("speed is shown as a percentage of default"),
                  static_cast<uint8>(Rule.Format), static_cast<uint8>(EMythicStatFormat::Percent));
        TestEqual(TEXT("default speed reads as 100%"),
                  MythicStatDisplay::FormatValue(1.0f, Rule.Format).ToString(), FString(TEXT("100%")));
        TestEqual(TEXT("double speed reads as 200%"),
                  MythicStatDisplay::FormatValue(2.0f, Rule.Format).ToString(), FString(TEXT("200%")));
    }

    {
        const FMythicStatRule Rule = MythicStatDisplay::GetRule(U::GetProficiencyXPBonusAttribute());
        TestEqual(TEXT("proficiency xp bonus is a fraction shown as a percentage"),
                  static_cast<uint8>(Rule.Format), static_cast<uint8>(EMythicStatFormat::Percent));
    }

    {
        const FMythicStatRule Rule = MythicStatDisplay::GetRule(U::GetItemRarityFindAttribute());
        TestEqual(TEXT("rarity find is a fraction shown as a percentage"),
                  static_cast<uint8>(Rule.Format), static_cast<uint8>(EMythicStatFormat::Percent));
    }

    {
        const FMythicStatRule Rule = MythicStatDisplay::GetRule(O::GetCriticalHitChanceAttribute());
        TestEqual(TEXT("crit chance is a probability shown as a percentage"),
                  static_cast<uint8>(Rule.Format), static_cast<uint8>(EMythicStatFormat::Percent));
    }

    return true;
}

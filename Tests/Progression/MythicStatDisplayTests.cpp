#include "Misc/AutomationTest.h"

#include "Stats/MythicStatDefinition.h"
#include "UI/ViewModels/MythicStatDisplay.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicStatDisplayFormatsTest,
    "Mythic.UI.StatDisplayFormats",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicStatDisplayFormatsTest::RunTest(const FString& Parameters) {
    TestEqual(TEXT("the migrated enum keeps Flat's serialized value"),
              static_cast<uint8>(EMythicStatFormat::Flat), static_cast<uint8>(0));
    TestEqual(TEXT("the migrated enum keeps Bipolar's serialized value"),
              static_cast<uint8>(EMythicStatFormat::Bipolar), static_cast<uint8>(5));

    FMythicStatNumberPresentation Percent;
    Percent.Format = EMythicStatFormat::Percent;
    Percent.DecimalPlaces = 1;
    TestEqual(TEXT("a fraction is displayed as a percentage"),
              MythicStatDisplay::FormatValue(0.6f, Percent).ToString(), FString(TEXT("60%")));
    TestEqual(TEXT("default movement scale reads as 100 percent"),
              MythicStatDisplay::FormatValue(1.0f, Percent).ToString(), FString(TEXT("100%")));

    FMythicStatNumberPresentation Multiplier = Percent;
    Multiplier.Format = EMythicStatFormat::Multiplier;
    TestEqual(TEXT("canonical 0.97 incoming-damage multiplier reads as a three percent reduction"),
              MythicStatDisplay::FormatValue(0.97f, Multiplier).ToString(), FString(TEXT("-3%")));
    TestEqual(TEXT("a +0.06 additive contribution to a multiplier formats as six percentage points"),
              MythicStatDisplay::FormatBonus(0.06f, Percent).ToString(), FString(TEXT("+6%")));

    UMythicStatDefinition* Definition = NewObject<UMythicStatDefinition>(GetTransientPackage());
    Definition->NumberPresentation = Multiplier;
    const FMythicStatNumberPresentation Additive = MythicStatDisplay::ResolveModifierPresentation(
        *Definition, EGameplayModOp::Additive);
    TestEqual(TEXT("additive modifiers on multiplier stats use percent presentation"),
              Additive.Format, EMythicStatFormat::Percent);

    const FMythicStatNumberPresentation Compound = MythicStatDisplay::ResolveModifierPresentation(
        *Definition, EGameplayModOp::MultiplyCompound);
    TestEqual(TEXT("compound multipliers retain one-based multiplier presentation"),
              Compound.Format, EMythicStatFormat::Multiplier);
    TestEqual(TEXT("an additive contribution has zero identity even when the final stat is one-neutral"),
              MythicStatDisplay::GetModifierContributionIdentity(EGameplayModOp::AddBase, 1.0f), 0.0f);
    TestEqual(TEXT("a compound contribution has one identity even when the final stat is zero-neutral"),
              MythicStatDisplay::GetModifierContributionIdentity(EGameplayModOp::MultiplyCompound, 0.0f), 1.0f);

    Definition->NeutralValue = 1.0f;
    Definition->SheetVisibility = EMythicStatSheetVisibility::WhenModifiedOrNonNeutral;
    TestFalse(TEXT("a neutral unmodified stat stays hidden"),
              MythicStatDisplay::ShouldRender(*Definition, 1.0f, 1.0f));
    TestTrue(TEXT("a GAS base/final difference makes the row visible"),
             MythicStatDisplay::ShouldRender(*Definition, 1.0f, 0.97f));
    Definition->SheetVisibility = EMythicStatSheetVisibility::Hidden;
    TestFalse(TEXT("Hidden is absolute even for a modified stat"),
              MythicStatDisplay::ShouldRender(*Definition, 1.0f, 0.97f));

    return true;
}

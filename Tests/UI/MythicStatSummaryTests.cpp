// Copyright Stellar Games. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/MythicStatContribution.h"
#include "GAS/MythicStatSummary.h"
#include "Settings/MythicCombatSettings.h"
#include "Tests/UI/MythicStatSummaryTestTypes.h"
#include "UI/ViewModels/MythicStatDisplay.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicStatSummaryComputeTest,
    "Mythic.UI.StatSummaryCompute",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicStatSummaryComputeTest::RunTest(const FString &Parameters) {
    UMythicStatSummaryDefinition *Definition = NewObject<UMythicStatSummaryDefinition>(GetTransientPackage());

    // A half-authored row reads as a plain zero, never a crash or a stale number.
    TestEqual(TEXT("no calculation class computes zero"), Definition->Compute(nullptr), 0.0f);

    Definition->CalculationClass = UMythicStatSummaryCalculation_Fixed::StaticClass();
    TestEqual(TEXT("compute runs the authored calculation"), Definition->Compute(nullptr), 1234.5f);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicStatSummaryLibraryAuditTest,
    "Mythic.UI.StatSummaryLibraryAudit",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicStatSummaryLibraryAuditTest::RunTest(const FString &Parameters) {
    const UMythicStatDisplaySettings *Display = GetDefault<UMythicStatDisplaySettings>();
    if (!Display || Display->SummaryLibrary.IsNull()) {
        AddInfo(TEXT("0 summaries audited: no summary library authored yet - the cards section stays absent."));
        return true;
    }

    const UMythicStatSummaryLibrary *Library = Display->SummaryLibrary.LoadSynchronous();
    if (!Library) {
        AddError(TEXT("a summary library is configured but does not load"));
        return false;
    }

    const FGameplayTag SummaryRoot = FGameplayTag::RequestGameplayTag(TEXT("Stat.Summary"), false);
    int32 Audited = 0;
    for (const UMythicStatSummaryDefinition *Definition : Library->Summaries) {
        if (!Definition) {
            AddError(TEXT("the library holds a null definition row"));
            continue;
        }
        ++Audited;
        const FString Name = Definition->GetName();
        TestTrue(FString::Printf(TEXT("%s: SummaryId sits under Stat.Summary"), *Name),
                 SummaryRoot.IsValid() && Definition->SummaryId.MatchesTag(SummaryRoot));
        TestFalse(FString::Printf(TEXT("%s: label is authored"), *Name), Definition->Label.IsEmpty());
        TestTrue(FString::Printf(TEXT("%s: a calculation is authored"), *Name),
                 Definition->CalculationClass != nullptr);
    }
    AddInfo(FString::Printf(TEXT("%d of %d summaries audited."), Audited, Library->Summaries.Num()));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicPrimaryAttributeRoundTripTest,
    "Mythic.UI.PrimaryAttributeRoundTrip",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicPrimaryAttributeRoundTripTest::RunTest(const FString &Parameters) {
    // The tooltip asks for contributions BY the row's attribute; a primary whose attribute finds no rows
    // renders an empty hover, which reads as a broken feature rather than a missing one.
    const UMythicCombatSettings *Settings = GetDefault<UMythicCombatSettings>();
    if (!Settings) {
        AddError(TEXT("no combat settings"));
        return false;
    }
    const TArray<FMythicStatContribution> &Rows = Settings->StatContributions.Contributions;

    int32 PowerRows = 0;
    int32 StrengthRows = 0;
    for (const FMythicStatContribution &Row : Rows) {
        if (Row.SourceStat == UMythicAttributeSet_Offense::GetPowerAttribute()) {
            ++PowerRows;
        }
        if (Row.SourceStat == UMythicAttributeSet_Defense::GetStrengthAttribute()) {
            ++StrengthRows;
        }
    }
    AddInfo(FString::Printf(TEXT("%d rows total: %d from Power, %d from Strength."), Rows.Num(), PowerRows, StrengthRows));
    TestTrue(TEXT("Power feeds at least one contribution row"), PowerRows > 0);
    TestTrue(TEXT("Strength feeds at least one contribution row"), StrengthRows > 0);
    return true;
}

#include "Misc/AutomationTest.h"

#if WITH_EDITOR

#include "Itemization/Inventory/Fragments/Actionable/AttackFragment.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/MythicTags_Inventory.h"
#include "Misc/DataValidation.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicItemDefinitionFragmentValidationTest,
    "Mythic.Itemization.ItemDefinition.FragmentValidation",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicItemDefinitionFragmentValidationTest::RunTest(const FString &Parameters) {
    UItemDefinition *Definition = NewObject<UItemDefinition>(GetTransientPackage());
    if (!TestNotNull(TEXT("a transient Item Definition can be created"), Definition)) {
        return false;
    }
    Definition->ItemType = ITEMIZATION_TYPE_MISC;

    UAttackFragment *InvalidAttack = NewObject<UAttackFragment>(Definition);
    if (!TestNotNull(TEXT("a transient Attack Fragment can be created"), InvalidAttack)) {
        return false;
    }
    Definition->Fragments.Add(InvalidAttack);

    FDataValidationContext Context;
    TestTrue(TEXT("an invalid nested fragment invalidates its Item Definition"),
             Definition->IsDataValid(Context) == EDataValidationResult::Invalid);

    bool bFoundIndexedFragmentDiagnostic = false;
    for (const FDataValidationContext::FIssue &Issue : Context.GetIssues()) {
        const FString Message = Issue.Message.ToString();
        bFoundIndexedFragmentDiagnostic |= Message.Contains(TEXT("Fragment 0"))
            && Message.Contains(TEXT("AttackFragment"))
            && Message.Contains(TEXT("is invalid"));
    }
    TestTrue(TEXT("the diagnostic identifies the fragment index, type, and nested validation failure"),
             bFoundIndexedFragmentDiagnostic);
    return true;
}

#endif // WITH_EDITOR

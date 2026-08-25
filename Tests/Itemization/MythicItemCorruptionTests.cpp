#include "Misc/AutomationTest.h"

#include "Itemization/Inventory/Fragments/Passive/AffixesFragment.h"
#include "Settings/MythicDeveloperSettings.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicItemCorruptionTest,
                                 "Mythic.Itemization.Corruption",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/**
 * Corruption is the risk half of crafting: it has to actually refuse the crafting.
 *
 * The flag was replicated and saved on every item while nothing wrote it and nothing read it, and the comment
 * beside it described a gate function that had never been written. State that costs bandwidth and refuses
 * nothing is worse than no feature, because the settings page reports it as on.
 */
bool FMythicItemCorruptionTest::RunTest(const FString &Parameters) {
    UAffixesFragment *Fragment = NewObject<UAffixesFragment>();
    if (!TestNotNull(TEXT("an affixes fragment can be constructed"), Fragment)) {
        return false;
    }

    FText Reason;

    TestFalse(TEXT("a fresh item is not corrupted"), Fragment->IsCorrupted());
    TestTrue(TEXT("and accepts a craft op"), Fragment->CanApplyCraftOp(Reason));
    TestTrue(TEXT("with no refusal to explain"), Reason.IsEmpty());

    Fragment->AffixesRuntimeReplicatedData.bCorrupted = true;

    TestTrue(TEXT("a corrupted item reports itself corrupted"), Fragment->IsCorrupted());
    TestFalse(TEXT("and refuses a craft op"), Fragment->CanApplyCraftOp(Reason));

    // A refusal the player cannot read is a bug report waiting to happen.
    TestFalse(TEXT("and says why it refused"), Reason.IsEmpty());

    // The settings flag has to mean something, or the settings page lies about the feature being on.
    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    if (TestNotNull(TEXT("developer settings resolve"), Settings)) {
        AddInfo(FString::Printf(TEXT("item corruption enabled: %s"),
                                Settings->bItemCorruptionEnabled ? TEXT("true") : TEXT("false")));
    }

    return true;
}

#endif

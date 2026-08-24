#include "Misc/AutomationTest.h"

#include "GAS/Effects/MythicCrowdControl.h"
#include "Settings/MythicCombatSettings.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicCcEscalationTest,
    "Mythic.Combat.CcEscalation",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicCcEscalationTest::RunTest(const FString &Parameters) {
    using Rules = FMythicCrowdControlRules;

    const UMythicCombatSettings *Settings = GetDefault<UMythicCombatSettings>();
    if (!TestNotNull(TEXT("combat settings exist"), Settings)) {
        return false;
    }
    const TArray<FMythicCcTierEscalation> &Table = Settings->CcEscalationByTier;
    // The whole point of the issue: the ladder is authored data, not a C++ switch. An empty table would mean the
    // move to settings silently lost the values.
    if (!TestTrue(TEXT("the CC escalation ladder ships authored, not empty"), Table.Num() >= 5)) {
        return false;
    }

    // Behaviour preserved: each tier still resolves to the values that used to be hardcoded.
    const FMythicCcEscalationConfig T5 = Rules::ConfigForTier(Table, 5);
    TestEqual(TEXT("tier 5 escalation step"), T5.ThresholdEscalationStep, 1.0f);
    TestEqual(TEXT("tier 5 goes immune after 2"), T5.ImmunityTriggerCount, 2);
    TestEqual(TEXT("tier 5 immune seconds"), T5.ImmuneSeconds, 8.0f);

    const FMythicCcEscalationConfig T1 = Rules::ConfigForTier(Table, 1);
    TestEqual(TEXT("tier 1 escalation step"), T1.ThresholdEscalationStep, 0.25f);
    TestEqual(TEXT("tier 1 goes immune after 8"), T1.ImmunityTriggerCount, 8);

    // The ladder is monotonic where it matters: tougher enemies escalate faster and go immune sooner.
    TestTrue(TEXT("higher tiers escalate harder"), T5.ThresholdEscalationStep > T1.ThresholdEscalationStep);
    TestTrue(TEXT("higher tiers go immune sooner"), T5.ImmunityTriggerCount < T1.ImmunityTriggerCount);

    // An unlisted tier falls back to the struct defaults rather than resisting nothing, so a new tier is playable.
    const FMythicCcEscalationConfig Unlisted = Rules::ConfigForTier(Table, 99);
    TestEqual(TEXT("an unlisted tier uses the gentle default step"),
              Unlisted.ThresholdEscalationStep, FMythicCcEscalationConfig().ThresholdEscalationStep);

    return true;
}

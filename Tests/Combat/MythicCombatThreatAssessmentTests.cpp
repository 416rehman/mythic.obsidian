#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "GAS/Combat/MythicCombatThreatAssessment.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicCombatThreatPressureBandsTest,
    "Mythic.Combat.ThreatAssessment.PressureBands",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicCombatThreatPressureBandsTest::RunTest(const FString &Parameters) {
    FMythicCombatThreatAssessmentInputs Inputs;
    Inputs.bAssessmentPermitted = true;
    Inputs.bCombatCapable = true;
    Inputs.ViewerEffectivePressure = 1.0f;

    Inputs.SubjectEffectivePressure = 1.34f;
    TestEqual(TEXT("below 1.35 pressure is unmarked"), FMythicCombatThreatAssessment::Assess(Inputs),
              EMythicThreatBand::None);
    Inputs.SubjectEffectivePressure = 1.35f;
    TestEqual(TEXT("1.35 pressure begins Risky"), FMythicCombatThreatAssessment::Assess(Inputs),
              EMythicThreatBand::Risky);
    Inputs.SubjectEffectivePressure = 2.25f;
    TestEqual(TEXT("2.25 pressure begins Deadly"), FMythicCombatThreatAssessment::Assess(Inputs),
              EMythicThreatBand::Deadly);
    Inputs.SubjectEffectivePressure = 4.0f;
    TestEqual(TEXT("4.0 pressure begins Overwhelming"), FMythicCombatThreatAssessment::Assess(Inputs),
              EMythicThreatBand::Overwhelming);

    Inputs.bAssessmentPermitted = false;
    TestEqual(TEXT("permission gate returns Unknown"), FMythicCombatThreatAssessment::Assess(Inputs),
              EMythicThreatBand::Unknown);
    Inputs.bAssessmentPermitted = true;
    Inputs.bCombatCapable = false;
    TestEqual(TEXT("noncombatant gate suppresses danger"), FMythicCombatThreatAssessment::Assess(Inputs),
              EMythicThreatBand::None);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicCombatThreatKnowledgeGatesTest,
    "Mythic.Combat.ThreatAssessment.KnowledgeAndRankGates",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicCombatThreatKnowledgeGatesTest::RunTest(const FString &Parameters) {
    FMythicCombatThreatAssessmentInputs Inputs;
    Inputs.bAssessmentPermitted = true;
    Inputs.bCombatCapable = true;
    Inputs.ViewerEffectivePressure = 100.0f;
    Inputs.SubjectEffectivePressure = 100.0f;

    Inputs.bImmuneToViewerDamage = true;
    Inputs.bImmunityKnownToViewer = false;
    TestEqual(TEXT("unknown immunity does not leak"), FMythicCombatThreatAssessment::Assess(Inputs),
              EMythicThreatBand::None);
    Inputs.bImmunityKnownToViewer = true;
    TestEqual(TEXT("known total immunity is Overwhelming"), FMythicCombatThreatAssessment::Assess(Inputs),
              EMythicThreatBand::Overwhelming);

    Inputs.bImmuneToViewerDamage = false;
    Inputs.bImmunityKnownToViewer = false;
    Inputs.bDamageable = false;
    Inputs.bDamageabilityKnownToViewer = false;
    TestEqual(TEXT("unknown invulnerability does not leak"), FMythicCombatThreatAssessment::Assess(Inputs),
              EMythicThreatBand::None);
    Inputs.bDamageabilityKnownToViewer = true;
    TestEqual(TEXT("known invulnerability is Overwhelming"), FMythicCombatThreatAssessment::Assess(Inputs),
              EMythicThreatBand::Overwhelming);

    Inputs.bDamageable = true;
    Inputs.bDamageabilityKnownToViewer = false;
    Inputs.Rank = EMythicCombatThreatRank::Elite;
    Inputs.bRankKnownToViewer = false;
    TestEqual(TEXT("unknown elite rank adds no floor"), FMythicCombatThreatAssessment::Assess(Inputs),
              EMythicThreatBand::None);
    Inputs.bRankKnownToViewer = true;
    TestEqual(TEXT("known elite has Risky floor"), FMythicCombatThreatAssessment::Assess(Inputs),
              EMythicThreatBand::Risky);
    Inputs.Rank = EMythicCombatThreatRank::Boss;
    TestEqual(TEXT("known boss has Deadly floor"), FMythicCombatThreatAssessment::Assess(Inputs),
              EMythicThreatBand::Deadly);
    Inputs.Rank = EMythicCombatThreatRank::WorldBoss;
    TestEqual(TEXT("known world boss has Overwhelming floor"), FMythicCombatThreatAssessment::Assess(Inputs),
              EMythicThreatBand::Overwhelming);

    Inputs.Rank = EMythicCombatThreatRank::NonCombatant;
    Inputs.bImmuneToViewerDamage = true;
    Inputs.bImmunityKnownToViewer = true;
    TestEqual(TEXT("known noncombatant remains None before immunity warning"),
              FMythicCombatThreatAssessment::Assess(Inputs), EMythicThreatBand::None);

    Inputs.Rank = EMythicCombatThreatRank::Standard;
    Inputs.bRankKnownToViewer = false;
    Inputs.ViewerEffectivePressure = 0.0f;
    TestEqual(TEXT("uninitialized viewer pressure fails to Unknown"), FMythicCombatThreatAssessment::Assess(Inputs),
              EMythicThreatBand::Unknown);
    Inputs.Rank = EMythicCombatThreatRank::Boss;
    Inputs.bRankKnownToViewer = true;
    TestEqual(TEXT("known boss still supplies a safe floor when pressure is unavailable"),
              FMythicCombatThreatAssessment::Assess(Inputs), EMythicThreatBand::Deadly);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

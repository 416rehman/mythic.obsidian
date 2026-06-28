// Mythic — objective prerequisite (multi-step quest chain) unit tests.
// Covers the pure chain gate: AreObjectivePrerequisitesMet + its effect on the ResolveObjectiveOfferResult offer
// decision. The live quest-giver assignment is server-driven and PIE-verified; this locks the gate logic.
// Run via: Session Frontend → Automation → Mythic.Objectives.Prerequisites

#include "Misc/AutomationTest.h"
#include "Objectives/ObjectiveDefinition.h"
#include "Objectives/ObjectiveTracker.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicObjectivePrereqTest,
    "Mythic.Objectives.Prerequisites",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicObjectivePrereqTest::RunTest(const FString &Parameters) {
    // Two chain steps: B requires A. (Fresh UObjectiveDefinitions default to TriggerEventTag=GAS.Event.Kill,
    // RequiredCount=1, so they are valid offers.) Raw pointers are safe — the test runs synchronously, no GC mid-test.
    UObjectiveDefinition *StepA = NewObject<UObjectiveDefinition>();
    UObjectiveDefinition *StepB = NewObject<UObjectiveDefinition>();
    StepB->PrerequisiteObjectives.Add(StepA);

    // ── AreObjectivePrerequisitesMet(Prerequisites, Tracked) ──
    {
        TArray<FObjectiveProgress> Tracked;
        const TArray<TObjectPtr<UObjectiveDefinition>> NoPrereqs;
        TestTrue(TEXT("no prerequisites → always met"), UObjectiveTracker::AreObjectivePrerequisitesMet(NoPrereqs, Tracked));
        TestFalse(TEXT("untracked prerequisite → not met"),
                  UObjectiveTracker::AreObjectivePrerequisitesMet(StepB->PrerequisiteObjectives, Tracked));

        FObjectiveProgress ProgA;
        ProgA.Definition = StepA;
        ProgA.bCompleted = false;
        Tracked.Add(ProgA);
        TestFalse(TEXT("tracked-but-incomplete prerequisite → not met"),
                  UObjectiveTracker::AreObjectivePrerequisitesMet(StepB->PrerequisiteObjectives, Tracked));

        Tracked[0].bCompleted = true;
        TestTrue(TEXT("completed prerequisite → met"),
                 UObjectiveTracker::AreObjectivePrerequisitesMet(StepB->PrerequisiteObjectives, Tracked));

        // A null prerequisite entry is ignored (designer slop), not a blocker.
        StepB->PrerequisiteObjectives.Add(nullptr);
        TestTrue(TEXT("a null prerequisite entry is ignored"),
                 UObjectiveTracker::AreObjectivePrerequisitesMet(StepB->PrerequisiteObjectives, Tracked));
    }

    // ── The gate's effect on ResolveObjectiveOfferResult ──
    {
        TArray<FObjectiveProgress> Tracked;
        FObjectiveProgress ProgA;
        ProgA.Definition = StepA;
        ProgA.bCompleted = false;
        Tracked.Add(ProgA);

        FObjectiveProgress Out;
        TestEqual(TEXT("offer gated while the prerequisite is incomplete"),
                  UObjectiveTracker::ResolveObjectiveOfferResult(Tracked, StepB, Out),
                  EObjectiveOfferResult::PrerequisitesNotMet);

        Tracked[0].bCompleted = true;
        TestEqual(TEXT("offer assigned once the prerequisite is complete"),
                  UObjectiveTracker::ResolveObjectiveOfferResult(Tracked, StepB, Out),
                  EObjectiveOfferResult::Assigned);
    }

    // A prereq-free objective still assigns immediately (the original, non-chained behaviour).
    {
        TArray<FObjectiveProgress> Empty;
        FObjectiveProgress Out;
        TestEqual(TEXT("a prereq-free objective assigns immediately"),
                  UObjectiveTracker::ResolveObjectiveOfferResult(Empty, StepA, Out),
                  EObjectiveOfferResult::Assigned);
    }

    return true;
}

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

// ─── Chain auto-advance: CollectAssignableNextObjectives picks the next step(s) on completion ───
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicObjectiveChainAdvanceTest,
    "Mythic.Objectives.ChainAdvance",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicObjectiveChainAdvanceTest::RunTest(const FString &Parameters) {
    // Chain: A → {B, C}; C is a CONVERGING step that requires B. (Fresh defs default to a valid trigger + count.)
    UObjectiveDefinition *A = NewObject<UObjectiveDefinition>();
    UObjectiveDefinition *B = NewObject<UObjectiveDefinition>();
    UObjectiveDefinition *C = NewObject<UObjectiveDefinition>();
    A->NextObjectives = {B, C};
    C->PrerequisiteObjectives = {B};

    // A just completed: B (no prereqs) is assignable; C (needs B, not done) is not.
    {
        TArray<FObjectiveProgress> Tracked;
        FObjectiveProgress PA;
        PA.Definition = A;
        PA.bCompleted = true;
        Tracked.Add(PA);

        TArray<UObjectiveDefinition *> Out;
        UObjectiveTracker::CollectAssignableNextObjectives(A->NextObjectives, Tracked, Out);
        TestTrue(TEXT("B is assignable after A completes"), Out.Contains(B));
        TestFalse(TEXT("C waits for its prerequisite B"), Out.Contains(C));
        TestEqual(TEXT("only B unlocks first"), Out.Num(), 1);

        // Now B is also complete: re-evaluating A's next steps yields C (B already tracked → skipped; C's prereq met).
        FObjectiveProgress PB;
        PB.Definition = B;
        PB.bCompleted = true;
        Tracked.Add(PB);
        Out.Reset();
        UObjectiveTracker::CollectAssignableNextObjectives(A->NextObjectives, Tracked, Out);
        TestFalse(TEXT("B is not re-assigned (already tracked)"), Out.Contains(B));
        TestTrue(TEXT("C unlocks once B is complete"), Out.Contains(C));
        TestEqual(TEXT("only C now"), Out.Num(), 1);
    }

    // Cycle safety: an objective whose NextObjectives points back at itself never re-assigns once tracked.
    {
        UObjectiveDefinition *Loop = NewObject<UObjectiveDefinition>();
        Loop->NextObjectives = {Loop};
        TArray<FObjectiveProgress> Tracked;
        FObjectiveProgress PL;
        PL.Definition = Loop;
        PL.bCompleted = true;
        Tracked.Add(PL);

        TArray<UObjectiveDefinition *> Out;
        UObjectiveTracker::CollectAssignableNextObjectives(Loop->NextObjectives, Tracked, Out);
        TestEqual(TEXT("a self-cycle does not re-assign"), Out.Num(), 0);
    }

    // Dedup + null-skip: A → {B, B, null} unlocks B exactly once.
    {
        UObjectiveDefinition *Parent = NewObject<UObjectiveDefinition>();
        UObjectiveDefinition *Step = NewObject<UObjectiveDefinition>();
        Parent->NextObjectives = {Step, Step, nullptr};
        TArray<FObjectiveProgress> Tracked;
        FObjectiveProgress PP;
        PP.Definition = Parent;
        PP.bCompleted = true;
        Tracked.Add(PP);

        TArray<UObjectiveDefinition *> Out;
        UObjectiveTracker::CollectAssignableNextObjectives(Parent->NextObjectives, Tracked, Out);
        TestEqual(TEXT("duplicate + null entries collapse to one assignment"), Out.Num(), 1);
        TestTrue(TEXT("the step is present"), Out.Contains(Step));
    }

    return true;
}


#include "Misc/AutomationTest.h"
#include "Objectives/ObjectiveDefinition.h"
#include "Objectives/ObjectiveTracker.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicObjectivePrereqTest,
    "Mythic.Objectives.Prerequisites",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicObjectivePrereqTest::RunTest(const FString &Parameters) {
    UObjectiveDefinition *StepA = NewObject<UObjectiveDefinition>();
    UObjectiveDefinition *StepB = NewObject<UObjectiveDefinition>();
    StepB->PrerequisiteObjectives.Add(StepA);

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

        StepB->PrerequisiteObjectives.Add(nullptr);
        TestTrue(TEXT("a null prerequisite entry is ignored"),
                 UObjectiveTracker::AreObjectivePrerequisitesMet(StepB->PrerequisiteObjectives, Tracked));
    }

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

    {
        TArray<FObjectiveProgress> Empty;
        FObjectiveProgress Out;
        TestEqual(TEXT("a prereq-free objective assigns immediately"),
                  UObjectiveTracker::ResolveObjectiveOfferResult(Empty, StepA, Out),
                  EObjectiveOfferResult::Assigned);
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicObjectiveChainAdvanceTest,
    "Mythic.Objectives.ChainAdvance",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicObjectiveChainAdvanceTest::RunTest(const FString &Parameters) {
    UObjectiveDefinition *A = NewObject<UObjectiveDefinition>();
    UObjectiveDefinition *B = NewObject<UObjectiveDefinition>();
    UObjectiveDefinition *C = NewObject<UObjectiveDefinition>();
    A->NextObjectives = {B, C};
    C->PrerequisiteObjectives = {B};

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

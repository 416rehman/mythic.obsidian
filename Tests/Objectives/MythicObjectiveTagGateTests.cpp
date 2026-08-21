
#include "Misc/AutomationTest.h"
#include "GameplayTagContainer.h"
#include "Objectives/ObjectiveDefinition.h"
#include "Objectives/ObjectiveTracker.h"
#include "GAS/MythicTags_GAS.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicObjectiveTagGateTest,
    "Mythic.Objectives.TagGate",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicObjectiveTagGateTest::RunTest(const FString &Parameters) {
    const FGameplayTag TagRequired = GAS_EVENT_REACHED_LOCATION;
    const FGameplayTag TagBlocking = GAS_EVENT_ITEM_ACQUIRED;

    FGameplayTagContainer OwnedNone;
    FGameplayTagContainer OwnedRequired;
    OwnedRequired.AddTag(TagRequired);
    FGameplayTagContainer OwnedBlocking;
    OwnedBlocking.AddTag(TagBlocking);

    {
        UObjectiveDefinition *Ungated = NewObject<UObjectiveDefinition>();
        TArray<FObjectiveProgress> Tracked;
        FObjectiveProgress Out;
        TestEqual(TEXT("ungated assigns with no owned tags"),
                  UObjectiveTracker::ResolveObjectiveOfferResult(Tracked, Ungated, Out, OwnedNone),
                  EObjectiveOfferResult::Assigned);
        TestEqual(TEXT("ungated assigns even with unrelated owned tags"),
                  UObjectiveTracker::ResolveObjectiveOfferResult(Tracked, Ungated, Out, OwnedBlocking),
                  EObjectiveOfferResult::Assigned);
        TestEqual(TEXT("ungated assigns via the default (no-tags) overload"),
                  UObjectiveTracker::ResolveObjectiveOfferResult(Tracked, Ungated, Out),
                  EObjectiveOfferResult::Assigned);
    }

    {
        UObjectiveDefinition *Gated = NewObject<UObjectiveDefinition>();
        Gated->Precondition.RequireAll.AddTag(TagRequired);
        TArray<FObjectiveProgress> Tracked;
        FObjectiveProgress Out;
        TestEqual(TEXT("RequireAll gate: not met without the tag → PreconditionNotMet"),
                  UObjectiveTracker::ResolveObjectiveOfferResult(Tracked, Gated, Out, OwnedNone),
                  EObjectiveOfferResult::PreconditionNotMet);
        TestEqual(TEXT("RequireAll gate: assigned once the tag is owned"),
                  UObjectiveTracker::ResolveObjectiveOfferResult(Tracked, Gated, Out, OwnedRequired),
                  EObjectiveOfferResult::Assigned);
    }

    {
        UObjectiveDefinition *Blocked = NewObject<UObjectiveDefinition>();
        Blocked->Precondition.BlockAny.AddTag(TagBlocking);
        TArray<FObjectiveProgress> Tracked;
        FObjectiveProgress Out;
        TestEqual(TEXT("BlockAny gate: refused while the blocking tag is owned"),
                  UObjectiveTracker::ResolveObjectiveOfferResult(Tracked, Blocked, Out, OwnedBlocking),
                  EObjectiveOfferResult::PreconditionNotMet);
        TestEqual(TEXT("BlockAny gate: assigned when the blocking tag is absent"),
                  UObjectiveTracker::ResolveObjectiveOfferResult(Tracked, Blocked, Out, OwnedNone),
                  EObjectiveOfferResult::Assigned);
    }

    {
        UObjectiveDefinition *StepA = NewObject<UObjectiveDefinition>();
        UObjectiveDefinition *StepB = NewObject<UObjectiveDefinition>();
        StepB->PrerequisiteObjectives.Add(StepA);
        StepB->Precondition.RequireAll.AddTag(TagRequired);

        FObjectiveProgress Out;

        {
            TArray<FObjectiveProgress> Tracked;
            TestEqual(TEXT("prereq unmet dominates (checked before precondition)"),
                      UObjectiveTracker::ResolveObjectiveOfferResult(Tracked, StepB, Out, OwnedRequired),
                      EObjectiveOfferResult::PrerequisitesNotMet);
        }

        {
            TArray<FObjectiveProgress> Tracked;
            FObjectiveProgress ProgA;
            ProgA.Definition = StepA;
            ProgA.bCompleted = true;
            Tracked.Add(ProgA);
            TestEqual(TEXT("prereq met, precondition unmet → PreconditionNotMet"),
                      UObjectiveTracker::ResolveObjectiveOfferResult(Tracked, StepB, Out, OwnedNone),
                      EObjectiveOfferResult::PreconditionNotMet);

            TestEqual(TEXT("prereq met AND precondition met → Assigned"),
                      UObjectiveTracker::ResolveObjectiveOfferResult(Tracked, StepB, Out, OwnedRequired),
                      EObjectiveOfferResult::Assigned);
        }
    }

    {
        UObjectiveDefinition *Parent = NewObject<UObjectiveDefinition>();
        UObjectiveDefinition *GatedNext = NewObject<UObjectiveDefinition>();
        GatedNext->Precondition.RequireAll.AddTag(TagRequired);
        Parent->NextObjectives = {GatedNext};

        TArray<FObjectiveProgress> Tracked;
        FObjectiveProgress PP;
        PP.Definition = Parent;
        PP.bCompleted = true;
        Tracked.Add(PP);

        TArray<UObjectiveDefinition *> Out;
        UObjectiveTracker::CollectAssignableNextObjectives(Parent->NextObjectives, Tracked, Out, OwnedNone);
        TestEqual(TEXT("gated successor deferred while precondition unmet"), Out.Num(), 0);

        Out.Reset();
        UObjectiveTracker::CollectAssignableNextObjectives(Parent->NextObjectives, Tracked, Out, OwnedRequired);
        TestTrue(TEXT("gated successor unlocks once the precondition is met"), Out.Contains(GatedNext));
    }

    return true;
}

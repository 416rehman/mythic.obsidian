
#include "Misc/AutomationTest.h"
#include "GameplayTagContainer.h"
#include "Objectives/ObjectiveDefinition.h"
#include "Objectives/ObjectiveTracker.h"
#include "GAS/MythicTags_GAS.h"
#include "World/MythicTags_World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicObjectiveBranchTest,
    "Mythic.Objectives.Branch",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicObjectiveBranchTest::RunTest(const FString &Parameters) {
    const FGameplayTag KillTag = GAS_EVENT_KILL;
    const FGameplayTag SpareTag = GAS_EVENT_TALKED_TO_NPC;
    const FGameplayTag UnrelatedTag = GAS_EVENT_ITEM_ACQUIRED;
    const FGameplayTag KillFlag = WORLD_RESOURCE_TREE;
    const FGameplayTag SpareFlag = WORLD_RESOURCE_STONE;

    const FGameplayTagContainer NoOwned;
    const TArray<FObjectiveProgress> NoTracked;

    UObjectiveDefinition *KillNext = NewObject<UObjectiveDefinition>();
    UObjectiveDefinition *SpareNext = NewObject<UObjectiveDefinition>();
    UObjectiveDefinition *Confront = NewObject<UObjectiveDefinition>();
    {
        FMythicObjectiveBranch KillBranch;
        KillBranch.OutcomeTag = KillTag;
        KillBranch.NextObjectives = {KillNext};
        KillBranch.GrantStoryTags.AddTag(KillFlag);
        KillBranch.CancelSiblings = {SpareNext};

        FMythicObjectiveBranch SpareBranch;
        SpareBranch.OutcomeTag = SpareTag;
        SpareBranch.NextObjectives = {SpareNext};
        SpareBranch.GrantStoryTags.AddTag(SpareFlag);
        SpareBranch.CancelSiblings = {KillNext};

        Confront->OutcomeBranches = {KillBranch, SpareBranch};
    }

    {
        const FMythicObjectiveBranchResult R =
            UObjectiveTracker::SelectBranchForOutcome(Confront->OutcomeBranches, KillTag, NoTracked, NoOwned);
        TestTrue(TEXT("Killed: a branch matched"), R.bMatched);
        TestEqual(TEXT("Killed: exactly one successor"), R.Assignable.Num(), 1);
        TestTrue(TEXT("Killed: successor is the kill step"), R.Assignable.Contains(KillNext));
        TestTrue(TEXT("Killed: grants KillFlag"), R.GrantStoryTags.HasTag(KillFlag));
        TestFalse(TEXT("Killed: does NOT grant SpareFlag"), R.GrantStoryTags.HasTag(SpareFlag));
        TestTrue(TEXT("Killed: cancels the spare path"), R.CancelSiblings.Contains(SpareNext));
        TestFalse(TEXT("Killed: does not cancel its own successor"), R.CancelSiblings.Contains(KillNext));
    }

    {
        const FMythicObjectiveBranchResult R =
            UObjectiveTracker::SelectBranchForOutcome(Confront->OutcomeBranches, SpareTag, NoTracked, NoOwned);
        TestTrue(TEXT("Spared: a branch matched"), R.bMatched);
        TestEqual(TEXT("Spared: exactly one successor"), R.Assignable.Num(), 1);
        TestTrue(TEXT("Spared: successor is the spare step"), R.Assignable.Contains(SpareNext));
        TestTrue(TEXT("Spared: grants SpareFlag"), R.GrantStoryTags.HasTag(SpareFlag));
        TestFalse(TEXT("Spared: does NOT grant KillFlag"), R.GrantStoryTags.HasTag(KillFlag));
        TestTrue(TEXT("Spared: cancels the kill path"), R.CancelSiblings.Contains(KillNext));
    }

    {
        const FMythicObjectiveBranchResult R =
            UObjectiveTracker::SelectBranchForOutcome(Confront->OutcomeBranches, UnrelatedTag, NoTracked, NoOwned);
        TestFalse(TEXT("no-match: bMatched false"), R.bMatched);
        TestEqual(TEXT("no-match: no successors"), R.Assignable.Num(), 0);
        TestEqual(TEXT("no-match: no siblings to cancel"), R.CancelSiblings.Num(), 0);
        TestTrue(TEXT("no-match: no story tags"), R.GrantStoryTags.IsEmpty());

        const FMythicObjectiveBranchResult RInvalid =
            UObjectiveTracker::SelectBranchForOutcome(Confront->OutcomeBranches, FGameplayTag(), NoTracked, NoOwned);
        TestFalse(TEXT("invalid outcome: bMatched false"), RInvalid.bMatched);
    }

    {
        UObjectiveDefinition *FirstStep = NewObject<UObjectiveDefinition>();
        UObjectiveDefinition *Converge = NewObject<UObjectiveDefinition>();
        Converge->PrerequisiteObjectives = {FirstStep};

        UObjectiveDefinition *Branching = NewObject<UObjectiveDefinition>();
        FMythicObjectiveBranch B;
        B.OutcomeTag = KillTag;
        B.NextObjectives = {FirstStep, Converge};
        Branching->OutcomeBranches = {B};

        const FMythicObjectiveBranchResult R =
            UObjectiveTracker::SelectBranchForOutcome(Branching->OutcomeBranches, KillTag, NoTracked, NoOwned);
        TestTrue(TEXT("converging: matched"), R.bMatched);
        TestTrue(TEXT("converging: FirstStep assignable"), R.Assignable.Contains(FirstStep));
        TestFalse(TEXT("converging: Converge deferred (prereq unmet)"), R.Assignable.Contains(Converge));
        TestEqual(TEXT("converging: only one unlocks first"), R.Assignable.Num(), 1);
    }

    {
        TestTrue(TEXT("derive: Kill event → KillTag"),
                 UObjectiveTracker::DeriveAchievedOutcome(Confront, KillTag, FGameplayTagContainer()) == KillTag);
        TestTrue(TEXT("derive: Talk event → SpareTag"),
                 UObjectiveTracker::DeriveAchievedOutcome(Confront, SpareTag, FGameplayTagContainer()) == SpareTag);
        TestFalse(TEXT("derive: unrelated event → invalid (default route)"),
                  UObjectiveTracker::DeriveAchievedOutcome(Confront, UnrelatedTag, FGameplayTagContainer()).IsValid());

        UObjectiveDefinition *PayloadRouted = NewObject<UObjectiveDefinition>();
        FMythicObjectiveBranch PB;
        PB.OutcomeTag = KillFlag;
        PayloadRouted->OutcomeBranches = {PB};
        FGameplayTagContainer Payload;
        Payload.AddTag(KillFlag);
        TestTrue(TEXT("derive: payload carries the outcome tag → routes it"),
                 UObjectiveTracker::DeriveAchievedOutcome(PayloadRouted, UnrelatedTag, Payload) == KillFlag);
        TestFalse(TEXT("derive: payload without the outcome tag → invalid"),
                  UObjectiveTracker::DeriveAchievedOutcome(PayloadRouted, UnrelatedTag, FGameplayTagContainer()).IsValid());

        TestFalse(TEXT("derive: null def → invalid"),
                  UObjectiveTracker::DeriveAchievedOutcome(nullptr, KillTag, FGameplayTagContainer()).IsValid());
    }

    {
        TestEqual(TEXT("classify: Kill → Killed"), UObjectiveTracker::ClassifyOutcome(KillTag),
                  EMythicObjectiveOutcome::Killed);
        TestEqual(TEXT("classify: Talk → Spared"), UObjectiveTracker::ClassifyOutcome(SpareTag),
                  EMythicObjectiveOutcome::Spared);
        TestEqual(TEXT("classify: other → Completed"), UObjectiveTracker::ClassifyOutcome(UnrelatedTag),
                  EMythicObjectiveOutcome::Completed);
    }

    return true;
}

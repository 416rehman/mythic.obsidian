#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "World/Harvesting/MythicHarvestRewardOutboxSubsystem.h"
#include "World/Harvesting/MythicHarvestRewardPlanner.h"
#include "World/Harvesting/MythicHarvestableDefinition.h"
#include "World/Harvesting/MythicHarvestTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHarvestPostCommitBarrierTest,
    "Mythic.Harvesting.Transaction.PostCommitBarrier",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestPostCommitBarrierTest::RunTest(
    const FString &Parameters) {
    FMythicHarvestPostCommitBarrier Barrier;
    int32 DispatchCount = 0;

    if (Barrier.TryBeginSideEffects()) {
        ++DispatchCount;
    }
    TestFalse(TEXT("preparation cannot dispatch before state commit"),
              Barrier.HaveSideEffectsStarted());
    TestEqual(TEXT("no pre-commit side effect ran"), DispatchCount, 0);

    Barrier.MarkStateCommitted();
    TestTrue(TEXT("state commit opens the post-commit boundary"),
             Barrier.IsStateCommitted());
    if (Barrier.TryBeginSideEffects()) {
        ++DispatchCount;
    }
    if (Barrier.TryBeginSideEffects()) {
        ++DispatchCount;
    }
    TestEqual(TEXT("recursive or replayed dispatch runs exactly once"),
              DispatchCount, 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHarvestPreparedCompletionAtomicityTest,
    "Mythic.Harvesting.Transaction.PreparedCompletionAtomicity",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestPreparedCompletionAtomicityTest::RunTest(
    const FString &Parameters) {
    if (!TestNotNull(TEXT("engine is available"), GEngine)) {
        return false;
    }

    UGameInstance *GameInstance = NewObject<UGameInstance>(GEngine);
    GameInstance->InitializeStandalone();
    UWorld *World = GameInstance->GetWorld();
    if (!TestNotNull(TEXT("standalone authority world exists"), World)) {
        GameInstance->Shutdown();
        return false;
    }

    UMythicHarvestRewardOutboxSubsystem *Outbox =
        World->GetSubsystem<UMythicHarvestRewardOutboxSubsystem>();
    if (!TestNotNull(TEXT("authority reward outbox exists"), Outbox)) {
        GameInstance->Shutdown();
        return false;
    }

    UMythicHarvestableDefinition *Definition =
        NewObject<UMythicHarvestableDefinition>(
            GetTransientPackage(), TEXT("AtomicHarvestDefinition"));
    UItemDefinition *Material = NewObject<UItemDefinition>(
        GetTransientPackage(), TEXT("AtomicHarvestMaterial"));
    FMythicHarvestRewardEntry &Reward =
        Definition->PrimaryMaterials.AddDefaulted_GetRef();
    Reward.ItemDefinition = Material;
    Reward.MinQuantity = 3;
    Reward.MaxQuantity = 3;
    Definition->CompletionProficiencyXP = 0.0f;
    Definition->QuestCredit.bEmitCompletionCredit = false;

    FMythicHarvestParticipantSnapshot Participant;
    Participant.ContributorKey = TEXT("character-atomic");
    Participant.ContributionQuanta = 100;
    Participant.ItemLevel = 7;
    Participant.QuantityMultiplierQuanta =
        FMythicHarvestRewardPlanner::QuantityMultiplierScale;
    const TArray<FMythicHarvestParticipantSnapshot> Participants = {
        Participant};

    const FGuid WorldEpoch(1, 2, 3, 4);
    const FMythicHarvestNodeId NodeId(FGuid(5, 6, 7, 8));
    constexpr uint32 Generation = 1;
    FMythicPreparedHarvestCompletion Prepared;
    const FMythicHarvestRewardPrepareResult PrepareResult =
        Outbox->PrepareCompletion(*Definition, WorldEpoch, NodeId,
                                  Generation, Participants, Prepared);

    TestTrue(TEXT("deterministic completion prepares successfully"),
             PrepareResult.WasPrepared());
    TestTrue(TEXT("prepared completion is structurally valid"),
             Prepared.IsValid());
    TestFalse(TEXT("prepare does not commit an idempotency key"),
              Outbox->HasKnownCompletion(WorldEpoch, NodeId, Generation));
    TestEqual(TEXT("prepare does not queue or deliver any side effect"),
              Outbox->GetPendingWorkCount(), 0);

    const FMythicPreparedHarvestCompletion Replay = Prepared;
    TestEqual(TEXT("post-state commit publishes the prepared outbox once"),
              Outbox->CommitPreparedCompletion(MoveTemp(Prepared)),
              EMythicHarvestCompletionAdmission::Committed);
    TestTrue(TEXT("committed completion key is durable"),
             Outbox->HasKnownCompletion(WorldEpoch, NodeId, Generation));
    const int32 PendingAfterFirstCommit = Outbox->GetPendingWorkCount();
    TestTrue(TEXT("commit queues frozen delivery without attempting it"),
             PendingAfterFirstCommit > 0);

    FMythicPreparedHarvestCompletion ReentrantReplay = Replay;
    TestEqual(TEXT("recursive exact completion is recognized as replay"),
              Outbox->CommitPreparedCompletion(MoveTemp(ReentrantReplay)),
              EMythicHarvestCompletionAdmission::AlreadyKnown);
    TestEqual(TEXT("replay cannot duplicate pending delivery"),
              Outbox->GetPendingWorkCount(), PendingAfterFirstCommit);

    FMythicPreparedHarvestCompletion AlreadyKnownPlan;
    const FMythicHarvestRewardPrepareResult AlreadyKnownResult =
        Outbox->PrepareCompletion(*Definition, WorldEpoch, NodeId,
                                  Generation, Participants,
                                  AlreadyKnownPlan);
    TestEqual(TEXT("prepare reports an already committed lifecycle exactly"),
              AlreadyKnownResult.Status,
              EMythicHarvestRewardPrepareStatus::AlreadyKnown);
    TestFalse(TEXT("an already-known lifecycle is not a new prepared plan"),
              AlreadyKnownResult.WasPrepared());
    TestFalse(TEXT("an already-known lifecycle cannot enter a new commit"),
              AlreadyKnownPlan.IsValid());

    GameInstance->Shutdown();
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

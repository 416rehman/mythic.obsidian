#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "System/MythicAssetManager.h"
#include "World/Harvesting/MythicHarvestReceiptLedgerComponent.h"
#include "World/Harvesting/MythicHarvestRewardEscrowTypes.h"
#include "World/Harvesting/MythicHarvestRewardOutboxSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHarvestCumulativeReceiptTest,
    "Mythic.Harvesting.Receipts.CumulativeApplyAndCompaction",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestCumulativeReceiptTest::RunTest(
    const FString &Parameters) {
    UMythicHarvestReceiptLedgerComponent *Ledger =
        NewObject<UMythicHarvestReceiptLedgerComponent>();
    const FGuid Epoch(1, 2, 3, 4);
    const FMythicHarvestNodeId Node(FGuid(5, 6, 7, 8));
    const FPrimaryAssetId ItemId(
        UMythicAssetManager::ItemDefinitionType, FName(TEXT("TestOre")));
    const FMythicHarvestReceiptKey Key =
        FMythicHarvestReceiptKey::MakeCompletion(
            Epoch, Node, 1,
            EMythicHarvestReceiptChannel::PrimaryMaterial, 2);
    const FGuid Fingerprint = FMythicHarvestReceiptFingerprint::Build(
        Key, ItemId, 7, 1234, 5, 0);

    FMythicHarvestReceiptApplyPlan Plan;
    TestEqual(TEXT("first receipt reserves full target"),
              Ledger->TryPlanApply(Key, Fingerprint, 7, 1, Plan),
              EMythicHarvestReceiptPlanStatus::Ready);
    TestTrue(TEXT("partial item insertion commits cumulatively"),
             Ledger->CommitPlannedApply(Plan, 3));
    TestEqual(TEXT("three units are now live"),
              Ledger->GetAppliedQuantity(Key), int64(3));

    TestEqual(TEXT("retry plans only the semantic remainder"),
              Ledger->TryPlanApply(Key, Fingerprint, 7, 1, Plan),
              EMythicHarvestReceiptPlanStatus::Ready);
    TestEqual(TEXT("four units remain"), Plan.RemainingQuantity,
              int64(4));
    TestTrue(TEXT("remainder closes the live receipt"),
             Ledger->CommitPlannedApply(Plan, 4));
    TestEqual(TEXT("full target is live"),
              Ledger->GetAppliedQuantity(Key), int64(7));

    FMythicHarvestReceiptLedgerSaveV1 DurableSnapshot;
    FName Diagnostic;
    TestTrue(TEXT("exact character snapshot captures receipt"),
             Ledger->BuildSaveSnapshot(DurableSnapshot, Diagnostic));
    TestTrue(TEXT("successful callback marks exact snapshot durable"),
             Ledger->MarkSnapshotDurable(DurableSnapshot, Diagnostic));
    TestEqual(TEXT("durable quantity advances only from callback"),
              Ledger->GetDurableAppliedQuantity(Key), int64(7));
    TestEqual(TEXT("a replay is terminally idempotent"),
              Ledger->TryPlanApply(Key, Fingerprint, 7, 1, Plan),
              EMythicHarvestReceiptPlanStatus::AlreadyApplied);

    const FGuid DriftedFingerprint = FMythicHarvestReceiptFingerprint::Build(
        Key, ItemId, 8, 1234, 5, 0);
    TestEqual(TEXT("payload drift fails closed"),
              Ledger->TryPlanApply(Key, DriftedFingerprint, 8, 1, Plan),
              EMythicHarvestReceiptPlanStatus::Conflict);

    int32 Removed = 0;
    const TSet<FMythicHarvestReceiptKey> EmptyDurableOutbox;
    const TMap<FMythicHarvestNodeId, uint32> EmptyCompletedGenerations;
    TestTrue(TEXT("later durable world omission permits compaction"),
             Ledger->CompactCompletedRows(
                 Epoch, 2, EmptyDurableOutbox,
                 EmptyCompletedGenerations, Removed, Diagnostic));
    TestEqual(TEXT("one completed receipt was compacted"), Removed, 1);
    TestEqual(TEXT("compaction leaves a bounded ledger"),
              Ledger->GetReceiptRowCount(), 0);
    TestFalse(TEXT("older world snapshots are rejected after compaction"),
              Ledger->ValidateWorldSnapshotMinimum(Epoch, 1, Diagnostic));
    TestTrue(TEXT("the exact omission boundary remains acceptable"),
             Ledger->ValidateWorldSnapshotMinimum(Epoch, 2, Diagnostic));

    FMythicHarvestReceiptLedgerSaveV1 CompactedSnapshot;
    TestTrue(TEXT("compacted character snapshot captures rollback watermark"),
             Ledger->BuildSaveSnapshot(CompactedSnapshot, Diagnostic));
    TestFalse(TEXT("character load preflight rejects an older active world"),
              Ledger->ValidateLoadSnapshotAgainstWorld(
                  CompactedSnapshot, Epoch, 1, Diagnostic));
    TestEqual(TEXT("load rollback has stable diagnostic"), Diagnostic,
              FName(TEXT("WorldSnapshotOlderThanHarvestReceiptWatermark")));
    TestTrue(TEXT("character load preflight accepts the durable boundary"),
             Ledger->ValidateLoadSnapshotAgainstWorld(
                 CompactedSnapshot, Epoch, 2, Diagnostic));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHarvestReceiptReservationFailureTest,
    "Mythic.Harvesting.Receipts.CommitFailureReleasesReservation",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestReceiptReservationFailureTest::RunTest(
    const FString &Parameters) {
    UMythicHarvestReceiptLedgerComponent *Ledger =
        NewObject<UMythicHarvestReceiptLedgerComponent>();
    const FGuid Epoch(101, 102, 103, 104);
    const FMythicHarvestReceiptKey Key =
        FMythicHarvestReceiptKey::MakeCompletion(
            Epoch, FMythicHarvestNodeId(FGuid(111, 112, 113, 114)),
            1, EMythicHarvestReceiptChannel::PrimaryMaterial, 0);
    const FPrimaryAssetId ItemId(
        UMythicAssetManager::ItemDefinitionType,
        FName(TEXT("ReservationTestItem")));
    const FGuid Fingerprint = FMythicHarvestReceiptFingerprint::Build(
        Key, ItemId, 4, 77, 1, 0);

    FMythicHarvestReceiptApplyPlan Plan;
    TestEqual(TEXT("valid contract reserves"),
              Ledger->TryPlanApply(Key, Fingerprint, 4, 1, Plan),
              EMythicHarvestReceiptPlanStatus::Ready);
    FMythicHarvestReceiptApplyPlan DriftedPlan = Plan;
    DriftedPlan.PayloadFingerprint = FGuid(1, 1, 1, 1);
    TestFalse(TEXT("mutated opaque plan fails commit"),
              Ledger->CommitPlannedApply(DriftedPlan, 4));
    TestFalse(TEXT("failed validation releases the exact reservation"),
              Ledger->HasOpenMutation());
    TestEqual(TEXT("the entitlement can be safely replanned"),
              Ledger->TryPlanApply(Key, Fingerprint, 4, 1, Plan),
              EMythicHarvestReceiptPlanStatus::Ready);
    FMythicHarvestReceiptApplyPlan WrongKeyPlan = Plan;
    WrongKeyPlan.Key.Generation = 2;
    TestFalse(TEXT("mutated receipt key fails commit"),
              Ledger->CommitPlannedApply(WrongKeyPlan, 4));
    TestFalse(TEXT("reservation token releases the original key"),
              Ledger->HasOpenMutation());
    TestEqual(TEXT("key mutation cannot strand the entitlement"),
              Ledger->TryPlanApply(Key, Fingerprint, 4, 1, Plan),
              EMythicHarvestReceiptPlanStatus::Ready);
    TestFalse(TEXT("out-of-range delta fails commit"),
              Ledger->CommitPlannedApply(Plan, 5));
    TestFalse(TEXT("range failure cannot strand save capture"),
              Ledger->HasOpenMutation());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHarvestCumulativeWorkReceiptTest,
    "Mythic.Harvesting.Receipts.CumulativeWorkRollbackDedupe",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestCumulativeWorkReceiptTest::RunTest(
    const FString &Parameters) {
    FMythicHarvestWorkRewardContract DisabledContract;
    TestTrue(TEXT("new contributor has no frozen work reward contract"),
             DisabledContract.IsUnset());
    DisabledContract.bInitialized = true;
    TestTrue(TEXT("zero-rate first hit freezes a valid disabled contract"),
             DisabledContract.IsValid());
    TestFalse(TEXT("disabled frozen contract cannot start rewarding after a balance edit"),
              DisabledContract.IsEnabled());

    UMythicHarvestReceiptLedgerComponent *Ledger =
        NewObject<UMythicHarvestReceiptLedgerComponent>();
    const FGuid Epoch(201, 202, 203, 204);
    const FMythicHarvestNodeId Node(FGuid(211, 212, 213, 214));
    const FString Contributor(TEXT("character-cumulative"));
    const FMythicHarvestReceiptKey Key =
        FMythicHarvestReceiptKey::MakeAppliedWork(
            Epoch, Node, 3, Contributor);
    const FMythicHarvestReceiptKey ReplayKey =
        FMythicHarvestReceiptKey::MakeAppliedWork(
            Epoch, Node, 3, Contributor);
    const FMythicHarvestReceiptKey OtherContributorKey =
        FMythicHarvestReceiptKey::MakeAppliedWork(
            Epoch, Node, 3, TEXT("character-other"));
    TestTrue(TEXT("same logical series has deterministic identity"),
             Key == ReplayKey);
    TestFalse(TEXT("contributors cannot alias work series"),
              Key == OtherContributorKey);

    const FPrimaryAssetId ProficiencyId(
        UMythicAssetManager::ProficiencyDefinitionType,
        FName(TEXT("Mining")));
    const int64 Rate =
        2 * FMythicHarvestReceiptQuantity::QuantaPerUnit;
    const FGuid Fingerprint =
        FMythicHarvestReceiptFingerprint::BuildAppliedWorkSeries(
            Key, ProficiencyId, Rate);
    int64 FirstTarget = 0;
    int64 ExtendedTarget = 0;
    TestTrue(TEXT("five work units produce a cumulative target"),
             FMythicHarvestReceiptQuantity::
                 TryCalculateCumulativeAppliedWorkXP(
                     5 * FMythicHarvestWork::QuantaPerWorkUnit,
                     Rate, FirstTarget));
    TestTrue(TEXT("eight work units produce an extended target"),
             FMythicHarvestReceiptQuantity::
                 TryCalculateCumulativeAppliedWorkXP(
                     8 * FMythicHarvestWork::QuantaPerWorkUnit,
                     Rate, ExtendedTarget));

    FMythicHarvestReceiptApplyPlan Plan;
    TestEqual(TEXT("first cumulative interval reserves"),
              Ledger->TryPlanApply(
                  Key, Fingerprint, FirstTarget, 1, Plan),
              EMythicHarvestReceiptPlanStatus::Ready);
    TestTrue(TEXT("first cumulative interval commits"),
             Ledger->CommitPlannedApply(Plan, Plan.RemainingQuantity));
    TestEqual(TEXT("pre-hit world replay is already covered"),
              Ledger->TryPlanApply(
                  ReplayKey, Fingerprint, FirstTarget, 1, Plan),
              EMythicHarvestReceiptPlanStatus::AlreadyApplied);
    TestEqual(TEXT("larger cumulative interval extends one series"),
              Ledger->TryPlanApply(
                  Key, Fingerprint, ExtendedTarget, 1, Plan),
              EMythicHarvestReceiptPlanStatus::Ready);
    TestEqual(TEXT("only newly uncovered XP remains"),
              Plan.RemainingQuantity, ExtendedTarget - FirstTarget);
    TestTrue(TEXT("extension commits only the uncovered suffix"),
             Ledger->CommitPlannedApply(Plan, Plan.RemainingQuantity));
    TestEqual(TEXT("one receipt row covers every work interval"),
              Ledger->GetReceiptRowCount(), 1);
    TestEqual(TEXT("cumulative applied target reaches eight units"),
              Ledger->GetAppliedQuantity(Key), ExtendedTarget);
    int32 Removed = 0;
    FName Diagnostic;
    const TSet<FMythicHarvestReceiptKey> EmptyPendingKeys;
    TMap<FMythicHarvestNodeId, uint32> CompletedGenerations;
    TestTrue(TEXT("partial lifecycle compaction request is valid"),
             Ledger->CompactCompletedRows(
                 Epoch, 2, EmptyPendingKeys, CompletedGenerations,
                 Removed, Diagnostic));
    TestEqual(TEXT("active-generation cumulative series is retained"),
              Removed, 0);
    CompletedGenerations.Add(Node, 3);
    TestTrue(TEXT("durably completed lifecycle permits series compaction"),
             Ledger->CompactCompletedRows(
                 Epoch, 2, EmptyPendingKeys, CompletedGenerations,
                 Removed, Diagnostic));
    TestEqual(TEXT("completed cumulative series compacts once"),
              Removed, 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHarvestGenerationCheckpointValidationTest,
    "Mythic.Harvesting.Receipts.GenerationCheckpointValidation",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestGenerationCheckpointValidationTest::RunTest(
    const FString &Parameters) {
    FMythicHarvestRewardOutboxSaveV1 Snapshot;
    Snapshot.WorldEpoch = FGuid(11, 12, 13, 14);
    Snapshot.SnapshotSequence = 1;
    FMythicSavedHarvestRewardCompletionV1 &Known =
        Snapshot.KnownCompletions.AddDefaulted_GetRef();
    Known.WorldEpoch = Snapshot.WorldEpoch;
    Known.NodeGuid = FGuid(21, 22, 23, 24);
    Known.Generation = 4;
    FMythicSavedHarvestGenerationHighWaterV1 &HighWater =
        Snapshot.GenerationHighWatermarks.AddDefaulted_GetRef();
    HighWater.WorldEpoch = Known.WorldEpoch;
    HighWater.NodeGuid = Known.NodeGuid;
    HighWater.HighestKnownGeneration = Known.Generation;

    FName Diagnostic;
    TestTrue(TEXT("exact known completion backs its O(1) checkpoint"),
             UMythicHarvestRewardOutboxSubsystem::ValidateSaveSnapshot(
                 Snapshot, Diagnostic));
    HighWater.HighestKnownGeneration = 5;
    TestFalse(TEXT("an unbacked generation checkpoint fails closed"),
              UMythicHarvestRewardOutboxSubsystem::ValidateSaveSnapshot(
                  Snapshot, Diagnostic));
    TestEqual(TEXT("checkpoint mismatch has stable diagnostic"),
              Diagnostic,
              FName(TEXT("HarvestGenerationHighWaterMismatch")));

    HighWater.HighestKnownGeneration = Known.Generation;
    FMythicSavedHarvestRewardCompletionV1 Historical = Known;
    Historical.Generation = Known.Generation - 1;
    Snapshot.KnownCompletions.Add(Historical);
    TestFalse(TEXT("historical exact rows cannot grow behind high-water"),
              UMythicHarvestRewardOutboxSubsystem::ValidateSaveSnapshot(
                  Snapshot, Diagnostic));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHarvestRetryBudgetFairnessTest,
    "Mythic.Harvesting.Rewards.FiveQueueRetryFairness",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestRetryBudgetFairnessTest::RunTest(
    const FString &Parameters) {
    uint8 Cursor = 0;
    int32 CompletionAttempts = 0;
    int32 ItemAttempts = 0;
    int32 WorkAttempts = 0;
    int32 DurabilityAttempts = 0;
    int32 EscrowAttempts = 0;
    for (int32 Retry = 0; Retry < 10; ++Retry) {
        const FMythicHarvestRewardRetryBudgets Budgets =
            UMythicHarvestRewardOutboxSubsystem::CalculateRetryBudgets(
                1, true, true, true, true, true, Cursor);
        CompletionAttempts += Budgets.CompletionBudget;
        ItemAttempts += Budgets.ItemBudget;
        WorkAttempts += Budgets.WorkBudget;
        DurabilityAttempts += Budgets.DurabilityBudget;
        EscrowAttempts += Budgets.EscrowBudget;
        Cursor = Budgets.NextQueueCursor;
    }
    TestEqual(TEXT("completion receives its rotating share"),
              CompletionAttempts, 2);
    TestEqual(TEXT("item receives its rotating share"), ItemAttempts, 2);
    TestEqual(TEXT("work receives its rotating share"), WorkAttempts, 2);
    TestEqual(TEXT("durability cost receives its rotating share"),
              DurabilityAttempts, 2);
    TestEqual(TEXT("character item escrow receives its rotating share"),
              EscrowAttempts, 2);
    TestEqual(TEXT("ten single attempts return to completion cursor"),
              Cursor, uint8(0));

    const FMythicHarvestRewardRetryBudgets SkipsInactive =
        UMythicHarvestRewardOutboxSubsystem::CalculateRetryBudgets(
            1, true, false, true, false, false, 1);
    TestEqual(TEXT("inactive item queue is skipped to work"),
              SkipsInactive.WorkBudget, 1);
    TestEqual(TEXT("skip advances fairly past work"),
              SkipsInactive.NextQueueCursor, uint8(3));

    const FMythicHarvestRewardRetryBudgets WrapsPastEscrow =
        UMythicHarvestRewardOutboxSubsystem::CalculateRetryBudgets(
            1, true, false, true, false, false, 4);
    TestEqual(TEXT("an inactive escrow queue wraps back to completion"),
              WrapsPastEscrow.CompletionBudget, 1);
    TestEqual(TEXT("the wrap leaves the cursor past completion"),
              WrapsPastEscrow.NextQueueCursor, uint8(1));

    int32 RowCursor = 0;
    TArray<int32> VisitedRows;
    for (int32 Attempt = 0; Attempt < 6; ++Attempt) {
        VisitedRows.Add(RowCursor);
        RowCursor = UMythicHarvestRewardOutboxSubsystem::
            AdvanceRetryRowCursor(RowCursor, 3, false);
    }
    TestEqual(TEXT("blocked first row cannot starve row one"),
              VisitedRows[1], 1);
    TestEqual(TEXT("blocked first row cannot starve row two"),
              VisitedRows[2], 2);
    TestEqual(TEXT("queue-local cursor wraps canonically"), RowCursor, 0);
    TestEqual(TEXT("removal keeps the shifted successor at the cursor"),
              UMythicHarvestRewardOutboxSubsystem::
                  AdvanceRetryRowCursor(2, 2, true),
              0);

    const int32 ContributorLimit =
        FMythicHarvestRewardOutboxSaveV1::
            AbsoluteMaximumPendingDeliveriesPerContributor;
    TestFalse(TEXT("one contributor may reach its isolated quota"),
              UMythicHarvestRewardOutboxSubsystem::
                  WouldExceedPerContributorPendingCapacity(
                      ContributorLimit, 0));
    TestTrue(TEXT("one full inventory cannot consume another contributor's capacity"),
             UMythicHarvestRewardOutboxSubsystem::
                 WouldExceedPerContributorPendingCapacity(
                     ContributorLimit, 1));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHarvestDurabilityReceiptRollbackTest,
    "Mythic.Harvesting.Receipts.DurabilityCostCrashRollbackAndToolSwitch",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestDurabilityReceiptRollbackTest::RunTest(
    const FString &Parameters) {
    UMythicHarvestReceiptLedgerComponent *Ledger =
        NewObject<UMythicHarvestReceiptLedgerComponent>();
    const FGuid Epoch(301, 302, 303, 304);
    const FMythicHarvestNodeId Node(FGuid(311, 312, 313, 314));
    const FString Contributor(TEXT("character-durability"));
    const FGuid AxeGuid(321, 322, 323, 324);
    const FGuid PickGuid(331, 332, 333, 334);
    const FMythicHarvestReceiptKey AxeKey =
        FMythicHarvestReceiptKey::MakeDurabilityCost(
            Epoch, Node, 2, Contributor, AxeGuid);
    const FMythicHarvestReceiptKey AxeReplayKey =
        FMythicHarvestReceiptKey::MakeDurabilityCost(
            Epoch, Node, 2, Contributor, AxeGuid);
    const FMythicHarvestReceiptKey PickKey =
        FMythicHarvestReceiptKey::MakeDurabilityCost(
            Epoch, Node, 2, Contributor, PickGuid);
    TestTrue(TEXT("same tool/lifecycle derives one deterministic cost series"),
             AxeKey == AxeReplayKey);
    TestFalse(TEXT("switching physical tools cannot alias durability costs"),
              AxeKey == PickKey);

    const FGuid Fingerprint =
        FMythicHarvestReceiptFingerprint::BuildDurabilityCostSeries(
            AxeKey, AxeGuid);
    FMythicHarvestReceiptApplyPlan Plan;
    TestEqual(TEXT("first three wear points reserve"),
              Ledger->TryPlanApply(AxeKey, Fingerprint, 3, 1, Plan),
              EMythicHarvestReceiptPlanStatus::Ready);
    TestTrue(TEXT("first wear prefix commits with the item mutation"),
             Ledger->CommitPlannedApply(Plan, 3));

    FMythicHarvestReceiptLedgerSaveV1 CharacterAfterThree;
    FName Diagnostic;
    TestTrue(TEXT("character snapshot atomically captures wear receipt"),
             Ledger->BuildSaveSnapshot(CharacterAfterThree, Diagnostic));
    TestTrue(TEXT("exact character write makes the prefix durable"),
             Ledger->MarkSnapshotDurable(CharacterAfterThree, Diagnostic));
    TestEqual(TEXT("stale lower world target is already paid"),
              Ledger->TryPlanApply(
                  AxeReplayKey, Fingerprint, 2, 1, Plan),
              EMythicHarvestReceiptPlanStatus::AlreadyApplied);
    TestEqual(TEXT("newer world target exposes only its unpaid suffix"),
              Ledger->TryPlanApply(AxeKey, Fingerprint, 5, 2, Plan),
              EMythicHarvestReceiptPlanStatus::Ready);
    TestEqual(TEXT("crash recovery charges exactly two additional points"),
              Plan.RemainingQuantity, int64(2));
    TestTrue(TEXT("suffix receipt commits exactly once"),
             Ledger->CommitPlannedApply(Plan, 2));

    FMythicHarvestReceiptLedgerSaveV1 CharacterAfterFive;
    TestTrue(TEXT("newer character snapshot captures cumulative target"),
             Ledger->BuildSaveSnapshot(CharacterAfterFive, Diagnostic));

    FMythicHarvestRewardOutboxSaveV1 WorldAfterFive;
    WorldAfterFive.WorldEpoch = Epoch;
    WorldAfterFive.SnapshotSequence = 2;
    FMythicSavedHarvestRewardCompletionV1 &CompletedGeneration =
        WorldAfterFive.KnownCompletions.AddDefaulted_GetRef();
    CompletedGeneration.WorldEpoch = Epoch;
    CompletedGeneration.NodeGuid = Node.GetGuid();
    CompletedGeneration.Generation = 1;
    FMythicSavedHarvestGenerationHighWaterV1 &GenerationHighWater =
        WorldAfterFive.GenerationHighWatermarks.AddDefaulted_GetRef();
    GenerationHighWater.WorldEpoch = Epoch;
    GenerationHighWater.NodeGuid = Node.GetGuid();
    GenerationHighWater.HighestKnownGeneration = 1;
    FMythicSavedHarvestDurabilityCostV1 &Cost =
        WorldAfterFive.DurabilityCosts.AddDefaulted_GetRef();
    Cost.WorldEpoch = Epoch;
    Cost.NodeGuid = Node.GetGuid();
    Cost.Generation = 2;
    Cost.ContributorKey = Contributor;
    Cost.ToolItemInstanceGuid = AxeGuid;
    Cost.CumulativeWearTarget = 5;
    Cost.DurablyAppliedWearTarget = 3;
    Cost.ReceiptKey = AxeKey;
    Cost.ReceiptPayloadFingerprint = Fingerprint;
    FMythicSavedHarvestContributorLedgerFenceV1 &Fence =
        WorldAfterFive.ContributorLedgerFences.AddDefaulted_GetRef();
    Fence.ContributorKey = Contributor;
    Fence.LedgerEpoch = CharacterAfterThree.LedgerEpoch;
    Fence.MinimumLedgerRevision = CharacterAfterThree.LedgerRevision;
    TestTrue(TEXT("world persists unpaid suffix and exact character lineage fence"),
             UMythicHarvestRewardOutboxSubsystem::ValidateSaveSnapshot(
                 WorldAfterFive, Diagnostic));

    FMythicHarvestItemEscrowSaveV1 EmptyEscrow;
    EmptyEscrow.EscrowEpoch = FGuid(351, 352, 353, 354);
    EmptyEscrow.EscrowRevision = 1;

    FMythicHarvestReceiptLedgerSaveV1 CharacterBeforeWear;
    CharacterBeforeWear.LedgerEpoch = CharacterAfterThree.LedgerEpoch;
    TestFalse(TEXT("newer world rejects character rollback before any wear"),
              UMythicHarvestRewardOutboxSubsystem::
                  ValidateCharacterReceiptSnapshotAgainstWorld(
                      WorldAfterFive, Contributor, CharacterBeforeWear,
                      EmptyEscrow, Diagnostic));
    TestEqual(TEXT("rollback rejection has stable diagnostic"), Diagnostic,
              FName(TEXT("HarvestCharacterSnapshotOlderThanWorld")));
    TestTrue(TEXT("newer character remains compatible with an older world"),
             UMythicHarvestRewardOutboxSubsystem::
                  ValidateCharacterReceiptSnapshotAgainstWorld(
                      WorldAfterFive, Contributor, CharacterAfterFive,
                      EmptyEscrow, Diagnostic));
    FMythicHarvestReceiptLedgerSaveV1 ReplacedCharacter =
        CharacterAfterFive;
    ReplacedCharacter.LedgerEpoch = FGuid(341, 342, 343, 344);
    TestFalse(TEXT("a replaced character ledger cannot alias the fenced revision"),
              UMythicHarvestRewardOutboxSubsystem::
                  ValidateCharacterReceiptSnapshotAgainstWorld(
                      WorldAfterFive, Contributor, ReplacedCharacter,
                      EmptyEscrow, Diagnostic));
    TestEqual(TEXT("lineage rejection has stable diagnostic"), Diagnostic,
              FName(TEXT("HarvestCharacterLedgerLineageMismatch")));

    FMythicHarvestItemEscrowSaveV1 UnrevisedEscrow = EmptyEscrow;
    UnrevisedEscrow.EscrowRevision = 0;
    TestFalse(TEXT("an unrevised escrow rejects an otherwise acceptable character"),
              UMythicHarvestRewardOutboxSubsystem::
                  ValidateCharacterReceiptSnapshotAgainstWorld(
                      WorldAfterFive, Contributor, CharacterAfterFive,
                      UnrevisedEscrow, Diagnostic));
    TestEqual(TEXT("escrow header rejection has stable diagnostic"), Diagnostic,
              FName(TEXT("InvalidHarvestItemEscrowHeader")));

    FMythicHarvestItemEscrowSaveV1 UnbackedEscrow = EmptyEscrow;
    FMythicSavedHarvestItemEscrowRowV1 &EscrowRow =
        UnbackedEscrow.Rows.AddDefaulted_GetRef();
    EscrowRow.ReceiptKey = FMythicHarvestReceiptKey::MakeCompletion(
        Epoch, Node, 2, EMythicHarvestReceiptChannel::PrimaryMaterial, 0);
    EscrowRow.ItemDefinitionId = FPrimaryAssetId(
        UMythicAssetManager::ItemDefinitionType,
        FName(TEXT("EscrowTestOre")));
    EscrowRow.OriginalQuantity = 2;
    EscrowRow.RemainingQuantity = 2;
    EscrowRow.ItemLevel = 1;
    EscrowRow.ItemSeed = 351352;
    EscrowRow.FirstObservedWorldSnapshotSequence = 2;
    EscrowRow.MutationRevision = UnbackedEscrow.EscrowRevision;
    EscrowRow.ReceiptPayloadFingerprint =
        FMythicHarvestReceiptFingerprint::Build(
            EscrowRow.ReceiptKey, EscrowRow.ItemDefinitionId,
            EscrowRow.OriginalQuantity, EscrowRow.ItemSeed,
            static_cast<uint32>(EscrowRow.ItemLevel),
            FMythicSavedHarvestItemEscrowRowV1::PackQualityAuxiliary(
                EscrowRow.bHasResolvedQuality, EscrowRow.ResolvedQuality));
    TestTrue(TEXT("the unbacked escrow row is itself well formed"),
             FMythicHarvestItemEscrowSaveV1::Validate(
                 UnbackedEscrow, Diagnostic));
    TestFalse(TEXT("escrowed items with no accepted receipt fail closed"),
              UMythicHarvestRewardOutboxSubsystem::
                  ValidateCharacterReceiptSnapshotAgainstWorld(
                      WorldAfterFive, Contributor, CharacterAfterFive,
                      UnbackedEscrow, Diagnostic));
    TestEqual(TEXT("escrow binding rejection has stable diagnostic"), Diagnostic,
              FName(TEXT("HarvestItemEscrowReceiptBindingMismatch")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHarvestSaveCallbackRestoreDomainTest,
    "Mythic.Harvesting.Receipts.SaveCallbackRestoreDomainRace",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestSaveCallbackRestoreDomainTest::RunTest(
    const FString &Parameters) {
    const FGuid OldToken(401, 402, 403, 404);
    const FGuid OldOperation(411, 412, 413, 414);
    const FGuid NewToken(421, 422, 423, 424);
    const FGuid NewOperation(431, 432, 433, 434);
    FMythicHarvestCharacterSaveRequestIdentity OldRequest;
    OldRequest.RequestToken = OldToken;
    OldRequest.OperationId = OldOperation;
    OldRequest.RestoreDomainEpoch = 7;
    FMythicHarvestCharacterSaveRequestIdentity NewRequest;
    NewRequest.RequestToken = NewToken;
    NewRequest.OperationId = NewOperation;
    NewRequest.RestoreDomainEpoch = 8;

    TestFalse(TEXT("old physical save cannot mutate after world restore"),
              FMythicHarvestCharacterSaveCallbackPolicy::
                  MatchesCurrentRequest(
                      OldRequest, OldToken, OldOperation, 7, 8));
    TestFalse(TEXT("queued old callback cannot clear a new-domain latch"),
              FMythicHarvestCharacterSaveCallbackPolicy::
                  MatchesCurrentRequest(
                      NewRequest, OldToken, OldOperation, 7, 8));
    TestFalse(TEXT("operation mismatch is rejected inside one domain"),
              FMythicHarvestCharacterSaveCallbackPolicy::
                  MatchesCurrentRequest(
                      NewRequest, NewToken, OldOperation, 8, 8));
    TestTrue(TEXT("only the exact queued new save may reconcile"),
             FMythicHarvestCharacterSaveCallbackPolicy::
                 MatchesCurrentRequest(
                     NewRequest, NewToken, NewOperation, 8, 8));
    TestTrue(TEXT("old successful disk write with no new mutation queues a corrective save"),
             FMythicHarvestCharacterSaveCallbackPolicy::
                 RequiresCorrectiveSave(true, 7, 8, false));
    TestFalse(TEXT("already queued new-domain save is the corrective write"),
              FMythicHarvestCharacterSaveCallbackPolicy::
                  RequiresCorrectiveSave(true, 7, 8, true));
    TestFalse(TEXT("failed old write cannot corrupt the restored disk lineage"),
              FMythicHarvestCharacterSaveCallbackPolicy::
                  RequiresCorrectiveSave(false, 7, 8, false));
    return true;
}

#endif

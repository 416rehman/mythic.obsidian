#include "World/Harvesting/MythicHarvestReceiptLedgerComponent.h"

#include "GameFramework/Actor.h"
#include "World/Harvesting/MythicHarvestSettings.h"

UMythicHarvestReceiptLedgerComponent::
UMythicHarvestReceiptLedgerComponent() {
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(false);
    LedgerEpoch = FGuid::NewGuid();
}

int32 UMythicHarvestReceiptLedgerComponent::GetConfiguredMaximumRows() const {
    const UMythicHarvestSettings *Settings =
        GetDefault<UMythicHarvestSettings>();
    return FMath::Clamp(
        Settings ? Settings->RewardReceiptMaximumRows
                 : FMythicHarvestReceiptLedgerSaveV1::AbsoluteMaximumRows,
        1, FMythicHarvestReceiptLedgerSaveV1::AbsoluteMaximumRows);
}

bool UMythicHarvestReceiptLedgerComponent::RowsHaveSameContract(
    const FMythicSavedHarvestReceiptRowV1 &Row,
    const FGuid &PayloadFingerprint, const int64 TargetQuantity) {
    const bool bCumulative = Row.Key.Channel
            == EMythicHarvestReceiptChannel::AppliedWorkProficiencyXP
        || Row.Key.Channel
            == EMythicHarvestReceiptChannel::DurabilityCost;
    return Row.PayloadFingerprint == PayloadFingerprint
        && (bCumulative
            || Row.TargetQuantity == TargetQuantity);
}

bool UMythicHarvestReceiptLedgerComponent::ApplyPlansMatch(
    const FMythicHarvestReceiptApplyPlan &Left,
    const FMythicHarvestReceiptApplyPlan &Right) {
    return Left.ReservationToken == Right.ReservationToken
        && Left.Status == Right.Status && Left.Key == Right.Key
        && Left.PayloadFingerprint == Right.PayloadFingerprint
        && Left.TargetQuantity == Right.TargetQuantity
        && Left.PreviouslyAppliedQuantity
            == Right.PreviouslyAppliedQuantity
        && Left.RemainingQuantity == Right.RemainingQuantity
        && Left.FirstObservedWorldSnapshotSequence
            == Right.FirstObservedWorldSnapshotSequence;
}

EMythicHarvestReceiptPlanStatus
UMythicHarvestReceiptLedgerComponent::TryPlanApply(
    const FMythicHarvestReceiptKey &Key,
    const FGuid &PayloadFingerprint, const int64 TargetQuantity,
    const uint64 FirstObservedWorldSnapshotSequence,
    FMythicHarvestReceiptApplyPlan &OutPlan) {
    OutPlan = FMythicHarvestReceiptApplyPlan();
    OutPlan.Key = Key;
    OutPlan.PayloadFingerprint = PayloadFingerprint;
    OutPlan.TargetQuantity = TargetQuantity;
    OutPlan.FirstObservedWorldSnapshotSequence =
        FirstObservedWorldSnapshotSequence;
    if (!Key.IsValid() || !PayloadFingerprint.IsValid()
        || TargetQuantity <= 0) {
        OutPlan.Status = EMythicHarvestReceiptPlanStatus::Invalid;
        return OutPlan.Status;
    }
    if (ReservedKeys.Contains(Key)) {
        OutPlan.Status = EMythicHarvestReceiptPlanStatus::Busy;
        return OutPlan.Status;
    }

    const FMythicSavedHarvestReceiptRowV1 *Existing =
        RowsByKey.Find(Key);
    if (Existing
        && !RowsHaveSameContract(*Existing, PayloadFingerprint,
                                 TargetQuantity)) {
        OutPlan.Status = EMythicHarvestReceiptPlanStatus::Conflict;
        return OutPlan.Status;
    }
    if (!Existing && RowsByKey.Num() >= GetConfiguredMaximumRows()) {
        OutPlan.Status =
            EMythicHarvestReceiptPlanStatus::CapacityExceeded;
        return OutPlan.Status;
    }

    OutPlan.PreviouslyAppliedQuantity =
        Existing ? Existing->AppliedQuantity : 0;
    OutPlan.RemainingQuantity = TargetQuantity
        - OutPlan.PreviouslyAppliedQuantity;
    if (OutPlan.RemainingQuantity <= 0) {
        OutPlan.Status = EMythicHarvestReceiptPlanStatus::AlreadyApplied;
        return OutPlan.Status;
    }
    OutPlan.Status = EMythicHarvestReceiptPlanStatus::Ready;
    do {
        OutPlan.ReservationToken = FGuid::NewGuid();
    } while (!OutPlan.ReservationToken.IsValid()
             || ReservedPlansByToken.Contains(OutPlan.ReservationToken));
    ReservedPlansByToken.Add(OutPlan.ReservationToken, OutPlan);
    ReservedKeys.Add(Key);
    return OutPlan.Status;
}

bool UMythicHarvestReceiptLedgerComponent::CommitPlannedApply(
    const FMythicHarvestReceiptApplyPlan &Plan, const int64 AppliedDelta,
    const EMythicHarvestQuestReceiptDisposition QuestDisposition) {
    const FMythicHarvestReceiptApplyPlan *ReservedPlan =
        ReservedPlansByToken.Find(Plan.ReservationToken);
    if (!ReservedPlan) {
        return false;
    }
    const FGuid ReservedToken = ReservedPlan->ReservationToken;
    const FMythicHarvestReceiptKey ReservedKey = ReservedPlan->Key;
    auto ReleaseReservationAndFail = [this, ReservedToken,
                                      ReservedKey]() {
        ReservedPlansByToken.Remove(ReservedToken);
        ReservedKeys.Remove(ReservedKey);
        return false;
    };
    if (!Plan.IsReady() || !ApplyPlansMatch(Plan, *ReservedPlan)
        || AppliedDelta < 0 || AppliedDelta > Plan.RemainingQuantity) {
        return ReleaseReservationAndFail();
    }
    const bool bQuest = Plan.Key.Channel
        == EMythicHarvestReceiptChannel::CompletionQuestCredit;
    if ((AppliedDelta > 0 && bQuest
         && (AppliedDelta != Plan.RemainingQuantity
             || (QuestDisposition
                     != EMythicHarvestQuestReceiptDisposition::Matched
                 && QuestDisposition
                     != EMythicHarvestQuestReceiptDisposition::NoMatch)))
        || (AppliedDelta > 0 && !bQuest
            && QuestDisposition
                != EMythicHarvestQuestReceiptDisposition::None)
        || (AppliedDelta > 0 && LedgerRevision == MAX_uint64)) {
        return ReleaseReservationAndFail();
    }

    FMythicSavedHarvestReceiptRowV1 *Existing =
        RowsByKey.Find(Plan.Key);
    if (Existing
        && (!RowsHaveSameContract(*Existing, Plan.PayloadFingerprint,
                                  Plan.TargetQuantity)
            || Existing->AppliedQuantity
                != Plan.PreviouslyAppliedQuantity)) {
        return ReleaseReservationAndFail();
    }

    ReservedPlansByToken.Remove(ReservedToken);
    ReservedKeys.Remove(ReservedKey);
    if (AppliedDelta == 0) {
        return true;
    }

    FMythicSavedHarvestReceiptRowV1 &Row = Existing
        ? *Existing
        : RowsByKey.Add(Plan.Key);
    if (!Existing) {
        Row.Key = Plan.Key;
        Row.PayloadFingerprint = Plan.PayloadFingerprint;
        Row.TargetQuantity = Plan.TargetQuantity;
        Row.FirstObservedWorldSnapshotSequence =
            Plan.FirstObservedWorldSnapshotSequence;
    }
    else if (Plan.Key.Channel
                 == EMythicHarvestReceiptChannel::AppliedWorkProficiencyXP
             || Plan.Key.Channel
                 == EMythicHarvestReceiptChannel::DurabilityCost) {
        Row.TargetQuantity = Plan.TargetQuantity;
    }
    ++LedgerRevision;
    Row.AppliedQuantity += AppliedDelta;
    Row.MutationRevision = LedgerRevision;
    if (bQuest) {
        Row.QuestDisposition = QuestDisposition;
    }
    return true;
}

void UMythicHarvestReceiptLedgerComponent::CancelPlannedApply(
    const FMythicHarvestReceiptApplyPlan &Plan) {
    if (const FMythicHarvestReceiptApplyPlan *ReservedPlan =
            ReservedPlansByToken.Find(Plan.ReservationToken)) {
        const FMythicHarvestReceiptKey ReservedKey = ReservedPlan->Key;
        ReservedPlansByToken.Remove(Plan.ReservationToken);
        ReservedKeys.Remove(ReservedKey);
    }
}

bool UMythicHarvestReceiptLedgerComponent::BuildSaveSnapshot(
    FMythicHarvestReceiptLedgerSaveV1 &OutSnapshot,
    FName &OutDiagnosticCode) const {
    OutSnapshot = FMythicHarvestReceiptLedgerSaveV1();
    if (!ReservedPlansByToken.IsEmpty() || !ReservedKeys.IsEmpty()) {
        OutDiagnosticCode = TEXT("HarvestReceiptMutationInProgress");
        return false;
    }
    OutSnapshot.LedgerRevision = LedgerRevision;
    OutSnapshot.LedgerEpoch = LedgerEpoch;
    RowsByKey.GenerateValueArray(OutSnapshot.Rows);
    OutSnapshot.WorldWatermarks.Reserve(
        MinimumWorldSnapshotByEpoch.Num());
    for (const TPair<FGuid, uint64> &Pair :
         MinimumWorldSnapshotByEpoch) {
        FMythicSavedHarvestReceiptWorldWatermarkV1 &Watermark =
            OutSnapshot.WorldWatermarks.AddDefaulted_GetRef();
        Watermark.WorldEpoch = Pair.Key;
        Watermark.MinimumAcceptedSnapshotSequence = Pair.Value;
    }
    OutSnapshot.SortCanonical();
    if (!FMythicHarvestReceiptLedgerSaveV1::Validate(
            OutSnapshot, OutDiagnosticCode, GetConfiguredMaximumRows())) {
        OutSnapshot = FMythicHarvestReceiptLedgerSaveV1();
        return false;
    }
    OutDiagnosticCode = NAME_None;
    return true;
}

bool UMythicHarvestReceiptLedgerComponent::RestoreSaveSnapshot(
    const FMythicHarvestReceiptLedgerSaveV1 &Snapshot,
    FName &OutDiagnosticCode) {
    if (!ReservedPlansByToken.IsEmpty() || !ReservedKeys.IsEmpty()
        || !FMythicHarvestReceiptLedgerSaveV1::Validate(
            Snapshot, OutDiagnosticCode, GetConfiguredMaximumRows())) {
        if (OutDiagnosticCode.IsNone()) {
            OutDiagnosticCode = TEXT("HarvestReceiptMutationInProgress");
        }
        return false;
    }

    TMap<FMythicHarvestReceiptKey, FMythicSavedHarvestReceiptRowV1>
        RestoredRows;
    TMap<FMythicHarvestReceiptKey, int64> RestoredDurable;
    RestoredRows.Reserve(Snapshot.Rows.Num());
    RestoredDurable.Reserve(Snapshot.Rows.Num());
    for (const FMythicSavedHarvestReceiptRowV1 &Row : Snapshot.Rows) {
        RestoredRows.Add(Row.Key, Row);
        RestoredDurable.Add(Row.Key, Row.AppliedQuantity);
    }
    TMap<FGuid, uint64> RestoredWatermarks;
    for (const FMythicSavedHarvestReceiptWorldWatermarkV1 &Watermark :
         Snapshot.WorldWatermarks) {
        RestoredWatermarks.Add(
            Watermark.WorldEpoch,
            Watermark.MinimumAcceptedSnapshotSequence);
    }

    RowsByKey = MoveTemp(RestoredRows);
    DurableAppliedByKey = MoveTemp(RestoredDurable);
    MinimumWorldSnapshotByEpoch = MoveTemp(RestoredWatermarks);
    LedgerRevision = Snapshot.LedgerRevision;
    DurableLedgerRevision = Snapshot.LedgerRevision;
    LedgerEpoch = Snapshot.LedgerEpoch;
    OutDiagnosticCode = NAME_None;
    return true;
}

bool UMythicHarvestReceiptLedgerComponent::
ValidateLoadSnapshotAgainstWorld(
    const FMythicHarvestReceiptLedgerSaveV1 &Snapshot,
    const FGuid &ActiveWorldEpoch,
    const uint64 ActiveWorldSnapshotSequence,
    FName &OutDiagnosticCode) const {
    if (!ActiveWorldEpoch.IsValid() || ActiveWorldSnapshotSequence == 0) {
        OutDiagnosticCode = TEXT("InvalidActiveHarvestWorldSnapshot");
        return false;
    }
    if (!FMythicHarvestReceiptLedgerSaveV1::Validate(
            Snapshot, OutDiagnosticCode, GetConfiguredMaximumRows())) {
        return false;
    }
    for (const FMythicSavedHarvestReceiptWorldWatermarkV1 &Watermark :
         Snapshot.WorldWatermarks) {
        if (Watermark.WorldEpoch == ActiveWorldEpoch
            && ActiveWorldSnapshotSequence
                < Watermark.MinimumAcceptedSnapshotSequence) {
            OutDiagnosticCode =
                TEXT("WorldSnapshotOlderThanHarvestReceiptWatermark");
            return false;
        }
    }
    OutDiagnosticCode = NAME_None;
    return true;
}

bool UMythicHarvestReceiptLedgerComponent::MarkSnapshotDurable(
    const FMythicHarvestReceiptLedgerSaveV1 &Snapshot,
    FName &OutDiagnosticCode) {
    if (!FMythicHarvestReceiptLedgerSaveV1::Validate(
            Snapshot, OutDiagnosticCode, GetConfiguredMaximumRows())
        || Snapshot.LedgerEpoch != LedgerEpoch) {
        if (OutDiagnosticCode.IsNone()) {
            OutDiagnosticCode = TEXT("HarvestReceiptLedgerLineageMismatch");
        }
        return false;
    }
    for (const FMythicSavedHarvestReceiptRowV1 &Saved : Snapshot.Rows) {
        const FMythicSavedHarvestReceiptRowV1 *Live =
            RowsByKey.Find(Saved.Key);
        if (!Live || !RowsHaveSameContract(
                         *Live, Saved.PayloadFingerprint,
                         Saved.TargetQuantity)
            || ((Saved.Key.Channel
                        == EMythicHarvestReceiptChannel::
                            AppliedWorkProficiencyXP
                    || Saved.Key.Channel
                        == EMythicHarvestReceiptChannel::DurabilityCost)
                && Saved.TargetQuantity > Live->TargetQuantity)
            || Saved.AppliedQuantity > Live->AppliedQuantity) {
            OutDiagnosticCode = TEXT("HarvestReceiptDurableSnapshotConflict");
            return false;
        }
    }
    TMap<FMythicHarvestReceiptKey, int64> CapturedDurableRows;
    CapturedDurableRows.Reserve(Snapshot.Rows.Num());
    for (const FMythicSavedHarvestReceiptRowV1 &Saved : Snapshot.Rows) {
        CapturedDurableRows.Add(Saved.Key, Saved.AppliedQuantity);
    }
    DurableAppliedByKey = MoveTemp(CapturedDurableRows);
    DurableLedgerRevision = Snapshot.LedgerRevision;
    OutDiagnosticCode = NAME_None;
    return true;
}

int64 UMythicHarvestReceiptLedgerComponent::GetAppliedQuantity(
    const FMythicHarvestReceiptKey &Key) const {
    const FMythicSavedHarvestReceiptRowV1 *Row = RowsByKey.Find(Key);
    return Row ? Row->AppliedQuantity : 0;
}

int64 UMythicHarvestReceiptLedgerComponent::GetDurableAppliedQuantity(
    const FMythicHarvestReceiptKey &Key) const {
    const int64 *Quantity = DurableAppliedByKey.Find(Key);
    return Quantity ? *Quantity : 0;
}

bool UMythicHarvestReceiptLedgerComponent::ValidateWorldSnapshotMinimum(
    const FGuid &WorldEpoch, const uint64 SnapshotSequence,
    FName &OutDiagnosticCode) const {
    const uint64 *Minimum = MinimumWorldSnapshotByEpoch.Find(WorldEpoch);
    if (Minimum && SnapshotSequence < *Minimum) {
        OutDiagnosticCode = TEXT("WorldSnapshotOlderThanHarvestReceiptWatermark");
        return false;
    }
    OutDiagnosticCode = NAME_None;
    return true;
}

bool UMythicHarvestReceiptLedgerComponent::CompactCompletedRows(
    const FGuid &WorldEpoch, const uint64 DurableWorldSnapshotSequence,
    const TSet<FMythicHarvestReceiptKey> &DurablePendingKeys,
    const TMap<FMythicHarvestNodeId, uint32>
        &DurableCompletedGenerationByNode,
    int32 &OutRemovedRowCount, FName &OutDiagnosticCode) {
    OutRemovedRowCount = 0;
    if (!WorldEpoch.IsValid() || DurableWorldSnapshotSequence == 0
        || !ReservedPlansByToken.IsEmpty() || !ReservedKeys.IsEmpty()) {
        OutDiagnosticCode = TEXT("InvalidHarvestReceiptCompactionBoundary");
        return false;
    }
    int32 EligibleRowCount = 0;
    for (const TPair<FMythicHarvestReceiptKey,
                     FMythicSavedHarvestReceiptRowV1> &Pair : RowsByKey) {
        const FMythicSavedHarvestReceiptRowV1 &Row = Pair.Value;
        const uint32 *DurableCompletedGeneration =
            DurableCompletedGenerationByNode.Find(Row.Key.NodeId);
        const bool bLifecycleSeriesMayCompact =
            (Row.Key.Channel
                    != EMythicHarvestReceiptChannel::AppliedWorkProficiencyXP
                && Row.Key.Channel
                    != EMythicHarvestReceiptChannel::DurabilityCost)
            || (DurableCompletedGeneration
                && Row.Key.Generation <= *DurableCompletedGeneration);
        if (Row.Key.WorldEpoch == WorldEpoch && Row.IsComplete()
            && bLifecycleSeriesMayCompact
            && Row.FirstObservedWorldSnapshotSequence
                < DurableWorldSnapshotSequence
            && !DurablePendingKeys.Contains(Row.Key)) {
            ++EligibleRowCount;
        }
    }
    if (EligibleRowCount > 0 && LedgerRevision == MAX_uint64) {
        OutDiagnosticCode = TEXT("HarvestReceiptRevisionExhausted");
        return false;
    }
    for (auto It = RowsByKey.CreateIterator(); It; ++It) {
        const FMythicSavedHarvestReceiptRowV1 &Row = It.Value();
        const uint32 *DurableCompletedGeneration =
            DurableCompletedGenerationByNode.Find(Row.Key.NodeId);
        const bool bLifecycleSeriesMayCompact =
            (Row.Key.Channel
                    != EMythicHarvestReceiptChannel::AppliedWorkProficiencyXP
                && Row.Key.Channel
                    != EMythicHarvestReceiptChannel::DurabilityCost)
            || (DurableCompletedGeneration
                && Row.Key.Generation <= *DurableCompletedGeneration);
        if (Row.Key.WorldEpoch != WorldEpoch || !Row.IsComplete()
            || !bLifecycleSeriesMayCompact
            || Row.FirstObservedWorldSnapshotSequence
                >= DurableWorldSnapshotSequence
            || DurablePendingKeys.Contains(Row.Key)) {
            continue;
        }
        DurableAppliedByKey.Remove(Row.Key);
        It.RemoveCurrent();
        ++OutRemovedRowCount;
    }
    if (OutRemovedRowCount > 0) {
        ++LedgerRevision;
        uint64 &Minimum = MinimumWorldSnapshotByEpoch.FindOrAdd(WorldEpoch);
        Minimum = FMath::Max(Minimum, DurableWorldSnapshotSequence);
    }
    OutDiagnosticCode = NAME_None;
    return true;
}

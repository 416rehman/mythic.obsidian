#include "World/Harvesting/MythicHarvestRewardEscrowComponent.h"

#include "GameFramework/Actor.h"
#include "World/Harvesting/MythicHarvestSettings.h"

UMythicHarvestRewardEscrowComponent::
UMythicHarvestRewardEscrowComponent() {
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(false);
    do {
        EscrowEpoch = FGuid::NewGuid();
    } while (!EscrowEpoch.IsValid());
}

int32 UMythicHarvestRewardEscrowComponent::
GetConfiguredMaximumRows() const {
    const UMythicHarvestSettings *Settings =
        GetDefault<UMythicHarvestSettings>();
    return FMath::Clamp(
        Settings ? Settings->RewardItemEscrowMaximumRows
                 : FMythicHarvestItemEscrowSaveV1::AbsoluteMaximumRows,
        1, FMythicHarvestItemEscrowSaveV1::AbsoluteMaximumRows);
}

int32 UMythicHarvestRewardEscrowComponent::
GetAvailableRowCapacity() const {
    return FMath::Max(0, GetConfiguredMaximumRows() - RowsByReceipt.Num());
}

void UMythicHarvestRewardEscrowComponent::NormalizeRetryCursor() {
    RetryRowCursor = DeliveryOrder.IsEmpty()
        ? 0 : FMath::Max(0, RetryRowCursor) % DeliveryOrder.Num();
}

EMythicHarvestEscrowStageStatus
UMythicHarvestRewardEscrowComponent::TryStage(
    const FMythicSavedHarvestItemEscrowRowV1 &Row) {
    if (!GetOwner() || !GetOwner()->HasAuthority() || !Row.IsValid()) {
        return EMythicHarvestEscrowStageStatus::Invalid;
    }
    if (const FMythicSavedHarvestItemEscrowRowV1 *Existing =
            RowsByReceipt.Find(Row.ReceiptKey)) {
        return Existing->HasSameContract(Row)
            ? EMythicHarvestEscrowStageStatus::AlreadyStaged
            : EMythicHarvestEscrowStageStatus::Conflict;
    }
    if (RowsByReceipt.Num() >= GetConfiguredMaximumRows()) {
        return EMythicHarvestEscrowStageStatus::CapacityExceeded;
    }
    if (EscrowRevision == MAX_uint64) {
        return EMythicHarvestEscrowStageStatus::RevisionExhausted;
    }
    FMythicSavedHarvestItemEscrowRowV1 Staged = Row;
    ++EscrowRevision;
    Staged.MutationRevision = EscrowRevision;
    RowsByReceipt.Add(Staged.ReceiptKey, Staged);
    DeliveryOrder.Add(Staged.ReceiptKey);
    NormalizeRetryCursor();
    return EMythicHarvestEscrowStageStatus::Staged;
}

bool UMythicHarvestRewardEscrowComponent::TryPlanNextDelivery(
    const int32 MaximumQuantity,
    FMythicHarvestEscrowDeliveryPlan &OutPlan,
    FMythicSavedHarvestItemEscrowRowV1 &OutRow) {
    OutPlan = FMythicHarvestEscrowDeliveryPlan();
    OutRow = FMythicSavedHarvestItemEscrowRowV1();
    if (!GetOwner() || !GetOwner()->HasAuthority()
        || MaximumQuantity <= 0 || ActiveDeliveryPlan.IsSet()
        || DeliveryOrder.IsEmpty()) {
        return false;
    }
    NormalizeRetryCursor();
    const FMythicHarvestReceiptKey &Key = DeliveryOrder[RetryRowCursor];
    const FMythicSavedHarvestItemEscrowRowV1 *Row =
        RowsByReceipt.Find(Key);
    if (!Row || !Row->IsValid()) {
        return false;
    }
    do {
        OutPlan.ReservationToken = FGuid::NewGuid();
    } while (!OutPlan.ReservationToken.IsValid());
    OutPlan.ReceiptKey = Key;
    OutPlan.PreviouslyRemainingQuantity = Row->RemainingQuantity;
    OutPlan.RequestedQuantity = FMath::Min(
        Row->RemainingQuantity, MaximumQuantity);
    ActiveDeliveryPlan = OutPlan;
    OutRow = *Row;
    OutRow.RemainingQuantity = OutPlan.RequestedQuantity;
    return true;
}

bool UMythicHarvestRewardEscrowComponent::DeliveryPlansMatch(
    const FMythicHarvestEscrowDeliveryPlan &Left,
    const FMythicHarvestEscrowDeliveryPlan &Right) {
    return Left.ReservationToken == Right.ReservationToken
        && Left.ReceiptKey == Right.ReceiptKey
        && Left.PreviouslyRemainingQuantity
            == Right.PreviouslyRemainingQuantity
        && Left.RequestedQuantity == Right.RequestedQuantity;
}

bool UMythicHarvestRewardEscrowComponent::CommitPlannedDelivery(
    const FMythicHarvestEscrowDeliveryPlan &Plan,
    const int32 InsertedQuantity) {
    if (!ActiveDeliveryPlan.IsSet()
        || !DeliveryPlansMatch(Plan, ActiveDeliveryPlan.GetValue())
        || !Plan.IsValid() || InsertedQuantity < 0
        || InsertedQuantity > Plan.RequestedQuantity) {
        ActiveDeliveryPlan.Reset();
        return false;
    }
    FMythicSavedHarvestItemEscrowRowV1 *Row =
        RowsByReceipt.Find(Plan.ReceiptKey);
    if (!Row
        || Row->RemainingQuantity != Plan.PreviouslyRemainingQuantity) {
        ActiveDeliveryPlan.Reset();
        return false;
    }
    if (InsertedQuantity == 0) {
        ActiveDeliveryPlan.Reset();
        if (!DeliveryOrder.IsEmpty()) {
            RetryRowCursor = (RetryRowCursor + 1) % DeliveryOrder.Num();
        }
        return true;
    }
    if (EscrowRevision == MAX_uint64) {
        ActiveDeliveryPlan.Reset();
        return false;
    }

    ++EscrowRevision;
    Row->RemainingQuantity -= InsertedQuantity;
    Row->MutationRevision = EscrowRevision;
    ActiveDeliveryPlan.Reset();
    if (Row->RemainingQuantity > 0) {
        RetryRowCursor = (RetryRowCursor + 1) % DeliveryOrder.Num();
        return true;
    }

    const int32 RemovedIndex = RetryRowCursor;
    RowsByReceipt.Remove(Plan.ReceiptKey);
    DeliveryOrder.RemoveAt(RemovedIndex, 1, EAllowShrinking::No);
    NormalizeRetryCursor();
    return true;
}

void UMythicHarvestRewardEscrowComponent::CancelPlannedDelivery(
    const FMythicHarvestEscrowDeliveryPlan &Plan) {
    if (ActiveDeliveryPlan.IsSet()
        && DeliveryPlansMatch(Plan, ActiveDeliveryPlan.GetValue())) {
        ActiveDeliveryPlan.Reset();
        if (!DeliveryOrder.IsEmpty()) {
            RetryRowCursor = (RetryRowCursor + 1) % DeliveryOrder.Num();
        }
    }
}

bool UMythicHarvestRewardEscrowComponent::BuildSaveSnapshot(
    FMythicHarvestItemEscrowSaveV1 &OutSnapshot,
    FName &OutDiagnosticCode) const {
    OutSnapshot = FMythicHarvestItemEscrowSaveV1();
    if (ActiveDeliveryPlan.IsSet()) {
        OutDiagnosticCode = TEXT("HarvestItemEscrowMutationInProgress");
        return false;
    }
    OutSnapshot.EscrowEpoch = EscrowEpoch;
    OutSnapshot.EscrowRevision = EscrowRevision;
    OutSnapshot.RetryRowCursor = RetryRowCursor;
    RowsByReceipt.GenerateValueArray(OutSnapshot.Rows);
    OutSnapshot.SortCanonical();
    if (!FMythicHarvestItemEscrowSaveV1::Validate(
            OutSnapshot, OutDiagnosticCode, GetConfiguredMaximumRows())) {
        OutSnapshot = FMythicHarvestItemEscrowSaveV1();
        return false;
    }
    OutDiagnosticCode = NAME_None;
    return true;
}

bool UMythicHarvestRewardEscrowComponent::RestoreSaveSnapshot(
    const FMythicHarvestItemEscrowSaveV1 &Snapshot,
    FName &OutDiagnosticCode) {
    if (ActiveDeliveryPlan.IsSet()
        || !FMythicHarvestItemEscrowSaveV1::Validate(
            Snapshot, OutDiagnosticCode, GetConfiguredMaximumRows())) {
        if (OutDiagnosticCode.IsNone()) {
            OutDiagnosticCode = TEXT("HarvestItemEscrowMutationInProgress");
        }
        return false;
    }
    TMap<FMythicHarvestReceiptKey, FMythicSavedHarvestItemEscrowRowV1>
        RestoredRows;
    TArray<FMythicHarvestReceiptKey> RestoredOrder;
    RestoredRows.Reserve(Snapshot.Rows.Num());
    RestoredOrder.Reserve(Snapshot.Rows.Num());
    for (const FMythicSavedHarvestItemEscrowRowV1 &Row : Snapshot.Rows) {
        RestoredRows.Add(Row.ReceiptKey, Row);
        RestoredOrder.Add(Row.ReceiptKey);
    }
    RowsByReceipt = MoveTemp(RestoredRows);
    DeliveryOrder = MoveTemp(RestoredOrder);
    RetryRowCursor = Snapshot.RetryRowCursor;
    EscrowRevision = Snapshot.EscrowRevision;
    DurableEscrowRevision = Snapshot.EscrowRevision;
    EscrowEpoch = Snapshot.EscrowEpoch;
    NormalizeRetryCursor();
    OutDiagnosticCode = NAME_None;
    return true;
}

bool UMythicHarvestRewardEscrowComponent::MarkSnapshotDurable(
    const FMythicHarvestItemEscrowSaveV1 &Snapshot,
    FName &OutDiagnosticCode) {
    if (!FMythicHarvestItemEscrowSaveV1::Validate(
            Snapshot, OutDiagnosticCode, GetConfiguredMaximumRows())
        || Snapshot.EscrowEpoch != EscrowEpoch
        || Snapshot.EscrowRevision > EscrowRevision) {
        if (OutDiagnosticCode.IsNone()) {
            OutDiagnosticCode = TEXT("HarvestItemEscrowLineageMismatch");
        }
        return false;
    }
    for (const FMythicSavedHarvestItemEscrowRowV1 &Saved : Snapshot.Rows) {
        const FMythicSavedHarvestItemEscrowRowV1 *Live =
            RowsByReceipt.Find(Saved.ReceiptKey);
        if (Live
            && (!Live->HasSameContract(Saved)
                || Saved.RemainingQuantity < Live->RemainingQuantity)) {
            OutDiagnosticCode = TEXT("HarvestItemEscrowDurableSnapshotConflict");
            return false;
        }
    }
    DurableEscrowRevision = Snapshot.EscrowRevision;
    OutDiagnosticCode = NAME_None;
    return true;
}

bool UMythicHarvestRewardEscrowComponent::ContainsExactContract(
    const FMythicSavedHarvestItemEscrowRowV1 &Row) const {
    const FMythicSavedHarvestItemEscrowRowV1 *Existing =
        RowsByReceipt.Find(Row.ReceiptKey);
    return Existing && Existing->HasSameContract(Row);
}

const FMythicSavedHarvestItemEscrowRowV1 *
UMythicHarvestRewardEscrowComponent::FindRow(
    const FMythicHarvestReceiptKey &ReceiptKey) const {
    return RowsByReceipt.Find(ReceiptKey);
}

void UMythicHarvestRewardEscrowComponent::AppendReceiptKeys(
    TSet<FMythicHarvestReceiptKey> &OutKeys) const {
    for (const TPair<FMythicHarvestReceiptKey,
                     FMythicSavedHarvestItemEscrowRowV1> &Pair :
         RowsByReceipt) {
        OutKeys.Add(Pair.Key);
    }
}

bool UMythicHarvestRewardEscrowComponent::
SnapshotContainsExactContract(
    const FMythicHarvestItemEscrowSaveV1 &Snapshot,
    const FMythicSavedHarvestItemEscrowRowV1 &Row) {
    const FMythicSavedHarvestItemEscrowRowV1 *Saved =
        Snapshot.FindRow(Row.ReceiptKey);
    return Saved && Saved->HasSameContract(Row)
        && Saved->RemainingQuantity <= Row.OriginalQuantity;
}


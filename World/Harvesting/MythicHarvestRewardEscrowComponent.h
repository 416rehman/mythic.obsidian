#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "World/Harvesting/MythicHarvestRewardEscrowTypes.h"

#include "MythicHarvestRewardEscrowComponent.generated.h"

enum class EMythicHarvestEscrowStageStatus : uint8 {
    Staged,
    AlreadyStaged,
    Conflict,
    CapacityExceeded,
    RevisionExhausted,
    Invalid,
};

/** Opaque reservation protecting inventory insertion and escrow reduction from re-entrant character saves. */
struct MYTHIC_API FMythicHarvestEscrowDeliveryPlan {
private:
    friend class UMythicHarvestRewardEscrowComponent;
    FGuid ReservationToken;

public:
    FMythicHarvestReceiptKey ReceiptKey;
    int32 PreviouslyRemainingQuantity = 0;
    int32 RequestedQuantity = 0;

    bool IsValid() const {
        return ReservationToken.IsValid() && ReceiptKey.IsValid()
            && PreviouslyRemainingQuantity > 0 && RequestedQuantity > 0
            && RequestedQuantity <= PreviouslyRemainingQuantity;
    }
};

/**
 * Authority-only character-owned mailbox for deterministic harvest items that do not currently fit in inventory.
 * Inventory and this queue share one character-save file, so a successful physical write atomically commits both.
 */
UCLASS()
class MYTHIC_API UMythicHarvestRewardEscrowComponent final
    : public UActorComponent {
    GENERATED_BODY()

public:
    UMythicHarvestRewardEscrowComponent();

    EMythicHarvestEscrowStageStatus TryStage(
        const FMythicSavedHarvestItemEscrowRowV1 &Row);

    bool TryPlanNextDelivery(
        int32 MaximumQuantity,
        FMythicHarvestEscrowDeliveryPlan &OutPlan,
        FMythicSavedHarvestItemEscrowRowV1 &OutRow);

    bool CommitPlannedDelivery(
        const FMythicHarvestEscrowDeliveryPlan &Plan,
        int32 InsertedQuantity);

    void CancelPlannedDelivery(
        const FMythicHarvestEscrowDeliveryPlan &Plan);

    bool BuildSaveSnapshot(
        FMythicHarvestItemEscrowSaveV1 &OutSnapshot,
        FName &OutDiagnosticCode) const;

    bool RestoreSaveSnapshot(
        const FMythicHarvestItemEscrowSaveV1 &Snapshot,
        FName &OutDiagnosticCode);

    bool MarkSnapshotDurable(
        const FMythicHarvestItemEscrowSaveV1 &Snapshot,
        FName &OutDiagnosticCode);

    bool ContainsExactContract(
        const FMythicSavedHarvestItemEscrowRowV1 &Row) const;

    const FMythicSavedHarvestItemEscrowRowV1 *FindRow(
        const FMythicHarvestReceiptKey &ReceiptKey) const;

    void AppendReceiptKeys(TSet<FMythicHarvestReceiptKey> &OutKeys) const;

    int32 GetPendingRowCount() const { return RowsByReceipt.Num(); }
    int32 GetAvailableRowCapacity() const;
    bool HasPendingDelivery() const { return !DeliveryOrder.IsEmpty(); }
    bool HasOpenMutation() const { return ActiveDeliveryPlan.IsSet(); }
    bool HasUndurableMutation() const {
        return EscrowRevision > DurableEscrowRevision;
    }
    uint64 GetEscrowRevision() const { return EscrowRevision; }

    static bool SnapshotContainsExactContract(
        const FMythicHarvestItemEscrowSaveV1 &Snapshot,
        const FMythicSavedHarvestItemEscrowRowV1 &Row);

private:
    int32 GetConfiguredMaximumRows() const;
    void NormalizeRetryCursor();
    static bool DeliveryPlansMatch(
        const FMythicHarvestEscrowDeliveryPlan &Left,
        const FMythicHarvestEscrowDeliveryPlan &Right);

    UPROPERTY(Transient)
    TMap<FMythicHarvestReceiptKey, FMythicSavedHarvestItemEscrowRowV1>
        RowsByReceipt;

    TArray<FMythicHarvestReceiptKey> DeliveryOrder;
    TOptional<FMythicHarvestEscrowDeliveryPlan> ActiveDeliveryPlan;
    int32 RetryRowCursor = 0;
    uint64 EscrowRevision = 1;
    uint64 DurableEscrowRevision = 1;
    FGuid EscrowEpoch;
};


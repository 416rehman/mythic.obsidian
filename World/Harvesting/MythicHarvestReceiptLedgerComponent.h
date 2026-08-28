#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "World/Harvesting/MythicHarvestReceiptTypes.h"

#include "MythicHarvestReceiptLedgerComponent.generated.h"

/** Result of inspecting and reserving one cumulative receipt application. */
enum class EMythicHarvestReceiptPlanStatus : uint8 {
    Ready,
    AlreadyApplied,
    Busy,
    Conflict,
    CapacityExceeded,
    Invalid,
};

/** Opaque native reservation returned before an entitlement side effect is attempted. */
struct MYTHIC_API FMythicHarvestReceiptApplyPlan {
private:
    friend class UMythicHarvestReceiptLedgerComponent;

    /** Transient capability identifying the exact in-flight reservation. */
    FGuid ReservationToken;

public:
    EMythicHarvestReceiptPlanStatus Status =
        EMythicHarvestReceiptPlanStatus::Invalid;
    FMythicHarvestReceiptKey Key;
    FGuid PayloadFingerprint;
    int64 TargetQuantity = 0;
    int64 PreviouslyAppliedQuantity = 0;
    int64 RemainingQuantity = 0;
    uint64 FirstObservedWorldSnapshotSequence = 0;

    bool IsReady() const {
        return Status == EMythicHarvestReceiptPlanStatus::Ready;
    }
};

/**
 * Authority-only player-owned exactly-once ledger for harvest delivery.
 *
 * Gameplay side effects and their cumulative receipt commit occur on the game thread under a per-key reservation.
 * Character snapshot capture fails closed while a reservation is open. Durable acknowledgement remains separate and
 * is advanced only from the exact receipt snapshot carried by a successful character-save callback.
 */
UCLASS()
class MYTHIC_API UMythicHarvestReceiptLedgerComponent final
    : public UActorComponent {
    GENERATED_BODY()

public:
    UMythicHarvestReceiptLedgerComponent();

    EMythicHarvestReceiptPlanStatus TryPlanApply(
        const FMythicHarvestReceiptKey &Key,
        const FGuid &PayloadFingerprint,
        int64 TargetQuantity,
        uint64 FirstObservedWorldSnapshotSequence,
        FMythicHarvestReceiptApplyPlan &OutPlan);

    bool CommitPlannedApply(
        const FMythicHarvestReceiptApplyPlan &Plan,
        int64 AppliedDelta,
        EMythicHarvestQuestReceiptDisposition QuestDisposition =
            EMythicHarvestQuestReceiptDisposition::None);

    void CancelPlannedApply(const FMythicHarvestReceiptApplyPlan &Plan);

    bool BuildSaveSnapshot(FMythicHarvestReceiptLedgerSaveV1 &OutSnapshot,
                           FName &OutDiagnosticCode) const;

    bool RestoreSaveSnapshot(
        const FMythicHarvestReceiptLedgerSaveV1 &Snapshot,
        FName &OutDiagnosticCode);

    /**
     * Preflights a character receipt snapshot against the active durable world snapshot before character state is
     * installed. This closes rollback replays after completed receipt rows have been compacted.
     */
    bool ValidateLoadSnapshotAgainstWorld(
        const FMythicHarvestReceiptLedgerSaveV1 &Snapshot,
        const FGuid &ActiveWorldEpoch,
        uint64 ActiveWorldSnapshotSequence,
        FName &OutDiagnosticCode) const;

    bool MarkSnapshotDurable(
        const FMythicHarvestReceiptLedgerSaveV1 &Snapshot,
        FName &OutDiagnosticCode);

    int64 GetAppliedQuantity(const FMythicHarvestReceiptKey &Key) const;
    int64 GetDurableAppliedQuantity(
        const FMythicHarvestReceiptKey &Key) const;

    bool ValidateWorldSnapshotMinimum(const FGuid &WorldEpoch,
                                      uint64 SnapshotSequence,
                                      FName &OutDiagnosticCode) const;

    bool CompactCompletedRows(
        const FGuid &WorldEpoch,
        uint64 DurableWorldSnapshotSequence,
        const TSet<FMythicHarvestReceiptKey> &DurablePendingKeys,
        const TMap<FMythicHarvestNodeId, uint32>
            &DurableCompletedGenerationByNode,
        int32 &OutRemovedRowCount,
        FName &OutDiagnosticCode);

    bool HasOpenMutation() const {
        return !ReservedPlansByToken.IsEmpty();
    }
    bool HasUndurableMutation() const {
        return LedgerRevision > DurableLedgerRevision;
    }
    int32 GetReceiptRowCount() const { return RowsByKey.Num(); }
    uint64 GetLedgerRevision() const { return LedgerRevision; }
    const FGuid &GetLedgerEpoch() const { return LedgerEpoch; }

private:
    int32 GetConfiguredMaximumRows() const;
    static bool RowsHaveSameContract(
        const FMythicSavedHarvestReceiptRowV1 &Row,
        const FGuid &PayloadFingerprint,
        int64 TargetQuantity);
    static bool ApplyPlansMatch(
        const FMythicHarvestReceiptApplyPlan &Left,
        const FMythicHarvestReceiptApplyPlan &Right);

    UPROPERTY(Transient)
    TMap<FMythicHarvestReceiptKey, FMythicSavedHarvestReceiptRowV1>
        RowsByKey;

    UPROPERTY(Transient)
    TMap<FMythicHarvestReceiptKey, int64> DurableAppliedByKey;

    UPROPERTY(Transient)
    TMap<FGuid, uint64> MinimumWorldSnapshotByEpoch;

    TMap<FGuid, FMythicHarvestReceiptApplyPlan> ReservedPlansByToken;
    TSet<FMythicHarvestReceiptKey> ReservedKeys;
    uint64 LedgerRevision = 0;
    uint64 DurableLedgerRevision = 0;
    FGuid LedgerEpoch;
};

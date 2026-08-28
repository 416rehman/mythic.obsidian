#pragma once

#include "CoreMinimal.h"
#include "World/Gathering/MythicYieldQuality.h"
#include "World/Harvesting/MythicHarvestReceiptTypes.h"

#include "MythicHarvestRewardEscrowTypes.generated.h"

/**
 * One immutable deterministic item entitlement accepted by a character but not yet fully inserted into inventory.
 * The character save owns this row after the world outbox's exact durable hand-off barrier succeeds.
 */
USTRUCT()
struct MYTHIC_API FMythicSavedHarvestItemEscrowRowV1 {
    GENERATED_BODY()

    UPROPERTY(SaveGame)
    FMythicHarvestReceiptKey ReceiptKey;

    UPROPERTY(SaveGame)
    FGuid ReceiptPayloadFingerprint;

    UPROPERTY(SaveGame)
    FPrimaryAssetId ItemDefinitionId;

    UPROPERTY(SaveGame)
    int32 OriginalQuantity = 0;

    UPROPERTY(SaveGame)
    int32 RemainingQuantity = 0;

    UPROPERTY(SaveGame)
    int32 ItemLevel = 1;

    UPROPERTY(SaveGame)
    bool bHasResolvedQuality = false;

    UPROPERTY(SaveGame)
    EMythicYieldQuality ResolvedQuality = EMythicYieldQuality::Common;

    UPROPERTY(SaveGame)
    uint64 ItemSeed = 0;

    /** World-outbox sequence at which the matching receipt contract first became observable. */
    UPROPERTY(SaveGame)
    uint64 FirstObservedWorldSnapshotSequence = 0;

    /** Character-escrow revision that most recently changed RemainingQuantity. */
    UPROPERTY(SaveGame)
    uint64 MutationRevision = 0;

    bool HasSameContract(const FMythicSavedHarvestItemEscrowRowV1 &Other) const;
    bool IsValid() const;

    static uint32 PackQualityAuxiliary(
        bool bInHasResolvedQuality,
        EMythicYieldQuality InResolvedQuality);
};

/** Complete replace-on-restore character-owned harvest item escrow. */
USTRUCT()
struct MYTHIC_API FMythicHarvestItemEscrowSaveV1 {
    GENERATED_BODY()

    static constexpr uint32 CurrentSchemaVersion = 1;
    static constexpr int32 AbsoluteMaximumRows = 4096;

    UPROPERTY(SaveGame)
    uint32 SchemaVersion = CurrentSchemaVersion;

    /** Stable lineage for this character-owned queue. */
    UPROPERTY(SaveGame)
    FGuid EscrowEpoch;

    UPROPERTY(SaveGame)
    uint64 EscrowRevision = 0;

    /** Next canonical row to receive an inventory-delivery attempt. */
    UPROPERTY(SaveGame)
    int32 RetryRowCursor = 0;

    UPROPERTY(SaveGame)
    TArray<FMythicSavedHarvestItemEscrowRowV1> Rows;

    void SortCanonical();

    static bool Validate(
        const FMythicHarvestItemEscrowSaveV1 &Snapshot,
        FName &OutDiagnosticCode,
        int32 ConfiguredMaximumRows = AbsoluteMaximumRows);

    /** Every queued item must be backed by the exact fully accepted receipt in the same atomic character snapshot. */
    static bool ValidateReceiptBinding(
        const FMythicHarvestItemEscrowSaveV1 &EscrowSnapshot,
        const FMythicHarvestReceiptLedgerSaveV1 &ReceiptSnapshot,
        FName &OutDiagnosticCode);

    const FMythicSavedHarvestItemEscrowRowV1 *FindRow(
        const FMythicHarvestReceiptKey &ReceiptKey) const;
};

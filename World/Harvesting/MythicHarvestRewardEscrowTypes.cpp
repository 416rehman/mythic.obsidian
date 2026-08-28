#include "World/Harvesting/MythicHarvestRewardEscrowTypes.h"

#include "System/MythicAssetManager.h"

namespace MythicHarvestEscrowTypesPrivate {

bool GuidLess(const FGuid &Left, const FGuid &Right) {
    if (Left.A != Right.A) return Left.A < Right.A;
    if (Left.B != Right.B) return Left.B < Right.B;
    if (Left.C != Right.C) return Left.C < Right.C;
    return Left.D < Right.D;
}

bool ReceiptKeyLess(const FMythicHarvestReceiptKey &Left,
                    const FMythicHarvestReceiptKey &Right) {
    if (Left.WorldEpoch != Right.WorldEpoch) {
        return GuidLess(Left.WorldEpoch, Right.WorldEpoch);
    }
    if (Left.NodeId.GetGuid() != Right.NodeId.GetGuid()) {
        return GuidLess(Left.NodeId.GetGuid(), Right.NodeId.GetGuid());
    }
    if (Left.Generation != Right.Generation) {
        return Left.Generation < Right.Generation;
    }
    if (Left.Channel != Right.Channel) {
        return static_cast<uint8>(Left.Channel)
            < static_cast<uint8>(Right.Channel);
    }
    if (Left.EntryOrdinal != Right.EntryOrdinal) {
        return Left.EntryOrdinal < Right.EntryOrdinal;
    }
    return GuidLess(Left.SeriesGuid, Right.SeriesGuid);
}

bool IsValidQuality(const EMythicYieldQuality Quality) {
    return Quality == EMythicYieldQuality::Ragged
        || Quality == EMythicYieldQuality::Common
        || Quality == EMythicYieldQuality::Fine
        || Quality == EMythicYieldQuality::Pristine;
}

} // namespace MythicHarvestEscrowTypesPrivate

uint32 FMythicSavedHarvestItemEscrowRowV1::PackQualityAuxiliary(
    const bool bInHasResolvedQuality,
    const EMythicYieldQuality InResolvedQuality) {
    return static_cast<uint32>(static_cast<uint8>(InResolvedQuality))
        | (bInHasResolvedQuality ? 0x100u : 0u);
}

bool FMythicSavedHarvestItemEscrowRowV1::HasSameContract(
    const FMythicSavedHarvestItemEscrowRowV1 &Other) const {
    return ReceiptKey == Other.ReceiptKey
        && ReceiptPayloadFingerprint == Other.ReceiptPayloadFingerprint
        && ItemDefinitionId == Other.ItemDefinitionId
        && OriginalQuantity == Other.OriginalQuantity
        && ItemLevel == Other.ItemLevel
        && bHasResolvedQuality == Other.bHasResolvedQuality
        && ResolvedQuality == Other.ResolvedQuality
        && ItemSeed == Other.ItemSeed
        && FirstObservedWorldSnapshotSequence
            == Other.FirstObservedWorldSnapshotSequence;
}

bool FMythicSavedHarvestItemEscrowRowV1::IsValid() const {
    const bool bItemChannel = ReceiptKey.Channel
            == EMythicHarvestReceiptChannel::PrimaryMaterial
        || ReceiptKey.Channel == EMythicHarvestReceiptChannel::BonusLoot;
    const bool bQualityValid =
        MythicHarvestEscrowTypesPrivate::IsValidQuality(ResolvedQuality)
        && (bHasResolvedQuality
                ? ResolvedQuality != EMythicYieldQuality::Ragged
                : ResolvedQuality == EMythicYieldQuality::Common);
    const FGuid ExpectedFingerprint = FMythicHarvestReceiptFingerprint::Build(
        ReceiptKey, ItemDefinitionId, OriginalQuantity, ItemSeed,
        static_cast<uint32>(ItemLevel),
        PackQualityAuxiliary(bHasResolvedQuality, ResolvedQuality));
    return ReceiptKey.IsValid() && bItemChannel
        && ReceiptPayloadFingerprint.IsValid()
        && ReceiptPayloadFingerprint == ExpectedFingerprint
        && ItemDefinitionId.IsValid()
        && ItemDefinitionId.PrimaryAssetType
            == UMythicAssetManager::ItemDefinitionType
        && OriginalQuantity > 0 && RemainingQuantity > 0
        && RemainingQuantity <= OriginalQuantity && ItemLevel > 0
        && ItemSeed != 0 && FirstObservedWorldSnapshotSequence > 0
        && MutationRevision > 0 && bQualityValid;
}

void FMythicHarvestItemEscrowSaveV1::SortCanonical() {
    Rows.Sort([](const FMythicSavedHarvestItemEscrowRowV1 &Left,
                 const FMythicSavedHarvestItemEscrowRowV1 &Right) {
        return MythicHarvestEscrowTypesPrivate::ReceiptKeyLess(
            Left.ReceiptKey, Right.ReceiptKey);
    });
    RetryRowCursor = Rows.IsEmpty()
        ? 0 : FMath::Max(0, RetryRowCursor) % Rows.Num();
}

bool FMythicHarvestItemEscrowSaveV1::Validate(
    const FMythicHarvestItemEscrowSaveV1 &Snapshot,
    FName &OutDiagnosticCode, const int32 ConfiguredMaximumRows) {
    if (Snapshot.SchemaVersion != CurrentSchemaVersion
        || !Snapshot.EscrowEpoch.IsValid()
        || ConfiguredMaximumRows < 1
        || ConfiguredMaximumRows > AbsoluteMaximumRows
        || Snapshot.Rows.Num() > ConfiguredMaximumRows
        || Snapshot.EscrowRevision == 0
        || Snapshot.RetryRowCursor < 0
        || (Snapshot.Rows.IsEmpty()
                ? Snapshot.RetryRowCursor != 0
                : Snapshot.RetryRowCursor >= Snapshot.Rows.Num())) {
        OutDiagnosticCode = TEXT("InvalidHarvestItemEscrowHeader");
        return false;
    }

    TSet<FMythicHarvestReceiptKey> SeenKeys;
    SeenKeys.Reserve(Snapshot.Rows.Num());
    for (const FMythicSavedHarvestItemEscrowRowV1 &Row : Snapshot.Rows) {
        if (!Row.IsValid() || Row.MutationRevision > Snapshot.EscrowRevision
            || SeenKeys.Contains(Row.ReceiptKey)) {
            OutDiagnosticCode = TEXT("InvalidHarvestItemEscrowRow");
            return false;
        }
        SeenKeys.Add(Row.ReceiptKey);
    }
    OutDiagnosticCode = NAME_None;
    return true;
}

bool FMythicHarvestItemEscrowSaveV1::ValidateReceiptBinding(
    const FMythicHarvestItemEscrowSaveV1 &EscrowSnapshot,
    const FMythicHarvestReceiptLedgerSaveV1 &ReceiptSnapshot,
    FName &OutDiagnosticCode) {
    for (const FMythicSavedHarvestItemEscrowRowV1 &EscrowRow :
         EscrowSnapshot.Rows) {
        const FMythicSavedHarvestReceiptRowV1 *ReceiptRow =
            ReceiptSnapshot.FindRow(EscrowRow.ReceiptKey);
        if (!ReceiptRow
            || ReceiptRow->PayloadFingerprint
                != EscrowRow.ReceiptPayloadFingerprint
            || ReceiptRow->TargetQuantity != EscrowRow.OriginalQuantity
            || ReceiptRow->AppliedQuantity != EscrowRow.OriginalQuantity) {
            OutDiagnosticCode = TEXT("HarvestItemEscrowReceiptBindingMismatch");
            return false;
        }
    }
    OutDiagnosticCode = NAME_None;
    return true;
}

const FMythicSavedHarvestItemEscrowRowV1 *
FMythicHarvestItemEscrowSaveV1::FindRow(
    const FMythicHarvestReceiptKey &ReceiptKey) const {
    return Rows.FindByPredicate(
        [&ReceiptKey](const FMythicSavedHarvestItemEscrowRowV1 &Row) {
            return Row.ReceiptKey == ReceiptKey;
        });
}

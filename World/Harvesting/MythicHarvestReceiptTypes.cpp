#include "World/Harvesting/MythicHarvestReceiptTypes.h"

#include "Hash/Blake3.h"

namespace MythicHarvestReceiptTypesPrivate {

bool GuidLess(const FGuid &Left, const FGuid &Right) {
    if (Left.A != Right.A) return Left.A < Right.A;
    if (Left.B != Right.B) return Left.B < Right.B;
    if (Left.C != Right.C) return Left.C < Right.C;
    return Left.D < Right.D;
}

void AddUInt32(TArray<uint8> &Bytes, const uint32 Value) {
    Bytes.Add(static_cast<uint8>(Value));
    Bytes.Add(static_cast<uint8>(Value >> 8));
    Bytes.Add(static_cast<uint8>(Value >> 16));
    Bytes.Add(static_cast<uint8>(Value >> 24));
}

void AddUInt64(TArray<uint8> &Bytes, const uint64 Value) {
    AddUInt32(Bytes, static_cast<uint32>(Value));
    AddUInt32(Bytes, static_cast<uint32>(Value >> 32));
}

void AddGuid(TArray<uint8> &Bytes, const FGuid &Value) {
    AddUInt32(Bytes, static_cast<uint32>(Value.A));
    AddUInt32(Bytes, static_cast<uint32>(Value.B));
    AddUInt32(Bytes, static_cast<uint32>(Value.C));
    AddUInt32(Bytes, static_cast<uint32>(Value.D));
}

bool AddUtf8(TArray<uint8> &Bytes, const FString &Value) {
    FTCHARToUTF8 Utf8(*Value);
    if (Utf8.Length() < 0 || Utf8.Length() > MAX_int32) {
        return false;
    }
    AddUInt32(Bytes, static_cast<uint32>(Utf8.Length()));
    Bytes.Append(reinterpret_cast<const uint8 *>(Utf8.Get()), Utf8.Length());
    return true;
}

bool KeyLess(const FMythicHarvestReceiptKey &Left,
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

bool IsSupportedChannel(const EMythicHarvestReceiptChannel Channel) {
    return Channel == EMythicHarvestReceiptChannel::PrimaryMaterial
        || Channel == EMythicHarvestReceiptChannel::BonusLoot
        || Channel == EMythicHarvestReceiptChannel::CompletionProficiencyXP
        || Channel == EMythicHarvestReceiptChannel::CompletionQuestCredit
        || Channel == EMythicHarvestReceiptChannel::AppliedWorkProficiencyXP
        || Channel == EMythicHarvestReceiptChannel::DurabilityCost;
}

} // namespace MythicHarvestReceiptTypesPrivate

FMythicHarvestReceiptKey FMythicHarvestReceiptKey::MakeCompletion(
    const FGuid &InWorldEpoch, const FMythicHarvestNodeId &InNodeId,
    const uint32 InGeneration,
    const EMythicHarvestReceiptChannel InChannel,
    const uint32 InEntryOrdinal) {
    FMythicHarvestReceiptKey Key;
    Key.WorldEpoch = InWorldEpoch;
    Key.NodeId = InNodeId;
    Key.Generation = InGeneration;
    Key.Channel = InChannel;
    Key.EntryOrdinal = InEntryOrdinal;
    return Key;
}

FMythicHarvestReceiptKey FMythicHarvestReceiptKey::MakeAppliedWork(
    const FGuid &InWorldEpoch, const FMythicHarvestNodeId &InNodeId,
    const uint32 InGeneration, const FString &ContributorKey) {
    FMythicHarvestReceiptKey Key = MakeCompletion(
        InWorldEpoch, InNodeId, InGeneration,
        EMythicHarvestReceiptChannel::AppliedWorkProficiencyXP);
    if (!Key.WorldEpoch.IsValid() || !Key.NodeId.IsValid()
        || Key.Generation == 0 || ContributorKey.IsEmpty()) {
        return Key;
    }

    TArray<uint8> Bytes;
    static constexpr ANSICHAR Domain[] =
        "MYTHIC_HARVEST_WORK_RECEIPT_SERIES_V1";
    Bytes.Append(reinterpret_cast<const uint8 *>(Domain), sizeof(Domain));
    MythicHarvestReceiptTypesPrivate::AddGuid(Bytes, Key.WorldEpoch);
    MythicHarvestReceiptTypesPrivate::AddGuid(
        Bytes, Key.NodeId.GetGuid());
    MythicHarvestReceiptTypesPrivate::AddUInt32(Bytes, Key.Generation);
    if (!MythicHarvestReceiptTypesPrivate::AddUtf8(Bytes, ContributorKey)) {
        return Key;
    }
    const FBlake3Hash Digest = FBlake3::HashBuffer(
        Bytes.GetData(), Bytes.Num());
    FMemory::Memcpy(&Key.SeriesGuid, Digest.GetBytes(),
                    sizeof(Key.SeriesGuid));
    if (!Key.SeriesGuid.IsValid()) {
        Key.SeriesGuid.D = 1;
    }
    return Key;
}

FMythicHarvestReceiptKey FMythicHarvestReceiptKey::MakeDurabilityCost(
    const FGuid &InWorldEpoch, const FMythicHarvestNodeId &InNodeId,
    const uint32 InGeneration, const FString &ContributorKey,
    const FGuid &ToolItemInstanceGuid) {
    FMythicHarvestReceiptKey Key = MakeCompletion(
        InWorldEpoch, InNodeId, InGeneration,
        EMythicHarvestReceiptChannel::DurabilityCost);
    if (!Key.WorldEpoch.IsValid() || !Key.NodeId.IsValid()
        || Key.Generation == 0 || ContributorKey.IsEmpty()
        || !ToolItemInstanceGuid.IsValid()) {
        return Key;
    }

    TArray<uint8> Bytes;
    static constexpr ANSICHAR Domain[] =
        "MYTHIC_HARVEST_DURABILITY_COST_SERIES_V1";
    Bytes.Append(reinterpret_cast<const uint8 *>(Domain), sizeof(Domain));
    MythicHarvestReceiptTypesPrivate::AddGuid(Bytes, Key.WorldEpoch);
    MythicHarvestReceiptTypesPrivate::AddGuid(
        Bytes, Key.NodeId.GetGuid());
    MythicHarvestReceiptTypesPrivate::AddUInt32(Bytes, Key.Generation);
    if (!MythicHarvestReceiptTypesPrivate::AddUtf8(Bytes, ContributorKey)) {
        return Key;
    }
    MythicHarvestReceiptTypesPrivate::AddGuid(Bytes, ToolItemInstanceGuid);
    const FBlake3Hash Digest = FBlake3::HashBuffer(
        Bytes.GetData(), Bytes.Num());
    FMemory::Memcpy(&Key.SeriesGuid, Digest.GetBytes(),
                    sizeof(Key.SeriesGuid));
    if (!Key.SeriesGuid.IsValid()) {
        Key.SeriesGuid.D = 1;
    }
    return Key;
}

bool FMythicHarvestReceiptKey::IsValid() const {
    using namespace MythicHarvestReceiptTypesPrivate;
    if (!WorldEpoch.IsValid() || !NodeId.IsValid() || Generation == 0
        || !IsSupportedChannel(Channel)) {
        return false;
    }
    if (Channel == EMythicHarvestReceiptChannel::AppliedWorkProficiencyXP
        || Channel == EMythicHarvestReceiptChannel::DurabilityCost) {
        return EntryOrdinal == 0 && SeriesGuid.IsValid();
    }
    if (SeriesGuid.IsValid()) {
        return false;
    }
    if (Channel == EMythicHarvestReceiptChannel::CompletionProficiencyXP
        || Channel == EMythicHarvestReceiptChannel::CompletionQuestCredit) {
        return EntryOrdinal == 0;
    }
    return true;
}

bool FMythicHarvestReceiptQuantity::TryFromUnits(
    const double Units, int64 &OutQuanta) {
    OutQuanta = 0;
    if (!FMath::IsFinite(Units) || Units <= 0.0
        || Units > static_cast<double>(MAX_int64)
            / static_cast<double>(QuantaPerUnit)) {
        return false;
    }
    const double Scaled = Units * static_cast<double>(QuantaPerUnit);
    if (!FMath::IsFinite(Scaled) || Scaled < 0.5
        || Scaled > static_cast<double>(MAX_int64)) {
        return false;
    }
    OutQuanta = FMath::RoundToInt64(Scaled);
    return OutQuanta > 0;
}

double FMythicHarvestReceiptQuantity::ToUnits(const int64 Quanta) {
    return Quanta > 0
        ? static_cast<double>(Quanta) / static_cast<double>(QuantaPerUnit)
        : 0.0;
}

bool FMythicHarvestWorkRewardContract::IsUnset() const {
    return !bInitialized;
}

bool FMythicHarvestWorkRewardContract::IsValid() const {
    if (!bInitialized || ProficiencyXPPerWorkUnitQuanta < 0) {
        return false;
    }
    if (ProficiencyXPPerWorkUnitQuanta == 0) {
        return !ProficiencyDefinitionId.IsValid() && ContextTags.IsEmpty();
    }
    if (!ProficiencyDefinitionId.IsValid()) {
        return false;
    }
    for (const FGameplayTag &Tag : ContextTags) {
        if (!Tag.IsValid()) {
            return false;
        }
    }
    return true;
}

bool FMythicHarvestReceiptQuantity::TryCalculateCumulativeAppliedWorkXP(
    const int64 CumulativeAppliedWorkQuanta,
    const int64 ProficiencyXPPerWorkUnitQuanta,
    int64 &OutCumulativeXPQuanta) {
    OutCumulativeXPQuanta = 0;
    constexpr int64 WorkScale = FMythicHarvestWork::QuantaPerWorkUnit;
    if (CumulativeAppliedWorkQuanta <= 0
        || ProficiencyXPPerWorkUnitQuanta <= 0
        || ProficiencyXPPerWorkUnitQuanta > MAX_int64 / WorkScale) {
        return false;
    }

    const int64 WholeWorkUnits =
        CumulativeAppliedWorkQuanta / WorkScale;
    const int64 FractionalWorkQuanta =
        CumulativeAppliedWorkQuanta % WorkScale;
    if (WholeWorkUnits
        > MAX_int64 / ProficiencyXPPerWorkUnitQuanta) {
        return false;
    }
    const int64 WholeXP =
        WholeWorkUnits * ProficiencyXPPerWorkUnitQuanta;
    const int64 FractionProduct =
        FractionalWorkQuanta * ProficiencyXPPerWorkUnitQuanta;
    const int64 FractionXP =
        (FractionProduct + WorkScale / 2) / WorkScale;
    if (WholeXP > MAX_int64 - FractionXP) {
        return false;
    }
    OutCumulativeXPQuanta = WholeXP + FractionXP;
    return OutCumulativeXPQuanta > 0;
}

bool FMythicHarvestReceiptLedgerSaveV1::Validate(
    const FMythicHarvestReceiptLedgerSaveV1 &Snapshot,
    FName &OutDiagnosticCode, const int32 ConfiguredMaximumRows) {
    using namespace MythicHarvestReceiptTypesPrivate;
    const int32 EffectiveMaximum = FMath::Clamp(
        ConfiguredMaximumRows, 1, AbsoluteMaximumRows);
    if (Snapshot.SchemaVersion != CurrentSchemaVersion) {
        OutDiagnosticCode = TEXT("UnsupportedHarvestReceiptSchema");
        return false;
    }
    if (!Snapshot.LedgerEpoch.IsValid()) {
        OutDiagnosticCode = TEXT("InvalidHarvestReceiptLedgerEpoch");
        return false;
    }
    if (Snapshot.Rows.Num() > EffectiveMaximum) {
        OutDiagnosticCode = TEXT("HarvestReceiptCapacityExceeded");
        return false;
    }
    if (Snapshot.WorldWatermarks.Num() > AbsoluteMaximumWorldWatermarks) {
        OutDiagnosticCode = TEXT("HarvestReceiptWatermarkCapacityExceeded");
        return false;
    }

    TSet<FMythicHarvestReceiptKey> SeenKeys;
    SeenKeys.Reserve(Snapshot.Rows.Num());
    uint64 HighestMutationRevision = 0;
    for (const FMythicSavedHarvestReceiptRowV1 &Row : Snapshot.Rows) {
        const bool bQuest = Row.Key.Channel
            == EMythicHarvestReceiptChannel::CompletionQuestCredit;
        const bool bCumulative = Row.Key.Channel
                == EMythicHarvestReceiptChannel::AppliedWorkProficiencyXP
            || Row.Key.Channel
                == EMythicHarvestReceiptChannel::DurabilityCost;
        const bool bDispositionValid = bQuest
            ? ((Row.AppliedQuantity == 0
                    && Row.QuestDisposition
                        == EMythicHarvestQuestReceiptDisposition::None)
               || (Row.AppliedQuantity == Row.TargetQuantity
                   && (Row.QuestDisposition
                           == EMythicHarvestQuestReceiptDisposition::Matched
                       || Row.QuestDisposition
                           == EMythicHarvestQuestReceiptDisposition::NoMatch)))
            : Row.QuestDisposition
                == EMythicHarvestQuestReceiptDisposition::None;
        if (!Row.Key.IsValid() || !Row.PayloadFingerprint.IsValid()
            || Row.TargetQuantity <= 0 || Row.AppliedQuantity < 0
            || Row.AppliedQuantity > Row.TargetQuantity
            || (bCumulative
                && Row.AppliedQuantity != Row.TargetQuantity)
            || (Row.AppliedQuantity > 0 && Row.MutationRevision == 0)
            || Row.MutationRevision > Snapshot.LedgerRevision
            || !bDispositionValid || SeenKeys.Contains(Row.Key)) {
            OutDiagnosticCode = TEXT("InvalidSavedHarvestReceipt");
            return false;
        }
        SeenKeys.Add(Row.Key);
        HighestMutationRevision = FMath::Max(
            HighestMutationRevision, Row.MutationRevision);
    }
    if (HighestMutationRevision > Snapshot.LedgerRevision) {
        OutDiagnosticCode = TEXT("InvalidHarvestReceiptRevision");
        return false;
    }

    TSet<FGuid> SeenEpochs;
    for (const FMythicSavedHarvestReceiptWorldWatermarkV1 &Watermark :
         Snapshot.WorldWatermarks) {
        if (!Watermark.WorldEpoch.IsValid()
            || Watermark.MinimumAcceptedSnapshotSequence == 0
            || SeenEpochs.Contains(Watermark.WorldEpoch)) {
            OutDiagnosticCode = TEXT("InvalidHarvestReceiptWorldWatermark");
            return false;
        }
        SeenEpochs.Add(Watermark.WorldEpoch);
    }
    OutDiagnosticCode = NAME_None;
    return true;
}

void FMythicHarvestReceiptLedgerSaveV1::SortCanonical() {
    Rows.Sort([](const FMythicSavedHarvestReceiptRowV1 &Left,
                 const FMythicSavedHarvestReceiptRowV1 &Right) {
        return MythicHarvestReceiptTypesPrivate::KeyLess(Left.Key,
                                                         Right.Key);
    });
    WorldWatermarks.Sort([](
        const FMythicSavedHarvestReceiptWorldWatermarkV1 &Left,
        const FMythicSavedHarvestReceiptWorldWatermarkV1 &Right) {
        return MythicHarvestReceiptTypesPrivate::GuidLess(
            Left.WorldEpoch, Right.WorldEpoch);
    });
}

const FMythicSavedHarvestReceiptRowV1 *
FMythicHarvestReceiptLedgerSaveV1::FindRow(
    const FMythicHarvestReceiptKey &Key) const {
    return Rows.FindByPredicate(
        [&Key](const FMythicSavedHarvestReceiptRowV1 &Row) {
            return Row.Key == Key;
        });
}

FGuid FMythicHarvestReceiptFingerprint::Build(
    const FMythicHarvestReceiptKey &Key,
    const FPrimaryAssetId &TypedAssetId, const int64 TargetQuantity,
    const uint64 FrozenSeed, const uint32 FrozenAuxiliaryA,
    const uint32 FrozenAuxiliaryB,
    const FGameplayTagContainer &FrozenContextTags) {
    using namespace MythicHarvestReceiptTypesPrivate;
    if (!Key.IsValid() || !TypedAssetId.IsValid() || TargetQuantity <= 0) {
        return FGuid();
    }

    TArray<uint8> Bytes;
    static constexpr ANSICHAR Domain[] =
        "MYTHIC_HARVEST_RECEIPT_PAYLOAD_V1";
    Bytes.Append(reinterpret_cast<const uint8 *>(Domain), sizeof(Domain));
    AddGuid(Bytes, Key.WorldEpoch);
    AddGuid(Bytes, Key.NodeId.GetGuid());
    AddUInt32(Bytes, Key.Generation);
    Bytes.Add(static_cast<uint8>(Key.Channel));
    AddUInt32(Bytes, Key.EntryOrdinal);
    AddGuid(Bytes, Key.SeriesGuid);
    AddUInt64(Bytes, static_cast<uint64>(TargetQuantity));
    AddUInt64(Bytes, FrozenSeed);
    AddUInt32(Bytes, FrozenAuxiliaryA);
    AddUInt32(Bytes, FrozenAuxiliaryB);
    if (!AddUtf8(Bytes, TypedAssetId.PrimaryAssetType.ToString())
        || !AddUtf8(Bytes, TypedAssetId.PrimaryAssetName.ToString())) {
        return FGuid();
    }

    TArray<FGameplayTag> SortedTags;
    FrozenContextTags.GetGameplayTagArray(SortedTags);
    SortedTags.Sort([](const FGameplayTag &Left,
                       const FGameplayTag &Right) {
        return Left.GetTagName().LexicalLess(Right.GetTagName());
    });
    AddUInt32(Bytes, static_cast<uint32>(SortedTags.Num()));
    for (const FGameplayTag &Tag : SortedTags) {
        if (!Tag.IsValid() || !AddUtf8(Bytes, Tag.ToString())) {
            return FGuid();
        }
    }

    const FBlake3Hash Digest = FBlake3::HashBuffer(Bytes.GetData(),
                                                   Bytes.Num());
    const uint8 *HashBytes = Digest.GetBytes();
    FGuid Result;
    FMemory::Memcpy(&Result, HashBytes, sizeof(Result));
    if (!Result.IsValid()) {
        Result.D = 1;
    }
    return Result;
}

FGuid FMythicHarvestReceiptFingerprint::BuildAppliedWorkSeries(
    const FMythicHarvestReceiptKey &Key,
    const FPrimaryAssetId &ProficiencyDefinitionId,
    const int64 ProficiencyXPPerWorkUnitQuanta,
    const FGameplayTagContainer &FrozenContextTags) {
    if (Key.Channel
            != EMythicHarvestReceiptChannel::AppliedWorkProficiencyXP
        || ProficiencyXPPerWorkUnitQuanta <= 0) {
        return FGuid();
    }
    // Build already domain-separates all receipt identity and typed asset
    // fields. For a cumulative work series this fixed rate is the immutable
    // contract quantity; the live cumulative XP target may advance.
    return Build(Key, ProficiencyDefinitionId,
                 ProficiencyXPPerWorkUnitQuanta, 0, 0, 0,
                 FrozenContextTags);
}

FGuid FMythicHarvestReceiptFingerprint::BuildDurabilityCostSeries(
    const FMythicHarvestReceiptKey &Key,
    const FGuid &ToolItemInstanceGuid) {
    using namespace MythicHarvestReceiptTypesPrivate;
    if (!Key.IsValid()
        || Key.Channel != EMythicHarvestReceiptChannel::DurabilityCost
        || !ToolItemInstanceGuid.IsValid()) {
        return FGuid();
    }
    TArray<uint8> Bytes;
    static constexpr ANSICHAR Domain[] =
        "MYTHIC_HARVEST_DURABILITY_COST_PAYLOAD_V1";
    Bytes.Append(reinterpret_cast<const uint8 *>(Domain), sizeof(Domain));
    AddGuid(Bytes, Key.WorldEpoch);
    AddGuid(Bytes, Key.NodeId.GetGuid());
    AddUInt32(Bytes, Key.Generation);
    AddUInt32(Bytes, static_cast<uint32>(Key.Channel));
    AddGuid(Bytes, Key.SeriesGuid);
    AddGuid(Bytes, ToolItemInstanceGuid);
    const FBlake3Hash Digest = FBlake3::HashBuffer(
        Bytes.GetData(), Bytes.Num());
    FGuid Result;
    FMemory::Memcpy(&Result, Digest.GetBytes(), sizeof(Result));
    if (!Result.IsValid()) {
        Result.D = 1;
    }
    return Result;
}

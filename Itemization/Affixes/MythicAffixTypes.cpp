#include "Itemization/Affixes/MythicAffixTypes.h"

#include "Itemization/Affixes/MythicAffixDefinition.h"
#include "Itemization/Affixes/MythicAffixPool.h"
#include "Itemization/Affixes/MythicAffixProfile.h"
#include "Itemization/Affixes/MythicAffixRollPolicy.h"
#include "System/MythicAssetManager.h"

#include <limits>

const FGuid MythicAffixSerialization::RolledAffixMagic(
    0xA771F1A1, 0x5E944FA7, 0xA5211C39, 0x1180C2D4);

namespace {
template <typename AssetType>
FPrimaryAssetId ResolvePrimaryAssetId(const TSoftObjectPtr<AssetType> &Asset) {
    if (const AssetType *Loaded = Asset.Get()) {
        return Loaded->GetPrimaryAssetId();
    }
    const FSoftObjectPath Path = Asset.ToSoftObjectPath();
    if (Path.IsValid()) {
        if (const UAssetManager *AssetManager = UAssetManager::GetIfInitialized()) {
            return AssetManager->GetPrimaryAssetIdForPath(Path);
        }
    }
    return FPrimaryAssetId();
}

bool SerializeGameplayTag(FArchive &Ar, FGameplayTag &Tag) {
    FName Name = Ar.IsSaving() ? Tag.GetTagName() : NAME_None;
    Ar << Name;
    if (Ar.IsLoading()) {
        Tag = FGameplayTag::RequestGameplayTag(Name, false);
    }
    return !Ar.IsError() && Tag.IsValid();
}

bool SerializeSoftDefinition(FArchive &Ar, FMythicAffixDefinitionHandle &Handle) {
    FString Path = Ar.IsSaving() ? Handle.Asset.ToSoftObjectPath().ToString() : FString();
    Ar << Path;
    if (Ar.IsError() || Path.Len() > MythicAffixSerialization::MaxSoftPathCharacters) {
        Ar.SetError();
        return false;
    }
    if (Ar.IsLoading()) {
        Handle.Asset = TSoftObjectPtr<UMythicAffixDefinition>(FSoftObjectPath(Path));
    }
    return Handle.IsValid();
}

bool NetSerializeDefinition(FArchive &Ar, FMythicAffixDefinitionHandle &Handle) {
    FPrimaryAssetId DefinitionId = Ar.IsSaving() ? Handle.GetPrimaryAssetId() : FPrimaryAssetId();
    Ar << DefinitionId;
    if (Ar.IsLoading()) {
        const FSoftObjectPath Path = UMythicAssetManager::Get().GetPrimaryAssetPath(DefinitionId);
        Handle.Asset = Path.IsValid()
            ? TSoftObjectPtr<UMythicAffixDefinition>(Path)
            : TSoftObjectPtr<UMythicAffixDefinition>();
    }
    return !Ar.IsError() && Handle.IsValid();
}

bool NetSerializeGameplayTag(FArchive &Ar, UPackageMap *Map, FGameplayTag &Tag) {
    bool bTagSuccess = true;
    Tag.NetSerialize(Ar, Map, bTagSuccess);
    return bTagSuccess && !Ar.IsError() && Tag.IsValid();
}

bool NetSerializeNonNegativeInt(FArchive &Ar, int32 &Value) {
    uint32 Packed = Ar.IsSaving() && Value >= 0 ? static_cast<uint32>(Value) : 0;
    if (Ar.IsSaving() && Value < 0) {
        Ar.SetError();
        return false;
    }
    Ar.SerializeIntPacked(Packed);
    if (Ar.IsLoading()) {
        if (Packed > static_cast<uint32>(MAX_int32)) {
            Ar.SetError();
            return false;
        }
        Value = static_cast<int32>(Packed);
    }
    return !Ar.IsError();
}

void NotifyOwner(UObject *Owner) {
    if (Owner && Owner->GetClass()->ImplementsInterface(UMythicAffixSnapshotOwner::StaticClass())) {
        Cast<IMythicAffixSnapshotOwner>(Owner)->OnAffixSnapshotsReplicated();
    }
}
}

FPrimaryAssetId FMythicAffixDefinitionHandle::GetPrimaryAssetId() const {
    return ResolvePrimaryAssetId(Asset);
}
UMythicAffixDefinition *FMythicAffixDefinitionHandle::GetAsset() const { return Asset.Get(); }
void FMythicAffixDefinitionHandle::SetAsset(UMythicAffixDefinition *InAsset) { Asset = InAsset; }

FPrimaryAssetId FMythicAffixPoolHandle::GetPrimaryAssetId() const { return ResolvePrimaryAssetId(Asset); }
UMythicAffixPool *FMythicAffixPoolHandle::GetAsset() const { return Asset.Get(); }
void FMythicAffixPoolHandle::SetAsset(UMythicAffixPool *InAsset) { Asset = InAsset; }

FPrimaryAssetId FMythicAffixRollPolicyHandle::GetPrimaryAssetId() const {
    return ResolvePrimaryAssetId(Asset);
}
UMythicAffixRollPolicy *FMythicAffixRollPolicyHandle::GetAsset() const { return Asset.Get(); }
void FMythicAffixRollPolicyHandle::SetAsset(UMythicAffixRollPolicy *InAsset) { Asset = InAsset; }

FPrimaryAssetId FMythicAffixProfileHandle::GetPrimaryAssetId() const { return ResolvePrimaryAssetId(Asset); }
UMythicAffixProfile *FMythicAffixProfileHandle::GetAsset() const { return Asset.Get(); }
void FMythicAffixProfileHandle::SetAsset(UMythicAffixProfile *InAsset) { Asset = InAsset; }

bool FMythicAffixQuantization::IsValid() const {
    switch (Mode) {
    case EMythicAffixQuantizationMode::None:
    case EMythicAffixQuantizationMode::WholeNumber:
        return true;
    case EMythicAffixQuantizationMode::Step:
        return FMath::IsFinite(Step) && Step > 0.0f;
    default:
        return false;
    }
}

float FMythicAffixQuantization::Apply(const float Value) const {
    if (!FMath::IsFinite(Value) || !IsValid()) {
        return std::numeric_limits<float>::quiet_NaN();
    }
    switch (Mode) {
    case EMythicAffixQuantizationMode::None:
        return Value;
    case EMythicAffixQuantizationMode::WholeNumber: {
        const float Rounded = FMath::RoundHalfFromZero(Value);
        return Rounded == 0.0f ? 0.0f : Rounded;
    }
    case EMythicAffixQuantizationMode::Step: {
        const float StepUnits = Value / Step;
        if (!FMath::IsFinite(StepUnits)) {
            return std::numeric_limits<float>::quiet_NaN();
        }
        const float Rounded = FMath::RoundHalfFromZero(StepUnits) * Step;
        if (!FMath::IsFinite(Rounded)) {
            return std::numeric_limits<float>::quiet_NaN();
        }
        // Canonical positive zero prevents the same semantic roll acquiring different serialized bit patterns.
        return Rounded == 0.0f ? 0.0f : Rounded;
    }
    default:
        return std::numeric_limits<float>::quiet_NaN();
    }
}

bool FRolledAffix::IsGameplayValid() const {
    return RollGuid.IsValid() && AffixDefinition.IsValid() && TierRank > 0
        && FMath::IsFinite(Magnitude) && Provenance.SourceKind.IsValid()
        && Provenance.GeneratedItemLevel > 0 && Provenance.AlgorithmVersion > 0;
}

bool FRolledAffix::Serialize(FArchive &Ar) {
    FGuid Magic = MythicAffixSerialization::RolledAffixMagic;
    int32 Version = MythicAffixSerialization::RolledAffixVersion;
    Ar << Magic;
    Ar << Version;
    if (Ar.IsLoading()
        && (Magic != MythicAffixSerialization::RolledAffixMagic
            || Version != MythicAffixSerialization::RolledAffixVersion)) {
        Ar.SetError();
        return false;
    }

    Ar << RollGuid;
    if (!SerializeSoftDefinition(Ar, AffixDefinition)) {
        return false;
    }
    Ar << TierRank;
    Ar << Magnitude;
    Ar << Provenance.ProfileId;
    Ar << Provenance.PolicyId;
    Ar << Provenance.PoolId;
    if (!SerializeGameplayTag(Ar, Provenance.RollGroup)
        || !SerializeGameplayTag(Ar, Provenance.SourceKind)) {
        return false;
    }
    Ar << Provenance.ProfileRevision;
    Ar << Provenance.PolicyRevision;
    Ar << Provenance.PoolRevision;
    Ar << Provenance.PoolRowRevision;
    Ar << Provenance.DefinitionRevision;
    Ar << Provenance.OriginGrantGuid;
    Ar << Provenance.OriginSliceGuid;
    Ar << Provenance.OriginPoolRowGuid;
    Ar << Provenance.OriginSocketGuid;
    Ar << Provenance.SourceItemGuid;
    Ar << Provenance.GameplayContentHash.Word0;
    Ar << Provenance.GameplayContentHash.Word1;
    Ar << Provenance.GameplayContentHash.Word2;
    Ar << Provenance.GameplayContentHash.Word3;
    Ar << Provenance.MutationRevision;
    Ar << Provenance.GeneratedItemLevel;
    uint8 Rarity = static_cast<uint8>(Provenance.GeneratedRarity.GetValue());
    Ar << Rarity;
    if (Ar.IsLoading()) Provenance.GeneratedRarity = static_cast<EItemRarity>(Rarity);
    Ar << Provenance.AlgorithmVersion;
    Ar << bIsLocked;

    if (Ar.IsError() || (Ar.IsLoading() && !IsGameplayValid())) {
        Ar.SetError();
        return false;
    }
    return true;
}

bool FRolledAffix::NetSerialize(FArchive &Ar, UPackageMap *Map, bool &bOutSuccess) {
    bOutSuccess = false;
    if (Ar.IsLoading()) {
        *this = FRolledAffix();
    }
    Ar << RollGuid;
    if (!NetSerializeDefinition(Ar, AffixDefinition)) {
        return true;
    }
    if (!NetSerializeNonNegativeInt(Ar, TierRank)) {
        return true;
    }
    Ar << Magnitude;
    if (!NetSerializeGameplayTag(Ar, Map, Provenance.RollGroup)
        || !NetSerializeGameplayTag(Ar, Map, Provenance.SourceKind)) {
        return true;
    }
    Ar << Provenance.OriginSocketGuid;
    Ar << Provenance.SourceItemGuid;
    if (!NetSerializeNonNegativeInt(Ar, Provenance.MutationRevision)
        || !NetSerializeNonNegativeInt(Ar, Provenance.GeneratedItemLevel)) {
        return true;
    }
    uint8 Rarity = static_cast<uint8>(Provenance.GeneratedRarity.GetValue());
    Ar << Rarity;
    if (Ar.IsLoading()) {
        Provenance.GeneratedRarity = static_cast<EItemRarity>(Rarity);
        Provenance.AlgorithmVersion = 1;
    }
    Ar << bIsLocked;
    bOutSuccess = !Ar.IsError() && IsGameplayValid();
    return true;
}

bool FMythicReplicatedAffixArray::Serialize(FArchive &Ar) {
    int32 Count = Items.Num();
    Ar << Count;
    if (Count < 0 || Count > MythicAffixSerialization::MaxAffixesPerContainer) {
        Ar.SetError();
        return false;
    }
    if (Ar.IsLoading()) {
        Items.SetNum(Count);
    }
    for (FMythicReplicatedAffixItem &Item : Items) {
        if (!Item.Affix.Serialize(Ar)) {
            Ar.SetError();
            return false;
        }
    }
    return !Ar.IsError();
}

void FMythicReplicatedAffixArray::PostReplicatedAdd(const TArrayView<int32> &, int32) {
    NotifyOwner(Owner.Get());
}
void FMythicReplicatedAffixArray::PostReplicatedChange(const TArrayView<int32> &, int32) {
    NotifyOwner(Owner.Get());
}
void FMythicReplicatedAffixArray::PreReplicatedRemove(const TArrayView<int32> &, int32) {
    NotifyOwner(Owner.Get());
}

void FMythicReplicatedAffixArray::ReplaceAll(TArray<FRolledAffix> &&Snapshots) {
    Items.Reset(Snapshots.Num());
    for (FRolledAffix &Snapshot : Snapshots) {
        FMythicReplicatedAffixItem &Item = Items.AddDefaulted_GetRef();
        Item.Affix = MoveTemp(Snapshot);
        MarkItemDirty(Item);
    }
    MarkArrayDirty();
}

void FMythicReplicatedAffixArray::GetSnapshots(TArray<FRolledAffix> &Out) const {
    Out.Reset(Items.Num());
    for (const FMythicReplicatedAffixItem &Item : Items) {
        Out.Add(Item.Affix);
    }
}

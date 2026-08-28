#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "GameplayTagsManager.h"
#include "Itemization/Affixes/MythicAffixDefinition.h"
#include "Itemization/Affixes/MythicAffixTypes.h"
#include "Itemization/Affixes/MythicTags_Affixes.h"
#include "Serialization/BufferArchive.h"
#include "Serialization/MemoryReader.h"

namespace {
UMythicAffixDefinition *MakeSerializationDefinition() {
    UMythicAffixDefinition *Definition = NewObject<UMythicAffixDefinition>(
        GetTransientPackage());
    Definition->AffixTag = UGameplayTagsManager::Get().RequestGameplayTag(
        FName(TEXT("Itemization.Affix.Power")), true);
    return Definition;
}

FRolledAffix MakeCurrentSnapshot(UMythicAffixDefinition *Definition,
                                 const FGuid RollGuid, const float Magnitude) {
    FRolledAffix Snapshot;
    Snapshot.RollGuid = RollGuid;
    Snapshot.AffixDefinition.SetAsset(Definition);
    Snapshot.TierRank = 2;
    Snapshot.Magnitude = Magnitude;
    Snapshot.Provenance.ProfileId = FPrimaryAssetId(
        FPrimaryAssetType(TEXT("AffixProfile")),
        FName(TEXT("Itemization.AffixProfile.Weapon")));
    Snapshot.Provenance.PolicyId = FPrimaryAssetId(
        FPrimaryAssetType(TEXT("AffixRollPolicy")),
        FName(TEXT("Itemization.AffixRollPolicy.Default")));
    Snapshot.Provenance.PoolId = FPrimaryAssetId(
        FPrimaryAssetType(TEXT("AffixPool")),
        FName(TEXT("Itemization.AffixPool.Weapon")));
    Snapshot.Provenance.RollGroup = AFFIX_ROLL_GROUP_PREFIX;
    Snapshot.Provenance.SourceKind = AFFIX_SOURCE_EXPLICIT;
    Snapshot.Provenance.ProfileRevision = 3;
    Snapshot.Provenance.PolicyRevision = 4;
    Snapshot.Provenance.PoolRevision = 5;
    Snapshot.Provenance.PoolRowRevision = 6;
    Snapshot.Provenance.DefinitionRevision = 7;
    Snapshot.Provenance.OriginSliceGuid = FGuid(10, 11, 12, 13);
    Snapshot.Provenance.OriginPoolRowGuid = FGuid(14, 15, 16, 17);
    Snapshot.Provenance.SourceItemGuid = FGuid(18, 19, 20, 21);
    Snapshot.Provenance.GameplayContentHash.Word0 = 22;
    Snapshot.Provenance.GameplayContentHash.Word1 = 23;
    Snapshot.Provenance.MutationRevision = 8;
    Snapshot.Provenance.GeneratedItemLevel = 42;
    Snapshot.Provenance.GeneratedRarity = EItemRarity::Legendary;
    Snapshot.Provenance.AlgorithmVersion = 1;
    Snapshot.bIsLocked = true;
    return Snapshot;
}

bool SaveSnapshot(FRolledAffix &Snapshot, TArray<uint8> &OutBytes) {
    FBufferArchive Archive;
    Archive.ArIsSaveGame = true;
    const bool bSuccess = Snapshot.Serialize(Archive) && !Archive.IsError();
    OutBytes.Reset();
    OutBytes.Append(Archive.GetData(), Archive.Num());
    return bSuccess;
}

bool LoadSnapshot(TConstArrayView<uint8> Bytes, FRolledAffix &OutSnapshot) {
    TArray<uint8> Copy;
    Copy.Append(Bytes.GetData(), Bytes.Num());
    FMemoryReader Archive(Copy, true);
    Archive.ArIsSaveGame = true;
    return OutSnapshot.Serialize(Archive) && !Archive.IsError()
        && Archive.Tell() == Archive.TotalSize();
}
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicCurrentAffixSerializationHardeningTest,
    "Mythic.Itemization.Save.CurrentAffixSnapshot",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicCurrentAffixSerializationHardeningTest::RunTest(const FString &Parameters) {
    UMythicAffixDefinition *Definition = MakeSerializationDefinition();
    FRolledAffix Source = MakeCurrentSnapshot(
        Definition, FGuid(1, 2, 3, 4), 17.25f);
    TArray<uint8> Buffer;
    TestTrue(TEXT("current snapshot writes its framed SaveGame format"),
             SaveSnapshot(Source, Buffer));

    FRolledAffix Restored;
    TestTrue(TEXT("current snapshot reads its exact framed format"),
             LoadSnapshot(Buffer, Restored));
    TestEqual(TEXT("RollGuid roundtrips"), Restored.RollGuid, Source.RollGuid);
    TestEqual(TEXT("direct Definition reference roundtrips"),
              Restored.AffixDefinition.GetAsset(), Definition);
    TestEqual(TEXT("derived tier rank roundtrips"), Restored.TierRank, 2);
    TestEqual(TEXT("singular magnitude roundtrips"), Restored.Magnitude, 17.25f);
    TestEqual(TEXT("source-item audit identity roundtrips"),
              Restored.Provenance.SourceItemGuid,
              Source.Provenance.SourceItemGuid);
    TestEqual(TEXT("gameplay content hash roundtrips"),
              Restored.Provenance.GameplayContentHash.Word1, uint64(23));
    TestEqual(TEXT("generated item level roundtrips"),
              Restored.Provenance.GeneratedItemLevel, 42);
    TestTrue(TEXT("crafting lock roundtrips"), Restored.bIsLocked);
    TestTrue(TEXT("restored current snapshot is gameplay-valid"),
             Restored.IsGameplayValid());

    FBufferArchive UnknownVersion;
    UnknownVersion.ArIsSaveGame = true;
    FGuid Marker = MythicAffixSerialization::RolledAffixMagic;
    int32 Version = MythicAffixSerialization::RolledAffixVersion + 1;
    UnknownVersion << Marker;
    UnknownVersion << Version;
    FRolledAffix Unknown;
    TestFalse(TEXT("unknown framed versions fail closed"),
              LoadSnapshot(UnknownVersion, Unknown));

    TArray<uint8> Truncated = Buffer;
    Truncated.SetNum(Truncated.Num() / 2);
    FRolledAffix TruncatedResult;
    TestFalse(TEXT("truncated current snapshots fail atomically"),
              LoadSnapshot(Truncated, TruncatedResult));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicCurrentAffixArraySerializationHardeningTest,
    "Mythic.Itemization.Save.CurrentAffixArray",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicCurrentAffixArraySerializationHardeningTest::RunTest(const FString &Parameters) {
    UMythicAffixDefinition *Definition = MakeSerializationDefinition();
    TArray<FRolledAffix> Snapshots;
    Snapshots.Add(MakeCurrentSnapshot(Definition, FGuid(1, 1, 1, 1), 5.0f));
    Snapshots.Add(MakeCurrentSnapshot(Definition, FGuid(2, 2, 2, 2), 9.0f));
    FMythicReplicatedAffixArray Source;
    Source.ReplaceAll(MoveTemp(Snapshots));

    FBufferArchive Buffer;
    Buffer.ArIsSaveGame = true;
    TestTrue(TEXT("current Fast Array writes every immutable snapshot"),
             Source.Serialize(Buffer));
    TestFalse(TEXT("current Fast Array save has no archive error"), Buffer.IsError());

    FMythicReplicatedAffixArray Restored;
    FMemoryReader Reader(Buffer, true);
    Reader.ArIsSaveGame = true;
    TestTrue(TEXT("current Fast Array reads every immutable snapshot"),
             Restored.Serialize(Reader));
    TestFalse(TEXT("current Fast Array load has no archive error"), Reader.IsError());
    TestEqual(TEXT("array cardinality roundtrips"), Restored.Items.Num(), 2);
    if (Restored.Items.Num() == 2) {
        TestEqual(TEXT("first singular magnitude roundtrips"),
                  Restored.Items[0].Affix.Magnitude, 5.0f);
        TestEqual(TEXT("second direct Definition reference roundtrips"),
                  Restored.Items[1].Affix.AffixDefinition.GetAsset(), Definition);
    }

    FBufferArchive OversizedBuffer;
    OversizedBuffer.ArIsSaveGame = true;
    int32 OversizedCount = MythicAffixSerialization::MaxAffixesPerContainer + 1;
    OversizedBuffer << OversizedCount;
    FMythicReplicatedAffixArray OversizedResult;
    FMemoryReader OversizedReader(OversizedBuffer, true);
    OversizedReader.ArIsSaveGame = true;
    TestFalse(TEXT("oversized current arrays fail before allocation"),
              OversizedResult.Serialize(OversizedReader));
    TestTrue(TEXT("oversized current arrays mark the archive invalid"),
             OversizedReader.IsError());
    return true;
}

#endif

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


    // Exercise save-load publication bookkeeping, not a simulated network connection.
    // Actual owner-only payload delivery is covered by the isolated multiplayer receipt.
    if (!TestEqual(TEXT("bookkeeping fixture has both restored rows"), Restored.Items.Num(), 2)) return false;
    Restored.SetOwner(Definition);
    TSet<int32> OriginalIds;
    for (const FMythicReplicatedAffixItem &Row : Restored.Items) {
        TestTrue(TEXT("restored affix gets a fresh network identity"), Row.ReplicationID != INDEX_NONE);
        TestTrue(TEXT("restored affix gets an initial row key"), Row.ReplicationKey != INDEX_NONE);
        OriginalIds.Add(Row.ReplicationID);
    }
    TestEqual(TEXT("restored network identities are unique"), OriginalIds.Num(), 2);
    TestTrue(TEXT("restored array differs from the empty default delta state"), Restored.ArrayReplicationKey > 0);
    const auto WriteArray = [](FMythicReplicatedAffixArray &Array, TArray<uint8> &Bytes) {
        FBufferArchive Out; Out.ArIsSaveGame = true;
        const bool bResult = Array.Serialize(Out) && !Out.IsError();
        Bytes = Out;
        return bResult;
    };
    const auto ReadArray = [](TArray<uint8> &Bytes, FMythicReplicatedAffixArray &Array) {
        FMemoryReader In(Bytes, true); In.ArIsSaveGame = true;
        return Array.Serialize(In) && !In.IsError() && In.AtEnd();
    };
    TArray<uint8> Resaved;
    TestTrue(TEXT("rehydrated affixes save without errors"), WriteArray(Restored, Resaved));
    TestTrue(TEXT("new replication metadata does not change canonical save bytes"), Resaved == Buffer);

    // Same cardinality and same persistent roll GUIDs must still replace changed payload.
    const int32 BeforeReloadKey = Restored.ArrayReplicationKey;
    const int32 BeforeReloadCounter = Restored.IDCounter;
    Source.Items[0].Affix.Magnitude = 13.0f;
    Source.Items[1].Affix.Magnitude = 17.0f;
    Source.MarkItemDirty(Source.Items[0]);
    Source.MarkItemDirty(Source.Items[1]);
    TArray<uint8> ReplacementBytes;
    if (!TestTrue(TEXT("same-size replacement saves"), WriteArray(Source, ReplacementBytes))
        || !TestTrue(TEXT("same-size replacement loads"), ReadArray(ReplacementBytes, Restored))
        || !TestEqual(TEXT("same-size replacement has both rows"), Restored.Items.Num(), 2)) return false;
    TestTrue(TEXT("same-size reload advances the array key"), Restored.ArrayReplicationKey > BeforeReloadKey);
    TestTrue(TEXT("same-size reload preserves monotonic ID allocation"), Restored.IDCounter > BeforeReloadCounter);
    for (const FMythicReplicatedAffixItem &Row : Restored.Items) {
        TestTrue(TEXT("replacement cannot reuse a prior connection's row identity"),
                 Row.ReplicationID != INDEX_NONE && !OriginalIds.Contains(Row.ReplicationID));
    }
    TestEqual(TEXT("same-count first magnitude changes"), Restored.Items[0].Affix.Magnitude, 13.0f);
    TestEqual(TEXT("same-count second magnitude changes"), Restored.Items[1].Affix.Magnitude, 17.0f);
    TestTrue(TEXT("successful reload preserves the runtime callback owner"), Restored.Owner.Get() == Definition);

    // First row is valid but different; the second framed row has an unsupported version.
    // This fails without archive overflow and must not publish the already-read first row.
    FBufferArchive Malformed;
    Malformed.ArIsSaveGame = true;
    int32 TwoRows = 2; Malformed << TwoRows;
    FRolledAffix FirstStaged = Source.Items[0].Affix;
    FirstStaged.Magnitude = 999.0f;
    if (!TestTrue(TEXT("malformed fixture contains a complete valid first row"), FirstStaged.Serialize(Malformed))) return false;
    FGuid RowMagic = MythicAffixSerialization::RolledAffixMagic;
    int32 BadVersion = MythicAffixSerialization::RolledAffixVersion + 1;
    Malformed << RowMagic; Malformed << BadVersion;
    const int32 BeforeFailureKey = Restored.ArrayReplicationKey;
    const int32 BeforeFailureCounter = Restored.IDCounter;
    TArray<int32> BeforeFailureIds;
    TArray<int32> BeforeFailureRowKeys;
    for (const auto &Row : Restored.Items) { BeforeFailureIds.Add(Row.ReplicationID); BeforeFailureRowKeys.Add(Row.ReplicationKey); }
    TArray<uint8> BeforeFailureBytes;
    if (!TestTrue(TEXT("capture prior valid affix save"), WriteArray(Restored, BeforeFailureBytes))) return false;
    TestFalse(TEXT("malformed second row rejects the complete replacement"), ReadArray(Malformed, Restored));
    TArray<uint8> AfterFailureBytes;
    TestTrue(TEXT("prior affixes still save after rejected load"), WriteArray(Restored, AfterFailureBytes));
    TestTrue(TEXT("failed staged read leaves every saved field unchanged"), BeforeFailureBytes == AfterFailureBytes);
    TestEqual(TEXT("failed read leaves array key unchanged"), Restored.ArrayReplicationKey, BeforeFailureKey);
    TestEqual(TEXT("failed read leaves ID counter unchanged"), Restored.IDCounter, BeforeFailureCounter);
    if (!TestEqual(TEXT("failed read keeps row count"), Restored.Items.Num(), BeforeFailureIds.Num())) return false;
    for (int32 Index = 0; Index < Restored.Items.Num(); ++Index) {
        TestEqual(TEXT("failed read preserves row identity"), Restored.Items[Index].ReplicationID, BeforeFailureIds[Index]);
        TestEqual(TEXT("failed read preserves row key"), Restored.Items[Index].ReplicationKey, BeforeFailureRowKeys[Index]);
    }
    TestTrue(TEXT("failed read preserves callback owner"), Restored.Owner.Get() == Definition);

    FMythicReplicatedAffixArray Empty;
    TArray<uint8> EmptyBytes;
    const int32 BeforeEmptyKey = Restored.ArrayReplicationKey;
    const int32 BeforeEmptyCounter = Restored.IDCounter;
    if (!TestTrue(TEXT("empty array saves"), WriteArray(Empty, EmptyBytes))
        || !TestTrue(TEXT("empty replacement loads"), ReadArray(EmptyBytes, Restored))) return false;
    TestTrue(TEXT("empty load removes every old row"), Restored.Items.IsEmpty());
    TestTrue(TEXT("empty load advertises deletion to prior delta state"), Restored.ArrayReplicationKey > BeforeEmptyKey);
    TestEqual(TEXT("empty load does not rewind ID allocation"), Restored.IDCounter, BeforeEmptyCounter);
    TestTrue(TEXT("empty load preserves callback owner"), Restored.Owner.Get() == Definition);

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

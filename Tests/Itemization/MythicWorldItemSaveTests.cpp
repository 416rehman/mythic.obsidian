#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Loot/MythicWorldItem.h"
#include "Misc/ScopeExit.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

namespace {
const FGuid TestWorldItemFrameMagic(0x4D59574C, 0x44495445, 0x4D465231, 0x8A931C57);

struct FParsedWorldItemFrame {
    FGuid SaveGuid;
    FString StableContainerId;
    FString ItemClassPath;
    int32 HashOffset = INDEX_NONE;
    int32 PayloadOffset = INDEX_NONE;
    int32 PayloadSize = 0;
};

bool ReadBoundedAscii(FArchive &Archive, FString &OutValue) {
    int32 Count = 0;
    Archive << Count;
    if (Archive.IsError() || Count <= 0 || Count > 4096
        || Archive.TotalSize() - Archive.Tell() < Count) return false;
    TArray<ANSICHAR> Bytes;
    Bytes.SetNumUninitialized(Count + 1);
    Archive.Serialize(Bytes.GetData(), Count);
    Bytes[Count] = 0;
    OutValue = UTF8_TO_TCHAR(Bytes.GetData());
    return !Archive.IsError() && !OutValue.IsEmpty();
}

bool ParseFrame(const TArray<uint8> &Bytes, FParsedWorldItemFrame &Out) {
    FMemoryReader Reader(Bytes, true);
    FGuid Magic;
    int32 Version = 0;
    Reader << Magic;
    Reader << Version;
    Reader << Out.SaveGuid;
    if (Magic != TestWorldItemFrameMagic || Version != 1 || !Out.SaveGuid.IsValid()
        || !ReadBoundedAscii(Reader, Out.StableContainerId)
        || !ReadBoundedAscii(Reader, Out.ItemClassPath)) return false;
    Reader << Out.PayloadSize;
    Out.HashOffset = static_cast<int32>(Reader.Tell());
    Reader.Seek(Reader.Tell() + 32);
    Out.PayloadOffset = static_cast<int32>(Reader.Tell());
    return !Reader.IsError() && Out.PayloadSize > 0
        && Out.PayloadOffset + Out.PayloadSize == Bytes.Num();
}

struct FWorldItemSaveFixture {
    UGameInstance *GameInstance = nullptr;
    UWorld *World = nullptr;
    UItemDefinition *Definition = nullptr;

    bool Initialize(FAutomationTestBase &Test) {
        if (!Test.TestNotNull(TEXT("engine is available"), GEngine)) return false;
        GameInstance = NewObject<UGameInstance>(GEngine);
        GameInstance->InitializeStandalone();
        World = GameInstance->GetWorld();
        if (!Test.TestNotNull(TEXT("standalone world exists"), World)) return false;
        Definition = NewObject<UItemDefinition>(GameInstance);
        return Test.TestNotNull(TEXT("loaded item definition fixture exists"), Definition);
    }

    AMythicWorldItem *SpawnItem(FAutomationTestBase &Test, const bool bWithItem = true) const {
        AMythicWorldItem *Actor = World ? World->SpawnActor<AMythicWorldItem>() : nullptr;
        if (!Test.TestNotNull(TEXT("world item actor spawned"), Actor) || !bWithItem) return Actor;
        UMythicItemInstance *Item = NewObject<UMythicItemInstance>(Actor);
        Item->SetOwner(Actor);
        Item->InitializeFixtureForTests(Definition, 1, 37);
        Actor->SetItemInstance(Item);
        if (!Test.TestTrue(TEXT("fixture item received a stable identity"),
                           Item->GetItemInstanceGuid().IsValid())) return nullptr;
        return Actor;
    }

    void Shutdown() {
        if (GameInstance) GameInstance->Shutdown();
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicWorldItemFramedSaveTest,
    "Mythic.Itemization.Save.WorldItemFramingIsTransactional",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicWorldItemFramedSaveTest::RunTest(const FString &Parameters) {
    FWorldItemSaveFixture Fixture;
    ON_SCOPE_EXIT { Fixture.Shutdown(); };
    if (!Fixture.Initialize(*this)) return false;

    AMythicWorldItem *Source = Fixture.SpawnItem(*this);
    AMythicWorldItem *Target = Fixture.SpawnItem(*this);
    if (!Source || !Target) return false;
    UMythicItemInstance *Sentinel = Target->ItemInstance;

    TArray<uint8> Framed;
    Source->SerializeCustomData(Framed);
    FParsedWorldItemFrame Parsed;
    if (!TestTrue(TEXT("world-item custom data has a bounded versioned frame"), ParseFrame(Framed, Parsed))) {
        return false;
    }

    TestTrue(TEXT("a complete framed payload restores"), Target->TryDeserializeCustomData(Framed));
    TestNotEqual(TEXT("restore publishes a staged replacement only after validation"), Target->ItemInstance, Sentinel);
    TestEqual(TEXT("stable item identity roundtrips"), Target->ItemInstance->GetItemInstanceGuid(),
              Source->ItemInstance->GetItemInstanceGuid());
    TestEqual(TEXT("runtime world-drop identity follows its persisted frame"),
              Target->GetSaveableActorId(), Source->GetSaveableActorId());

    AMythicWorldItem *CorruptTarget = Fixture.SpawnItem(*this);
    UMythicItemInstance *CorruptSentinel = CorruptTarget ? CorruptTarget->ItemInstance : nullptr;
    if (!CorruptTarget || !CorruptSentinel) return false;
    TArray<uint8> Corrupt = Framed;
    Corrupt.Last() ^= 0x5a;
    AddExpectedError(TEXT("nested item checksum mismatch"), EAutomationExpectedErrorFlags::Contains, 1);
    TestFalse(TEXT("checksum corruption rejects the entire item"),
              CorruptTarget->TryDeserializeCustomData(Corrupt));
    TestEqual(TEXT("failed restore leaves the published item untouched"),
              CorruptTarget->ItemInstance, CorruptSentinel);

    AMythicWorldItem *VersionTarget = Fixture.SpawnItem(*this);
    UMythicItemInstance *VersionSentinel = VersionTarget ? VersionTarget->ItemInstance : nullptr;
    if (!VersionTarget || !VersionSentinel) return false;
    TArray<uint8> UnknownVersion = Framed;
    const int32 UnsupportedVersion = 999;
    FMemory::Memcpy(UnknownVersion.GetData() + 16, &UnsupportedVersion, sizeof(UnsupportedVersion));
    AddExpectedError(TEXT("invalid/unsupported framed header version"),
                     EAutomationExpectedErrorFlags::Contains, 1);
    TestFalse(TEXT("an unknown framed version fails closed"),
              VersionTarget->TryDeserializeCustomData(UnknownVersion));
    TestEqual(TEXT("unknown-version failure is also non-destructive"),
              VersionTarget->ItemInstance, VersionSentinel);
    return true;
}

#endif

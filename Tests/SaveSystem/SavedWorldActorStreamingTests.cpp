#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Engine/Level.h"
#include "Engine/World.h"
#include "Subsystem/SaveSystem/World/MythicSaveableActor.h"
#include "Subsystem/SaveSystem/World/SavedWorldActor.h"
#include "World/Interactables/MythicToggleable.h"

namespace {

class FScopedSavedWorldActorWorld final {
public:
    FScopedSavedWorldActorWorld() {
        InitializationValues = UWorld::InitializationValues()
                                   .CreatePhysicsScene(false)
                                   .ShouldSimulatePhysics(false)
                                   .EnableTraceCollision(false)
                                   .CreateNavigation(false)
                                   .CreateAISystem(false);
        World = UWorld::CreateWorld(
            EWorldType::PIE, false,
            MakeUniqueObjectName(nullptr, UWorld::StaticClass(),
                                 TEXT("SavedWorldActorStreamingTest")),
            nullptr, true, ERHIFeatureLevel::Num, &InitializationValues,
            true);
        if (World) {
            World->SetPlayInEditorInitialNetMode(NM_ListenServer);
            World->InitWorld(InitializationValues);
        }
    }

    ~FScopedSavedWorldActorWorld() {
        if (World) {
            World->DestroyWorld(false);
        }
    }

    UWorld *Get() const { return World; }

private:
    UWorld::InitializationValues InitializationValues;
    UWorld *World = nullptr;
};

FSerializedWorldActorData MakePlacedRecord(const FString &ActorId) {
    FSerializedWorldActorData Record;
    Record.ActorId = ActorId;
    Record.ActorClass = FSoftClassPath(AMythicToggleable::StaticClass());
    Record.Transform = FTransform::Identity;
    Record.bWasRuntimeSpawned = false;
    return Record;
}

} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSavedWorldActorNonResidentPlacedRecordTest,
    "Mythic.SaveSystem.WorldActors.NonResidentPlacedRecordDefers",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FSavedWorldActorNonResidentPlacedRecordTest::RunTest(
    const FString & /*Parameters*/) {
    FScopedSavedWorldActorWorld Fixture;
    UWorld *World = Fixture.Get();
    if (!World) {
        AddError(TEXT("the streaming fixture could not create a world"));
        return false;
    }

    TArray<FSerializedWorldActorData> Records;
    Records.Add(MakePlacedRecord(TEXT("/Game/Levels/IslandMap_WP.IslandMap_WP:PersistentLevel.UnresidentPlot")));

    FName DiagnosticCode = TEXT("Unset");
    TestTrue(TEXT("a placed record whose World Partition cell is not resident passes preflight"),
             FSerializedWorldActorHelper::PreflightDeserialize(World, Records, DiagnosticCode));
    TestTrue(TEXT("an accepted preflight reports no diagnostic"), DiagnosticCode.IsNone());

    TArray<FSerializedWorldActorData> Deferred;
    TestTrue(TEXT("restore accepts the domain"),
             FSerializedWorldActorHelper::DeserializeAll(World, Records, Deferred));
    TestEqual(TEXT("the non-resident placed record is deferred, not dropped"), Deferred.Num(), 1);
    if (Deferred.Num() == 1) {
        TestEqual(TEXT("the deferred record keeps its exact identity"),
                  Deferred[0].ActorId, Records[0].ActorId);
    }

    // The same domain must still reject what it always rejected.
    TArray<FSerializedWorldActorData> DuplicateRecords;
    DuplicateRecords.Add(MakePlacedRecord(TEXT("/Game/Levels/IslandMap_WP.IslandMap_WP:PersistentLevel.Duplicated")));
    DuplicateRecords.Add(MakePlacedRecord(TEXT("/Game/Levels/IslandMap_WP.IslandMap_WP:PersistentLevel.Duplicated")));

    DiagnosticCode = NAME_None;
    TestFalse(TEXT("two records claiming one identity still reject"),
              FSerializedWorldActorHelper::PreflightDeserialize(World, DuplicateRecords, DiagnosticCode));
    TestEqual(TEXT("the duplicate rejection names its cause"),
              DiagnosticCode, FName(TEXT("DuplicateSavedWorldActorId")));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSavedWorldActorDeferredStreamInTest,
    "Mythic.SaveSystem.WorldActors.DeferredRecordAppliesOnStreamIn",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FSavedWorldActorDeferredStreamInTest::RunTest(
    const FString & /*Parameters*/) {
    FScopedSavedWorldActorWorld Fixture;
    UWorld *World = Fixture.Get();
    if (!World) {
        AddError(TEXT("the streaming fixture could not create a world"));
        return false;
    }

    AMythicToggleable *Toggleable = World->SpawnActor<AMythicToggleable>();
    if (!Toggleable) {
        AddError(TEXT("the fixture could not spawn a saveable actor"));
        return false;
    }
    IMythicSaveableActor *Saveable = Cast<IMythicSaveableActor>(Toggleable);
    if (!Saveable) {
        AddError(TEXT("the spawned fixture actor does not implement the saveable interface"));
        return false;
    }
    const FString ResidentId = Saveable->GetSaveableActorId();

    TArray<FSerializedWorldActorData> Captured;
    TestTrue(TEXT("capture succeeds"),
             FSerializedWorldActorHelper::SerializeAll(World, {}, Captured));
    TestEqual(TEXT("exactly the one saveable actor is captured"), Captured.Num(), 1);
    if (Captured.Num() != 1) {
        return false;
    }

    TArray<FSerializedWorldActorData> Deferred;
    FSerializedWorldActorData AsIfStreamedOut = Captured[0];
    AsIfStreamedOut.bWasRuntimeSpawned = false;
    Deferred.Add(AsIfStreamedOut);

    FSerializedWorldActorData Unrelated =
        MakePlacedRecord(TEXT("/Game/Levels/IslandMap_WP.IslandMap_WP:PersistentLevel.StillUnresident"));
    Deferred.Add(Unrelated);

    TestTrue(TEXT("the streamed-in level applies its matching deferred record"),
             FSerializedWorldActorHelper::RestoreDeferredPlacedActors(World->PersistentLevel, Deferred));
    TestEqual(TEXT("only the matched record is consumed"), Deferred.Num(), 1);
    if (Deferred.Num() == 1) {
        TestEqual(TEXT("the record with no resident actor stays pending"),
                  Deferred[0].ActorId, Unrelated.ActorId);
    }

    TestFalse(TEXT("a second stream-in of the same level applies nothing"),
              FSerializedWorldActorHelper::RestoreDeferredPlacedActors(World->PersistentLevel, Deferred));

    // A pending record must survive the next save, or an unvisited cell loses its state.
    TArray<FSerializedWorldActorData> Resaved;
    TestTrue(TEXT("capture succeeds with carry-forward"),
             FSerializedWorldActorHelper::SerializeAll(World, Deferred, Resaved));
    TestEqual(TEXT("the resident actor and the still-pending record are both written"),
              Resaved.Num(), 2);
    TestTrue(TEXT("the still-pending record is re-emitted unchanged"),
             Resaved.ContainsByPredicate([&Unrelated](const FSerializedWorldActorData &Record) {
                 return Record.ActorId.Equals(Unrelated.ActorId, ESearchCase::CaseSensitive);
             }));
    TestTrue(TEXT("the resident actor is still written from the live world"),
             Resaved.ContainsByPredicate([&ResidentId](const FSerializedWorldActorData &Record) {
                 return Record.ActorId.Equals(ResidentId, ESearchCase::CaseSensitive);
             }));

    // A resident actor already captured live must not be duplicated by its own stale carry-forward row.
    TArray<FSerializedWorldActorData> StaleCarryForward;
    StaleCarryForward.Add(AsIfStreamedOut);

    TArray<FSerializedWorldActorData> Deduplicated;
    TestTrue(TEXT("capture succeeds with a stale carry-forward row"),
             FSerializedWorldActorHelper::SerializeAll(World, StaleCarryForward, Deduplicated));
    TestEqual(TEXT("the live capture wins over its stale carried row"), Deduplicated.Num(), 1);

    return true;
}

#endif

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "GameModes/GameState/MythicGameState.h"
#include "UObject/UnrealType.h"
#include "World/Harvesting/MythicHarvestReplicationCell.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHarvestPresentationStreamOrderingTest,
    "Mythic.World.Harvesting.Replication.PresentationStreamOrdering",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestPresentationStreamOrderingTest::RunTest(
    const FString & /*Parameters*/) {
    const FGuid NonceA(1, 2, 3, 4);
    const FGuid NonceB(5, 6, 7, 8);
    const FGuid NonceC(9, 10, 11, 12);

    FMythicHarvestPresentationStreamToken Initial;
    TestFalse(TEXT("an invalid nonce cannot initialize a stream"),
              FMythicHarvestPresentationStreamToken::TryMakeInitial(
                  FGuid(), Initial));
    TestTrue(TEXT("a valid nonce initializes stream serial one"),
             FMythicHarvestPresentationStreamToken::TryMakeInitial(
                 NonceA, Initial));
    TestTrue(TEXT("the initialized stream is valid"), Initial.IsValid());
    TestEqual(TEXT("the initialized stream starts at serial one"),
              Initial.GetSerial(), 1u);

    FMythicHarvestPresentationStreamToken Advanced;
    TestTrue(TEXT("the serial advances within one nonce-scoped lifetime"),
             FMythicHarvestPresentationStreamToken::TryAdvance(
                 Initial, Advanced));
    TestEqual(TEXT("the advanced stream is newer"),
              FMythicHarvestPresentationStreamToken::Compare(
                  Advanced, Initial),
              EMythicHarvestPresentationStreamOrder::Newer);
    TestEqual(TEXT("the prior stream is older"),
              FMythicHarvestPresentationStreamToken::Compare(
                  Initial, Advanced),
              EMythicHarvestPresentationStreamOrder::Older);
    TestEqual(TEXT("an exact stream pair is idempotent"),
              FMythicHarvestPresentationStreamToken::Compare(
                  Advanced, Advanced),
              EMythicHarvestPresentationStreamOrder::Same);

    TestEqual(TEXT("another nonce is a conflict even at the same serial"),
              FMythicHarvestPresentationStreamToken::Compare(
                  FMythicHarvestPresentationStreamToken(
                      NonceC, Advanced.GetSerial()),
                  Advanced),
              EMythicHarvestPresentationStreamOrder::Conflict);
    TestEqual(TEXT("the next serial under the same nonce is newer"),
              FMythicHarvestPresentationStreamToken::Compare(
                  FMythicHarvestPresentationStreamToken(
                      Advanced.GetNonce(), Advanced.GetSerial() + 1),
                  Advanced),
              EMythicHarvestPresentationStreamOrder::Newer);
    TestEqual(TEXT("another nonce is a conflict even at a different serial"),
              FMythicHarvestPresentationStreamToken::Compare(
                  FMythicHarvestPresentationStreamToken(
                      NonceC, Advanced.GetSerial() + 1),
                  Advanced),
              EMythicHarvestPresentationStreamOrder::Conflict);
    TestEqual(TEXT("an invalid wire token is not orderable"),
              FMythicHarvestPresentationStreamToken::Compare(
                  FMythicHarvestPresentationStreamToken(), Advanced),
              EMythicHarvestPresentationStreamOrder::Invalid);

    const FMythicHarvestPresentationStreamToken HalfRangeApart(
        Advanced.GetNonce(), Advanced.GetSerial() + 0x80000000u);
    TestEqual(TEXT("an exact half-range separation fails closed"),
              FMythicHarvestPresentationStreamToken::Compare(
                  HalfRangeApart, Advanced),
              EMythicHarvestPresentationStreamOrder::Conflict);

    const FMythicHarvestPresentationStreamToken PreWrap(
        NonceA, MAX_uint32);
    FMythicHarvestPresentationStreamToken Wrapped;
    TestTrue(TEXT("the nonzero stream serial advances across wrap"),
             FMythicHarvestPresentationStreamToken::TryAdvance(
                 PreWrap, Wrapped));
    TestEqual(TEXT("wrap skips zero"), Wrapped.GetSerial(), 1u);
    TestEqual(TEXT("wrapped serial one is newer than the prior maximum"),
              FMythicHarvestPresentationStreamToken::Compare(
                  Wrapped, PreWrap),
              EMythicHarvestPresentationStreamOrder::Newer);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHarvestReplicationItemVersionTest,
    "Mythic.World.Harvesting.Replication.ItemVersioning",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestReplicationItemVersionTest::RunTest(const FString &Parameters) {
    FMythicHarvestReplicatedNodeItem Older;
    Older.PresentationStream = FMythicHarvestPresentationStreamToken(
        FGuid(0x11111111, 0x22222222, 0x33333333, 0x44444444), 7);
    Older.NodeId = FMythicHarvestNodeId(FGuid(1, 2, 3, 4));
    Older.Generation = 7;
    Older.Revision = 11;
    Older.State = EMythicHarvestNodeState::Available;
    Older.QuantizedRemainingWork = 12000;
    Older.QuantizedMaxWork = 65535;

    FMythicHarvestReplicatedNodeItem Newer = Older;
    Newer.Revision = 12;
    int32 VersionOrder = 0;
    TestTrue(TEXT("same-stream versions are comparable"),
             FMythicHarvestReplicatedNodeItem::TryCompareVersion(
                 Newer, Older, VersionOrder));
    TestEqual(TEXT("a larger revision orders after the same generation"),
              VersionOrder, 1);
    TestTrue(TEXT("reverse same-stream versions are comparable"),
             FMythicHarvestReplicatedNodeItem::TryCompareVersion(
                 Older, Newer, VersionOrder));
    TestEqual(TEXT("a smaller revision orders before the same generation"),
              VersionOrder, -1);

    FMythicHarvestReplicatedNodeItem NextGeneration = Older;
    NextGeneration.Generation = 8;
    NextGeneration.Revision = 0;
    TestTrue(TEXT("generation versions remain comparable"),
             FMythicHarvestReplicatedNodeItem::TryCompareVersion(
                 NextGeneration, Newer, VersionOrder));
    TestEqual(TEXT("a larger generation wins even when its revision resets"),
              VersionOrder, 1);

    FMythicHarvestReplicatedNodeItem WrappedRevision = Older;
    Older.Revision = MAX_uint32;
    WrappedRevision.Revision = 1;
    TestTrue(TEXT("wrapped revisions remain comparable"),
             FMythicHarvestReplicatedNodeItem::TryCompareVersion(
                 WrappedRevision, Older, VersionOrder));
    TestEqual(TEXT("nonzero revision wrap remains newer than the previous maximum"),
              VersionOrder, 1);
    TestTrue(TEXT("reverse wrapped revisions remain comparable"),
             FMythicHarvestReplicatedNodeItem::TryCompareVersion(
                 Older, WrappedRevision, VersionOrder));
    TestEqual(TEXT("a pre-wrap revision cannot overwrite the wrapped revision"),
              VersionOrder, -1);

    FMythicHarvestReplicatedNodeItem PreWrapGeneration = Newer;
    PreWrapGeneration.Generation = MAX_uint32;
    FMythicHarvestReplicatedNodeItem WrappedGeneration = Newer;
    WrappedGeneration.Generation = 1;
    TestTrue(TEXT("wrapped generations remain comparable"),
             FMythicHarvestReplicatedNodeItem::TryCompareVersion(
                 WrappedGeneration, PreWrapGeneration, VersionOrder));
    TestEqual(TEXT("nonzero generation wrap remains newer than the previous maximum"),
              VersionOrder, 1);

    FMythicHarvestReplicatedNodeItem OtherStream = Newer;
    OtherStream.PresentationStream =
        FMythicHarvestPresentationStreamToken(
            FGuid(9, 8, 7, 6), 8);
    TestFalse(TEXT("node versions from different streams are never compared"),
              FMythicHarvestReplicatedNodeItem::TryCompareVersion(
                  OtherStream, Newer, VersionOrder));

    FMythicHarvestReplicatedNodeItem SamePayload;
    SamePayload.CopyReplicatedPayloadFrom(Older);
    TestTrue(TEXT("payload copy reproduces every gameplay field"),
             SamePayload.HasSameReplicatedPayload(Older));

    SamePayload.ReplicationID = 41;
    SamePayload.ReplicationKey = 19;
    SamePayload.MostRecentArrayReplicationKey = 17;
    SamePayload.CopyReplicatedPayloadFrom(Newer);
    TestEqual(TEXT("payload copy preserves Fast Array replication identity"),
              SamePayload.ReplicationID, 41);
    TestEqual(TEXT("payload copy preserves Fast Array item key"),
              SamePayload.ReplicationKey, 19);
    TestEqual(TEXT("payload copy preserves Fast Array receive key"),
              SamePayload.MostRecentArrayReplicationKey, 17);
    TestFalse(TEXT("a newer gameplay payload does not compare equal"),
              SamePayload.HasSameReplicatedPayload(Older));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHarvestReplicationCellDefaultsTest,
    "Mythic.World.Harvesting.Replication.CellDefaults",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestReplicationCellDefaultsTest::RunTest(const FString &Parameters) {
    const AMythicHarvestReplicationCell *Cell = GetDefault<AMythicHarvestReplicationCell>();
    TestNotNull(TEXT("cell class has a default object"), Cell);
    if (!Cell) {
        return false;
    }

    TestTrue(TEXT("cell actor replicates"), Cell->GetIsReplicated());
    TestFalse(TEXT("cell is not globally relevant"), Cell->bAlwaysRelevant);
    TestFalse(TEXT("cell is not restricted to an owner"), Cell->bOnlyRelevantToOwner);
    TestFalse(TEXT("cell does not inherit owner relevancy"), Cell->bNetUseOwnerRelevancy);
    TestFalse(TEXT("runtime cell is not loaded as map content on clients"), Cell->bNetLoadOnClient);
    TestFalse(TEXT("static grid proxy does not replicate movement"), Cell->IsReplicatingMovement());
    TestEqual(TEXT("idle cell is fully dormant"), Cell->NetDormancy, DORM_DormantAll);
    TestNull(TEXT("cell has no owning connection"), Cell->GetOwner());
    TestNotNull(TEXT("cell has a spatial root component"), Cell->GetRootComponent());
    TestTrue(TEXT("default cull distance is finite and positive"),
             FMath::IsFinite(Cell->GetNetCullDistanceSquared())
                 && Cell->GetNetCullDistanceSquared() > 0.0f);

    const UClass *CellClass = AMythicHarvestReplicationCell::StaticClass();
    TestTrue(TEXT("runtime cell cannot be placed as authored world content"),
             CellClass->HasAnyClassFlags(CLASS_NotPlaceable));
    TestNull(TEXT("authority mutation is not a reflected Blueprint function"),
             CellClass->FindFunctionByName(TEXT("UpsertNodeDelta")));

    const FProperty *CellCoordinateProperty =
        FindFProperty<FProperty>(CellClass, TEXT("CellCoordinate"));
    const FProperty *NodeArrayProperty =
        FindFProperty<FProperty>(CellClass, TEXT("ReplicatedNodes"));
    const FProperty *CellStreamProperty =
        FindFProperty<FProperty>(
            CellClass, TEXT("ReplicatedPresentationStream"));
    const FProperty *ItemStreamProperty = FindFProperty<FProperty>(
        FMythicHarvestReplicatedNodeItem::StaticStruct(),
        TEXT("PresentationStream"));
    TestNotNull(TEXT("cell coordinate is reflected"), CellCoordinateProperty);
    TestNotNull(TEXT("Fast Array is reflected"), NodeArrayProperty);
    TestNotNull(TEXT("cell carries a stream barrier even when its Fast Array is empty"),
                CellStreamProperty);
    TestNotNull(TEXT("Fast Array rows carry a reflected presentation stream"),
                ItemStreamProperty);
    if (CellCoordinateProperty) {
        TestTrue(TEXT("cell coordinate replicates"),
                 CellCoordinateProperty->HasAnyPropertyFlags(CPF_Net));
        TestFalse(TEXT("cell coordinate is not exposed to Blueprint"),
                  CellCoordinateProperty->HasAnyPropertyFlags(CPF_BlueprintVisible));
    }
    if (NodeArrayProperty) {
        TestTrue(TEXT("Fast Array property replicates"),
                 NodeArrayProperty->HasAnyPropertyFlags(CPF_Net));
        TestFalse(TEXT("Fast Array property is not exposed to Blueprint"),
                  NodeArrayProperty->HasAnyPropertyFlags(CPF_BlueprintVisible));
    }
    if (CellStreamProperty) {
        TestTrue(TEXT("cell-level stream barrier replicates"),
                 CellStreamProperty->HasAnyPropertyFlags(CPF_Net));
        TestEqual(TEXT("cell-level stream barrier replays batches after property ordering"),
                  CellStreamProperty->RepNotifyFunc,
                  FName(TEXT("OnRep_PresentationStream")));
        TestFalse(TEXT("cell-level stream barrier is native-only"),
                  CellStreamProperty->HasAnyPropertyFlags(
                      CPF_BlueprintVisible));
    }
    if (ItemStreamProperty) {
        TestFalse(TEXT("the presentation stream is not exposed to Blueprint"),
                  ItemStreamProperty->HasAnyPropertyFlags(
                      CPF_BlueprintVisible));
    }

    const FProperty *CoordinatorProperty = FindFProperty<FProperty>(
        AMythicGameState::StaticClass(),
        TEXT("HarvestPresentationStreamToken"));
    TestNotNull(TEXT("GameState owns the presentation coordinator scalar"),
                CoordinatorProperty);
    if (CoordinatorProperty) {
        TestTrue(TEXT("the GameState coordinator scalar replicates"),
                 CoordinatorProperty->HasAnyPropertyFlags(CPF_Net));
        TestFalse(TEXT("the GameState coordinator is not exposed to Blueprint"),
                  CoordinatorProperty->HasAnyPropertyFlags(
                      CPF_BlueprintVisible));
        TestEqual(TEXT("the coordinator uses its reset/replay RepNotify"),
                  CoordinatorProperty->RepNotifyFunc,
                  FName(TEXT("OnRep_HarvestPresentationStreamToken")));
    }
    const AMythicGameState *GameStateDefault =
        GetDefault<AMythicGameState>();
    TestNotNull(TEXT("abstract Mythic GameState has a class default"),
                GameStateDefault);
    if (GameStateDefault) {
        TestTrue(TEXT("the presentation coordinator is always relevant"),
                 GameStateDefault->bAlwaysRelevant);
    }

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

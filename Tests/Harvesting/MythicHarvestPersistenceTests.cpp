#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Subsystem/SaveSystem/World/WorldData.h"
#include "World/Harvesting/MythicHarvestSaveTypes.h"

namespace MythicHarvestPersistenceTestsPrivate {

FMythicHarvestWorldSaveV1 MakeSnapshot() {
    FMythicHarvestWorldSaveV1 Snapshot;
    Snapshot.WorldEpoch = FGuid(0x11111111, 0x22222222, 0x33333333,
                                0x44444444);
    return Snapshot;
}

FMythicSavedHarvestNodeV1 MakeDepletedNode(
    const FGuid &NodeGuid, const FGuid &WorldEpoch) {
    FMythicSavedHarvestNodeV1 Node;
    Node.NodeGuid = NodeGuid;
    Node.WorldEpoch = WorldEpoch;
    Node.Generation = 7;
    Node.Revision = 12;
    Node.State = EMythicHarvestNodeState::Depleted;
    Node.RemainingRespawnSeconds = 42.25;
    return Node;
}

FMythicSavedHarvestContributorV1 MakeContributor(
    const TCHAR *ContributorKey, const int64 ContributionQuanta) {
    FMythicSavedHarvestContributorV1 Contributor;
    Contributor.ContributorKey = ContributorKey;
    Contributor.ContributionQuanta = ContributionQuanta;
    Contributor.ItemLevel = 37;
    Contributor.QuantityMultiplierQuanta = 1250;
    Contributor.ProficiencyLevel = 9;
    Contributor.WorkRewardContract.bInitialized = true;
    return Contributor;
}

FMythicSavedHarvestNodeV1 MakePartialNode(
    const FGuid &NodeGuid, const FGuid &WorldEpoch) {
    FMythicSavedHarvestNodeV1 Node;
    Node.NodeGuid = NodeGuid;
    Node.WorldEpoch = WorldEpoch;
    Node.Generation = 8;
    Node.Revision = 4;
    Node.State = EMythicHarvestNodeState::Available;
    Node.CapturedMaximumWorkQuanta = 100000;
    Node.RemainingWorkQuanta = 65000;
    Node.Contributors.Add(MakeContributor(TEXT("character-a"), 35000));
    return Node;
}

} // namespace MythicHarvestPersistenceTestsPrivate

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHarvestSaveValidationTest,
    "Mythic.Harvesting.Persistence.VersionOneValidation",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestSaveValidationTest::RunTest(
    const FString &Parameters) {
    using namespace MythicHarvestPersistenceTestsPrivate;

    FName Diagnostic;
    FMythicHarvestWorldSaveV1 Snapshot = MakeSnapshot();
    TestTrue(TEXT("empty row set represents all nodes available"),
             FMythicHarvestWorldSaveV1::Validate(Snapshot, Diagnostic));
    TestTrue(TEXT("valid empty snapshot clears diagnostic"),
             Diagnostic.IsNone());

    const FGuid NodeGuidA(1, 2, 3, 4);
    Snapshot.Nodes.Add(MakeDepletedNode(NodeGuidA, Snapshot.WorldEpoch));
    TestTrue(TEXT("depleted stable row is valid"),
             FMythicHarvestWorldSaveV1::Validate(Snapshot, Diagnostic));
    Snapshot.Nodes.Add(MakePartialNode(
        FGuid(5, 6, 7, 8), Snapshot.WorldEpoch));
    Snapshot.SortCanonical();
    TestTrue(TEXT("partial Available work and contributors are durable"),
             FMythicHarvestWorldSaveV1::Validate(Snapshot, Diagnostic));
    TestNotNull(TEXT("save row carries exact partial remaining work"),
                FindFProperty<FProperty>(
                    FMythicSavedHarvestNodeV1::StaticStruct(),
                    TEXT("RemainingWorkQuanta")));
    TestNotNull(TEXT("save row carries its immutable generation work contract"),
                FindFProperty<FProperty>(
                    FMythicSavedHarvestNodeV1::StaticStruct(),
                    TEXT("CapturedMaximumWorkQuanta")));
    TestNotNull(TEXT("save contributor carries its frozen typed work reward contract"),
                FindFProperty<FProperty>(
                    FMythicSavedHarvestContributorV1::StaticStruct(),
                    TEXT("WorkRewardContract")));

    FMythicHarvestWorldSaveV1 Invalid = Snapshot;
    Invalid.SchemaVersion = 2;
    TestFalse(TEXT("hard cutover rejects unknown schema"),
              FMythicHarvestWorldSaveV1::Validate(Invalid, Diagnostic));
    TestEqual(TEXT("schema diagnostic is structured"), Diagnostic,
              FName(TEXT("UnsupportedHarvestWorldSchema")));

    Invalid = MakeSnapshot();
    Invalid.Nodes.Add(MakePartialNode(
        FGuid(5, 6, 7, 8), Invalid.WorldEpoch));
    Invalid.Nodes[0].RemainingWorkQuanta =
        Invalid.Nodes[0].CapturedMaximumWorkQuanta;
    TestFalse(TEXT("untouched Available nodes remain implicit"),
              FMythicHarvestWorldSaveV1::Validate(Invalid, Diagnostic));

    Invalid = MakeSnapshot();
    Invalid.Nodes.Add(MakePartialNode(
        FGuid(5, 6, 7, 8), Invalid.WorldEpoch));
    Invalid.Nodes[0].Contributors.Reset();
    TestFalse(TEXT("partial work without entitlement inputs fails closed"),
              FMythicHarvestWorldSaveV1::Validate(Invalid, Diagnostic));

    Invalid = MakeSnapshot();
    Invalid.Nodes.Add(MakePartialNode(
        FGuid(5, 6, 7, 8), Invalid.WorldEpoch));
    Invalid.Nodes[0].Contributors[0].WorkRewardContract =
        FMythicHarvestWorkRewardContract();
    TestFalse(TEXT("partial work cannot restore without its first-hit-frozen reward contract"),
              FMythicHarvestWorldSaveV1::Validate(Invalid, Diagnostic));

    Invalid = MakeSnapshot();
    Invalid.Nodes.Add(MakePartialNode(
        FGuid(5, 6, 7, 8), Invalid.WorldEpoch));
    Invalid.Nodes[0].Contributors[0].ContributionQuanta -= 1;
    TestFalse(TEXT("partial restore rejects an under-attributed contributor ledger"),
              FMythicHarvestWorldSaveV1::Validate(Invalid, Diagnostic));
    TestEqual(TEXT("work mismatch diagnostic is structured"), Diagnostic,
              FName(TEXT("HarvestContributorWorkMismatch")));

    Invalid = MakeSnapshot();
    Invalid.Nodes.Add(MakePartialNode(
        FGuid(5, 6, 7, 8), Invalid.WorldEpoch));
    Invalid.Nodes[0].Contributors[0].ContributionQuanta += 1;
    TestFalse(TEXT("partial restore rejects an over-attributed contributor ledger"),
              FMythicHarvestWorldSaveV1::Validate(Invalid, Diagnostic));

    Invalid = MakeSnapshot();
    Invalid.Nodes.Add(MakePartialNode(
        FGuid(5, 6, 7, 8), Invalid.WorldEpoch));
    Invalid.Nodes[0].Contributors[0].ContributorKey =
        TEXT("session:connection-42");
    TestFalse(TEXT("session identities never become durable contributors"),
              FMythicHarvestWorldSaveV1::Validate(Invalid, Diagnostic));

    Invalid = MakeSnapshot();
    Invalid.Nodes.Add(MakePartialNode(
        FGuid(5, 6, 7, 8), Invalid.WorldEpoch));
    const FMythicSavedHarvestContributorV1 DuplicateContributor =
        Invalid.Nodes[0].Contributors[0];
    Invalid.Nodes[0].Contributors.Add(DuplicateContributor);
    TestFalse(TEXT("duplicate contributor identities fail closed"),
              FMythicHarvestWorldSaveV1::Validate(Invalid, Diagnostic));

    Invalid = Snapshot;
    Invalid.Nodes[0].WorldEpoch = FGuid(9, 8, 7, 6);
    TestFalse(TEXT("node capture epoch must match enclosing world snapshot"),
              FMythicHarvestWorldSaveV1::Validate(Invalid, Diagnostic));

    Invalid = MakeSnapshot();
    Invalid.Nodes.Add(MakeDepletedNode(NodeGuidA, Invalid.WorldEpoch));
    const FMythicSavedHarvestNodeV1 DuplicateNode = Invalid.Nodes[0];
    Invalid.Nodes.Add(DuplicateNode);
    TestFalse(TEXT("duplicate stable identities fail closed"),
              FMythicHarvestWorldSaveV1::Validate(Invalid, Diagnostic));
    TestEqual(TEXT("duplicate diagnostic is structured"), Diagnostic,
              FName(TEXT("DuplicateSavedHarvestNode")));

    Invalid = MakeSnapshot();
    Invalid.Nodes.Add(MakeDepletedNode(NodeGuidA, Invalid.WorldEpoch));
    Invalid.Nodes[0].RemainingRespawnSeconds = -0.001;
    TestFalse(TEXT("negative relative respawn time is rejected"),
              FMythicHarvestWorldSaveV1::Validate(Invalid, Diagnostic));

    Invalid = MakeSnapshot();
    Invalid.Nodes.Add(MakeDepletedNode(FGuid(1, 0, 0, 0), Invalid.WorldEpoch));
    Invalid.Nodes.Add(MakeDepletedNode(FGuid(2, 0, 0, 0), Invalid.WorldEpoch));
    TestFalse(TEXT("restore rejects node rows above its operational allocation ceiling"),
              FMythicHarvestWorldSaveV1::Validate(
                  Invalid, Diagnostic, 1));
    TestEqual(TEXT("node capacity diagnostic is structured"), Diagnostic,
              FName(TEXT("HarvestWorldNodeCapacityExceeded")));

    Invalid = MakeSnapshot();
    Invalid.Nodes.Add(MakePartialNode(
        FGuid(1, 0, 0, 0), Invalid.WorldEpoch));
    Invalid.Nodes[0].Contributors.Add(
        MakeContributor(TEXT("character-b"), 1));
    Invalid.Nodes[0].Contributors[0].ContributionQuanta -= 1;
    Invalid.SortCanonical();
    TestFalse(TEXT("restore bounds contributor rows before iterating their payloads"),
              FMythicHarvestWorldSaveV1::Validate(
                  Invalid, Diagnostic,
                  FMythicHarvestWorldSaveV1::AbsoluteMaximumNodes, 1));
    TestEqual(TEXT("contributor capacity diagnostic is structured"), Diagnostic,
              FName(TEXT("HarvestContributorCapacityExceeded")));

    Invalid = MakeSnapshot();
    Invalid.Nodes.Add(MakeDepletedNode(FGuid(1, 0, 0, 0), Invalid.WorldEpoch));
    Invalid.Nodes[0].ReplicationCellCoordinate.X =
        FMythicHarvestWorldSaveV1::AbsoluteMaximumCellCoordinateMagnitude + 1;
    TestFalse(TEXT("restore rejects spatial cells outside the supported deployment envelope"),
              FMythicHarvestWorldSaveV1::Validate(Invalid, Diagnostic));

    Invalid = MakeSnapshot();
    Invalid.Nodes.Add(MakeDepletedNode(FGuid(1, 0, 0, 0), Invalid.WorldEpoch));
    Invalid.Nodes.Add(MakeDepletedNode(FGuid(2, 0, 0, 0), Invalid.WorldEpoch));
    Invalid.Nodes[1].ReplicationCellCoordinate = FIntPoint(1, 0);
    TestFalse(TEXT("restore bounds distinct spatial proxies before spawning them"),
              FMythicHarvestWorldSaveV1::Validate(
                  Invalid, Diagnostic,
                  FMythicHarvestWorldSaveV1::AbsoluteMaximumNodes,
                  FMythicHarvestWorldSaveV1::AbsoluteMaximumContributorsPerNode,
                  FMythicHarvestWorldSaveV1::AbsoluteMaximumTotalContributors,
                  1));
    TestEqual(TEXT("replication-cell capacity diagnostic is structured"), Diagnostic,
              FName(TEXT("HarvestReplicationCellCapacityExceeded")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHarvestSaveCanonicalOrderTest,
    "Mythic.Harvesting.Persistence.CanonicalStableIdentityOrder",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestSaveCanonicalOrderTest::RunTest(
    const FString &Parameters) {
    using namespace MythicHarvestPersistenceTestsPrivate;

    FName Diagnostic;
    FMythicHarvestWorldSaveV1 Snapshot = MakeSnapshot();
    const FGuid High(9, 0, 0, 0);
    const FGuid Low(1, 0, 0, 0);
    Snapshot.Nodes.Add(MakeDepletedNode(High, Snapshot.WorldEpoch));
    Snapshot.Nodes.Add(MakeDepletedNode(Low, Snapshot.WorldEpoch));
    FMythicSavedHarvestNodeV1 Partial = MakePartialNode(
        FGuid(5, 0, 0, 0), Snapshot.WorldEpoch);
    Partial.Contributors.Reset();
    Partial.Contributors.Add(MakeContributor(TEXT("character-z"), 10000));
    Partial.Contributors.Add(MakeContributor(TEXT("character-a"), 25000));
    Snapshot.Nodes.Add(MoveTemp(Partial));
    Snapshot.SortCanonical();

    TestEqual(TEXT("first row is lowest stable GUID"),
              Snapshot.Nodes[0].NodeGuid, Low);
    TestEqual(TEXT("second row is partial stable GUID"),
              Snapshot.Nodes[1].NodeGuid, FGuid(5, 0, 0, 0));
    TestEqual(TEXT("third row is highest stable GUID"),
              Snapshot.Nodes[2].NodeGuid, High);
    TestEqual(TEXT("contributors sort by case-sensitive canonical key"),
              Snapshot.Nodes[1].Contributors[0].ContributorKey,
              FString(TEXT("character-a")));
    TestTrue(TEXT("canonical partial snapshot validates"),
             FMythicHarvestWorldSaveV1::Validate(Snapshot, Diagnostic));

    FSerializedWorldData WorldData;
    WorldData.HarvestWorld = Snapshot;
    WorldData.HarvestRewardOutbox.WorldEpoch = Snapshot.WorldEpoch;
    TestEqual(TEXT("world DTO embeds matching harvest capture epochs"),
              WorldData.HarvestRewardOutbox.WorldEpoch,
              WorldData.HarvestWorld.WorldEpoch);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

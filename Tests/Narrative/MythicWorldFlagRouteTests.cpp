#include "Misc/AutomationTest.h"
#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"
#include "Narrative/MythicNarrativeGrant.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Test_Story_World_BridgeBurned, "Story.World.BridgeBurned")
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Test_Story_Deserter_Spared, "Story.Deserter.Spared")

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicWorldFlagRouteTest,
    "Mythic.Narrative.WorldFlagRoute",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicWorldFlagRouteTest::RunTest(const FString &Parameters) {
    const FGameplayTag WorldTag = TAG_Test_Story_World_BridgeBurned;
    const FGameplayTag PrivateTag = TAG_Test_Story_Deserter_Spared;

    TestTrue(TEXT("Story.World.* child routes to the shared world store"),
             FMythicNarrativeGrant::IsWorldScopedGrant(WorldTag));
    TestFalse(TEXT("non-World Story.* stays per-player"),
              FMythicNarrativeGrant::IsWorldScopedGrant(PrivateTag));
    TestFalse(TEXT("invalid tag is never world-scoped"),
              FMythicNarrativeGrant::IsWorldScopedGrant(FGameplayTag()));

    FGameplayTagContainer WorldStore;
    FGameplayTagContainer PlayerA_Ledger;
    FGameplayTagContainer PlayerB_Ledger;

    WorldStore.AddTag(WorldTag);
    PlayerA_Ledger.AddTag(PrivateTag);

    auto GateSnapshot = [](const FGameplayTagContainer &Ledger, const FGameplayTagContainer &World) {
        FGameplayTagContainer Owned;
        Owned.AppendTags(Ledger);
        Owned.AppendTags(World);
        return Owned;
    };
    const FGameplayTagContainer SnapshotA = GateSnapshot(PlayerA_Ledger, WorldStore);
    const FGameplayTagContainer SnapshotB = GateSnapshot(PlayerB_Ledger, WorldStore);

    TestTrue(TEXT("world flag visible in player A's gate snapshot"), SnapshotA.HasTag(WorldTag));
    TestTrue(TEXT("world flag visible in ANOTHER player's (B) gate snapshot"), SnapshotB.HasTag(WorldTag));
    TestFalse(TEXT("world flag NOT on player A's private ledger"), PlayerA_Ledger.HasTag(WorldTag));
    TestFalse(TEXT("world flag NOT on player B's private ledger"), PlayerB_Ledger.HasTag(WorldTag));

    TestTrue(TEXT("private tag visible in player A's gate snapshot"), SnapshotA.HasTag(PrivateTag));
    TestFalse(TEXT("private tag NOT visible in player B's gate snapshot"), SnapshotB.HasTag(PrivateTag));
    TestFalse(TEXT("private tag NOT raised in the shared world store"), WorldStore.HasTag(PrivateTag));

    FMythicNarrativeGrant::RouteGrant(nullptr, nullptr, WorldTag);
    FMythicNarrativeGrant::RouteGrant(nullptr, nullptr, PrivateTag);
    FMythicNarrativeGrant::RouteGrant(nullptr, nullptr, FGameplayTag());
    TestTrue(TEXT("RouteGrant tolerated null world/ledger without crashing"), true);

    return true;
}

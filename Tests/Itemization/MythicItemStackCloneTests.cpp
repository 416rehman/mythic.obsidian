#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Itemization/Affixes/MythicAffixDefinition.h"
#include "Itemization/Affixes/MythicAffixRng.h"
#include "Itemization/Affixes/MythicTags_Affixes.h"
#include "Itemization/Inventory/Fragments/Passive/AffixesFragment.h"
#include "Itemization/Inventory/Fragments/Passive/MythicGemFragment.h"
#include "Itemization/Inventory/Fragments/Passive/SocketsFragment.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Misc/ScopeExit.h"
#include "Player/MythicPlayerController.h"

namespace {
struct FStackCloneFixture {
    UGameInstance *GameInstance = nullptr;
    AMythicPlayerController *Owner = nullptr;
    UMythicAffixDefinition *AffixDefinition = nullptr;

    bool Initialize(FAutomationTestBase &Test) {
        if (!Test.TestNotNull(TEXT("engine is available"), GEngine)) return false;
        GameInstance = NewObject<UGameInstance>(GEngine);
        GameInstance->InitializeStandalone();
        UWorld *World = GameInstance->GetWorld();
        if (!Test.TestNotNull(TEXT("standalone world exists"), World)) return false;
        Owner = World->SpawnActor<AMythicPlayerController>();
        if (!Test.TestNotNull(TEXT("authority owner spawned"), Owner)
            || !Test.TestTrue(TEXT("fixture owner has authority"), Owner->HasAuthority())) {
            return false;
        }
        AffixDefinition = NewObject<UMythicAffixDefinition>(
            GameInstance, TEXT("StackCloneAffixDefinition"));
        AffixDefinition->AffixTag = FGameplayTag::RequestGameplayTag(
            FName(TEXT("Itemization.Affix.Power")), true);
        return Test.TestTrue(TEXT("typed Affix Definition fixture is valid"),
                             AffixDefinition->GetPrimaryAssetId().IsValid());
    }

    void Shutdown() const {
        if (GameInstance) GameInstance->Shutdown();
    }
};

FRolledAffix MakeOwnedSnapshot(UMythicAffixDefinition *Definition,
                               const FGuid &OwnerItemGuid,
                               const FGuid &RollGuid,
                               const FGameplayTag SourceKind,
                               const float Magnitude,
                               const bool bLocked) {
    FRolledAffix Snapshot;
    Snapshot.RollGuid = RollGuid;
    Snapshot.AffixDefinition.SetAsset(Definition);
    Snapshot.TierRank = 3;
    Snapshot.Magnitude = Magnitude;
    Snapshot.Provenance.RollGroup = AFFIX_ROLL_GROUP_PREFIX;
    Snapshot.Provenance.SourceKind = SourceKind;
    Snapshot.Provenance.SourceItemGuid = OwnerItemGuid;
    Snapshot.Provenance.GeneratedItemLevel = 27;
    Snapshot.Provenance.AlgorithmVersion = 1;
    Snapshot.bIsLocked = bLocked;
    return Snapshot;
}

FGuid ExpectedClonedRollGuid(const FGuid &NewItemGuid, const FGuid &OldRollGuid) {
    FMythicAffixCanonicalWriter Fields("MYTHIC_ITEM_STACK_CLONE_ROLL_FIELDS_V1");
    Fields.AddGuid(NewItemGuid);
    Fields.AddGuid(OldRollGuid);
    return FMythicAffixRngFactory::GuidFromCanonicalBytes(
        "Mythic.Item.StackClone.Roll.V1", Fields.GetBytes());
}

FGuid ExpectedClonedSocketGuid(const FGuid &NewItemGuid, const FGuid &OldSocketGuid) {
    FMythicAffixCanonicalWriter Fields("MYTHIC_ITEM_STACK_CLONE_SOCKET_FIELDS_V1");
    Fields.AddGuid(NewItemGuid);
    Fields.AddGuid(OldSocketGuid);
    return FMythicAffixRngFactory::GuidFromCanonicalBytes(
        "Mythic.Item.StackClone.Socket.V1", Fields.GetBytes());
}

UMythicItemInstance *BuildCurrentStack(
    FStackCloneFixture &Fixture, const bool bIncludeBaseAndSocketState) {
    UItemDefinition *Definition = NewObject<UItemDefinition>(Fixture.GameInstance);
    Definition->StackSizeMax = 20;

    if (bIncludeBaseAndSocketState) {
        UAffixesFragment *AffixesTemplate = NewObject<UAffixesFragment>(Definition);
        FRolledAffix Placeholder = MakeOwnedSnapshot(
            Fixture.AffixDefinition, FGuid(), FGuid(1, 2, 3, 4),
            AFFIX_SOURCE_EXPLICIT, 11.5f, false);
        TArray<FRolledAffix> PlaceholderSnapshots{MoveTemp(Placeholder)};
        AffixesTemplate->AffixSnapshots.ReplaceAll(MoveTemp(PlaceholderSnapshots));
        Definition->Fragments.Add(AffixesTemplate);
    }

    UMythicGemFragment *GemTemplate = NewObject<UMythicGemFragment>(Definition);
    GemTemplate->GemType = FGameplayTag::RequestGameplayTag(
        FName(TEXT("Itemization.Gem.Ruby")), true);
    Definition->Fragments.Add(GemTemplate);

    if (bIncludeBaseAndSocketState) {
        Definition->Fragments.Add(NewObject<USocketsFragment>(Definition));
    }

    UMythicItemInstance *Item = NewObject<UMythicItemInstance>(Fixture.Owner);
    Item->SetOwner(Fixture.Owner);
    Item->InitializeFixtureForTests(Definition, 10, 27);
    const FGuid ItemGuid = Item->GetItemInstanceGuid();
    if (!ItemGuid.IsValid()) return nullptr;

    if (bIncludeBaseAndSocketState) {
        UAffixesFragment *Affixes = const_cast<UAffixesFragment *>(
            Item->GetFragment<UAffixesFragment>());
        if (!Affixes || Affixes->AffixSnapshots.Items.Num() != 1) return nullptr;
        Affixes->AffixSnapshots.Items[0].Affix.Provenance.SourceItemGuid = ItemGuid;
        Affixes->AffixSnapshots.MarkArrayDirty();
    }

    UMythicGemFragment *Gem = const_cast<UMythicGemFragment *>(
        Item->GetFragment<UMythicGemFragment>());
    if (!Gem) return nullptr;
    TArray<FRolledAffix> GemSnapshots;
    GemSnapshots.Add(MakeOwnedSnapshot(
        Fixture.AffixDefinition, ItemGuid, FGuid(5, 6, 7, 8),
        AFFIX_SOURCE_GEM, 7.25f, true));
    Gem->GrantedAffixSnapshots.ReplaceAll(MoveTemp(GemSnapshots));

    if (bIncludeBaseAndSocketState) {
        USocketsFragment *Sockets = const_cast<USocketsFragment *>(
            Item->GetFragment<USocketsFragment>());
        if (!Sockets) return nullptr;
        FMythicReplicatedSocketItem &Socket = Sockets->SocketStates.Items.AddDefaulted_GetRef();
        Socket.SocketGuid = FGuid(9, 10, 11, 12);
        Socket.SocketedGemType = Gem->GemType;
        Socket.SourceGemItemGuid = FGuid(13, 14, 15, 16);
        Socket.bFilled = true;
        FRolledAffix SocketSnapshot = MakeOwnedSnapshot(
            Fixture.AffixDefinition, Socket.SourceGemItemGuid,
            FGuid(17, 18, 19, 20), AFFIX_SOURCE_SOCKET, 4.5f, true);
        SocketSnapshot.Provenance.OriginSocketGuid = Socket.SocketGuid;
        Socket.SocketedAffixSnapshots.Add(MoveTemp(SocketSnapshot));
        Sockets->SocketStates.MarkArrayDirty();
    }
    return Item;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicItemStackSemanticCloneTest,
    "Mythic.Itemization.Inventory.StackClonePreservesSemanticsAndRekeysIdentity",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicItemStackSemanticCloneTest::RunTest(const FString &Parameters) {
    FStackCloneFixture Fixture;
    ON_SCOPE_EXIT { Fixture.Shutdown(); };
    if (!Fixture.Initialize(*this)) return false;

    UMythicItemInstance *Source = BuildCurrentStack(Fixture, true);
    if (!TestNotNull(TEXT("current source stack exists"), Source)) return false;
    const FGuid OldItemGuid = Source->GetItemInstanceGuid();
    const UAffixesFragment *OldAffixes = Source->GetFragment<UAffixesFragment>();
    const UMythicGemFragment *OldGem = Source->GetFragment<UMythicGemFragment>();
    const USocketsFragment *OldSockets = Source->GetFragment<USocketsFragment>();
    if (!OldAffixes || !OldGem || !OldSockets) return false;
    const FGuid OldBaseRoll = OldAffixes->AffixSnapshots.Items[0].Affix.RollGuid;
    const FGuid OldGemRoll = OldGem->GrantedAffixSnapshots.Items[0].Affix.RollGuid;
    const FGuid OldSocketGuid = OldSockets->SocketStates.Items[0].SocketGuid;
    const FGuid OldSocketRoll = OldSockets->SocketStates.Items[0].SocketedAffixSnapshots[0].RollGuid;

    UMythicItemInstance *Clone = Source->CloneForStackSplit(Fixture.Owner, 3);
    if (!TestNotNull(TEXT("complete current state clones"), Clone)) return false;
    const FGuid NewItemGuid = Clone->GetItemInstanceGuid();
    TestTrue(TEXT("clone receives a fresh physical item identity"),
             NewItemGuid.IsValid() && NewItemGuid != OldItemGuid);
    TestEqual(TEXT("requested split quantity is isolated on clone"), Clone->GetStacks(), 3);
    TestNull(TEXT("clone remains unowned until an inventory transaction publishes it"),
             Clone->GetOwningActor());

    const UAffixesFragment *NewAffixes = Clone->GetFragment<UAffixesFragment>();
    const UMythicGemFragment *NewGem = Clone->GetFragment<UMythicGemFragment>();
    const USocketsFragment *NewSockets = Clone->GetFragment<USocketsFragment>();
    if (!NewAffixes || !NewGem || !NewSockets) return false;
    const FRolledAffix &NewBase = NewAffixes->AffixSnapshots.Items[0].Affix;
    const FRolledAffix &NewGemRoll = NewGem->GrantedAffixSnapshots.Items[0].Affix;
    const FMythicReplicatedSocketItem &NewSocket = NewSockets->SocketStates.Items[0];
    const FRolledAffix &NewSocketRoll = NewSocket.SocketedAffixSnapshots[0];

    TestEqual(TEXT("base magnitude is immutable across a stack clone"), NewBase.Magnitude, 11.5f);
    TestTrue(TEXT("base direct definition is preserved"),
             NewBase.AffixDefinition
                 == OldAffixes->AffixSnapshots.Items[0].Affix.AffixDefinition);
    TestEqual(TEXT("base roll is deterministically rekeyed"), NewBase.RollGuid,
              ExpectedClonedRollGuid(NewItemGuid, OldBaseRoll));
    TestEqual(TEXT("base provenance follows the new physical item"),
              NewBase.Provenance.SourceItemGuid, NewItemGuid);

    TestEqual(TEXT("gem magnitude is immutable across a stack clone"), NewGemRoll.Magnitude, 7.25f);
    TestEqual(TEXT("gem roll is deterministically rekeyed"), NewGemRoll.RollGuid,
              ExpectedClonedRollGuid(NewItemGuid, OldGemRoll));
    TestEqual(TEXT("gem provenance follows the new physical item"),
              NewGemRoll.Provenance.SourceItemGuid, NewItemGuid);

    TestEqual(TEXT("host socket receives a fresh deterministic identity"), NewSocket.SocketGuid,
              ExpectedClonedSocketGuid(NewItemGuid, OldSocketGuid));
    TestEqual(TEXT("socket roll is deterministically rekeyed"), NewSocketRoll.RollGuid,
              ExpectedClonedRollGuid(NewItemGuid, OldSocketRoll));
    TestEqual(TEXT("socket origin follows the cloned host socket"),
              NewSocketRoll.Provenance.OriginSocketGuid, NewSocket.SocketGuid);
    TestEqual(TEXT("external source-gem provenance remains unchanged"),
              NewSocketRoll.Provenance.SourceItemGuid,
              OldSockets->SocketStates.Items[0].SourceGemItemGuid);
    TestEqual(TEXT("socket magnitude remains immutable"), NewSocketRoll.Magnitude, 4.5f);

    TestEqual(TEXT("source stack quantity is untouched by cloning"), Source->GetStacks(), 10);
    TestEqual(TEXT("source physical identity is untouched"), Source->GetItemInstanceGuid(), OldItemGuid);
    TestEqual(TEXT("source base roll identity is untouched"),
              OldAffixes->AffixSnapshots.Items[0].Affix.RollGuid, OldBaseRoll);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicGemStackCloneCompatibilityTest,
    "Mythic.Itemization.Inventory.GemStackCloneRemainsStackCompatible",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicGemStackCloneCompatibilityTest::RunTest(const FString &Parameters) {
    FStackCloneFixture Fixture;
    ON_SCOPE_EXIT { Fixture.Shutdown(); };
    if (!Fixture.Initialize(*this)) return false;

    UMythicItemInstance *Source = BuildCurrentStack(Fixture, false);
    if (!TestNotNull(TEXT("current gem stack exists"), Source)) return false;
    UMythicItemInstance *Clone = Source->CloneForStackSplit(Fixture.Owner, 4);
    if (!TestNotNull(TEXT("gem stack semantic clone succeeds"), Clone)) return false;
    TestTrue(TEXT("fresh physical identities remain stack-compatible when immutable rolls match"),
             Source->isStackableWith(Clone) && Clone->isStackableWith(Source));

    UMythicGemFragment *SourceGem = const_cast<UMythicGemFragment *>(
        Source->GetFragment<UMythicGemFragment>());
    SourceGem->GrantedAffixSnapshots.Items[0].Affix.Provenance.SourceItemGuid = FGuid::NewGuid();
    TestNull(TEXT("mismatched physical provenance fails the entire clone without fallback"),
             Source->CloneForStackSplit(Fixture.Owner, 1));
    TestEqual(TEXT("failed clone leaves the source quantity unchanged"), Source->GetStacks(), 10);
    return true;
}

#endif

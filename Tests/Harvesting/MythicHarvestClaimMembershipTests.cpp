#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Engine/World.h"
#include "World/Harvesting/MythicHarvestClaimMembershipSubsystem.h"

namespace {

class FScopedHarvestClaimAuthorityWorld final {
public:
    FScopedHarvestClaimAuthorityWorld() {
        InitializationValues = UWorld::InitializationValues()
                                   .CreatePhysicsScene(false)
                                   .ShouldSimulatePhysics(false)
                                   .EnableTraceCollision(false)
                                   .CreateNavigation(false)
                                   .CreateAISystem(false);
        World = UWorld::CreateWorld(
            EWorldType::PIE, false,
            MakeUniqueObjectName(nullptr, UWorld::StaticClass(),
                                 TEXT("HarvestClaimMembershipTest")),
            nullptr, true, ERHIFeatureLevel::Num, &InitializationValues,
            true);
        if (World) {
            World->SetPlayInEditorInitialNetMode(NM_ListenServer);
            World->InitWorld(InitializationValues);
        }
    }

    ~FScopedHarvestClaimAuthorityWorld() {
        if (World) {
            World->DestroyWorld(false);
        }
    }

    UWorld *Get() const { return World; }

private:
    UWorld::InitializationValues InitializationValues;
    UWorld *World = nullptr;
};

} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHarvestTypedClaimIdentityTest,
    "Mythic.Harvesting.Claims.TypedIdentityDomainSeparation",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestTypedClaimIdentityTest::RunTest(
    const FString & /*Parameters*/) {
    const FString PersistentPlayerKey(TEXT("character:account-17:slot-3"));
    const FGuid PartyId(0x10203040, 0x50607080, 0x90a0b0c0,
                        0xd0e0f001);

    const FMythicHarvestClaimIdentity PlayerClaim =
        FMythicHarvestClaimIdentity::MakePlayer(PersistentPlayerKey);
    const FMythicHarvestClaimIdentity SamePlayerClaim =
        FMythicHarvestClaimIdentity::MakePlayer(PersistentPlayerKey);
    const FMythicHarvestClaimIdentity CaseDistinctPlayerClaim =
        FMythicHarvestClaimIdentity::MakePlayer(
            TEXT("Character:account-17:slot-3"));
    const FMythicHarvestClaimIdentity PartyClaim =
        FMythicHarvestClaimIdentity::MakeParty(PartyId);

    TestTrue(TEXT("a persistent player claim is valid"),
             PlayerClaim.IsValid());
    TestTrue(TEXT("a typed party claim is valid"), PartyClaim.IsValid());
    TestTrue(TEXT("the same player key identifies the same claim"),
             PlayerClaim == SamePlayerClaim);
    TestFalse(TEXT("persistent player keys remain case-sensitive"),
              PlayerClaim == CaseDistinctPlayerClaim);
    TestFalse(TEXT("player and party domains cannot alias"),
              PlayerClaim == PartyClaim);
    TestTrue(TEXT("player claims carry only the player domain payload"),
             PlayerClaim.Kind
                     == FMythicHarvestClaimIdentity::EKind::Player
                 && PlayerClaim.PersistentPlayerKey == PersistentPlayerKey
                 && !PlayerClaim.PartyId.IsValid());
    TestTrue(TEXT("party claims carry only the party domain payload"),
             PartyClaim.Kind == FMythicHarvestClaimIdentity::EKind::Party
                 && PartyClaim.PersistentPlayerKey.IsEmpty()
                 && PartyClaim.PartyId == PartyId);

    TestFalse(TEXT("the default none claim is invalid"),
              FMythicHarvestClaimIdentity().IsValid());
    TestFalse(TEXT("an empty player key is invalid"),
              FMythicHarvestClaimIdentity::MakePlayer(FString()).IsValid());
    TestFalse(TEXT("a pre-load session identity cannot become a claim"),
              FMythicHarvestClaimIdentity::MakePlayer(
                  TEXT("session:connection-42"))
                  .IsValid());
    TestFalse(TEXT("an invalid party GUID cannot become a claim"),
              FMythicHarvestClaimIdentity::MakeParty(FGuid()).IsValid());

    FMythicHarvestClaimIdentity MixedPlayerClaim = PlayerClaim;
    MixedPlayerClaim.PartyId = PartyId;
    TestFalse(TEXT("a player claim cannot also carry a party payload"),
              MixedPlayerClaim.IsValid());

    FMythicHarvestClaimIdentity MixedPartyClaim = PartyClaim;
    MixedPartyClaim.PersistentPlayerKey = PersistentPlayerKey;
    TestFalse(TEXT("a party claim cannot also carry a player payload"),
              MixedPartyClaim.IsValid());

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHarvestAtomicPartyMembershipTest,
    "Mythic.Harvesting.Claims.AtomicPartyMembership",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestAtomicPartyMembershipTest::RunTest(
    const FString & /*Parameters*/) {
    FScopedHarvestClaimAuthorityWorld ScopedWorld;
    UWorld *World = ScopedWorld.Get();
    TestNotNull(TEXT("authority claim test world initializes"), World);
    if (!World) {
        return false;
    }
    TestEqual(TEXT("claim test world is authoritative"),
              World->GetNetMode(), NM_ListenServer);

    UMythicHarvestClaimMembershipSubsystem *Membership =
        World->GetSubsystem<UMythicHarvestClaimMembershipSubsystem>();
    TestNotNull(TEXT("authority claim membership subsystem initializes"),
                Membership);
    if (!Membership) {
        return false;
    }

    const FGuid PartyA(0x11111111, 0x22222222, 0x33333333,
                       0x44444444);
    const FGuid PartyB(0xaaaaaaaa, 0xbbbbbbbb, 0xcccccccc,
                       0xdddddddd);
    const FString Alice(TEXT("character:alice"));
    const FString Bob(TEXT("character:bob"));
    const FString Charlie(TEXT("character:charlie"));

    TArray<FString> InitialPartyA = {Alice, Bob};
    TestTrue(TEXT("an authoritative party snapshot is accepted"),
             Membership->ReplacePartyMembers(PartyA, 1, InitialPartyA));

    FGuid ResolvedParty;
    TestTrue(TEXT("the first member resolves to party A"),
             Membership->TryResolveParty(Alice, ResolvedParty));
    TestEqual(TEXT("the first member maps to party A"), ResolvedParty,
              PartyA);
    TestTrue(TEXT("the second member resolves to party A"),
             Membership->TryResolveParty(Bob, ResolvedParty));
    TestEqual(TEXT("the second member maps to party A"), ResolvedParty,
              PartyA);

    TArray<FString> InvalidSessionReplacement = {
        Charlie, TEXT("session:unbound-controller")};
    TestFalse(TEXT("a session-key snapshot is rejected as one unit"),
              Membership->ReplacePartyMembers(
                  PartyA, 2, InvalidSessionReplacement));
    TestTrue(TEXT("session-key rejection preserves the first old member"),
             Membership->TryResolveParty(Alice, ResolvedParty));
    TestEqual(TEXT("the first old mapping is unchanged"), ResolvedParty,
              PartyA);
    TestTrue(TEXT("session-key rejection preserves the second old member"),
             Membership->TryResolveParty(Bob, ResolvedParty));
    TestEqual(TEXT("the second old mapping is unchanged"), ResolvedParty,
              PartyA);
    TestFalse(TEXT("the rejected new member is not partially installed"),
              Membership->TryResolveParty(Charlie, ResolvedParty));
    TestFalse(TEXT("failed lookup clears its output GUID"),
              ResolvedParty.IsValid());

    TArray<FString> InvalidPartyReplacement = {Charlie};
    TestFalse(TEXT("an invalid party identity rejects the full snapshot"),
              Membership->ReplacePartyMembers(
                  FGuid(), 1, InvalidPartyReplacement));
    TestTrue(TEXT("invalid party rejection preserves the old snapshot"),
             Membership->TryResolveParty(Alice, ResolvedParty));
    TestEqual(TEXT("invalid party rejection preserves its party identity"),
              ResolvedParty, PartyA);

    TArray<FString> DuplicateReplacement = {Alice, Alice};
    TestFalse(TEXT("a duplicate-member snapshot is rejected atomically"),
              Membership->ReplacePartyMembers(PartyA, 2,
                                              DuplicateReplacement));
    TestTrue(TEXT("duplicate rejection preserves party A membership"),
             Membership->TryResolveParty(Bob, ResolvedParty));
    TestEqual(TEXT("duplicate rejection preserves party A identity"),
              ResolvedParty, PartyA);

    TArray<FString> ReplacementPartyA = {Charlie};
    TestTrue(TEXT("a complete replacement snapshot is accepted"),
             Membership->ReplacePartyMembers(PartyA, 2, ReplacementPartyA));
    TestFalse(TEXT("replacement removes the first omitted member"),
              Membership->TryResolveParty(Alice, ResolvedParty));
    TestFalse(TEXT("replacement removes the second omitted member"),
              Membership->TryResolveParty(Bob, ResolvedParty));
    TestTrue(TEXT("replacement installs the new member"),
             Membership->TryResolveParty(Charlie, ResolvedParty));
    TestEqual(TEXT("the new member maps to party A"), ResolvedParty,
              PartyA);

    TArray<FString> PartyBMembers = {Bob};
    TestTrue(TEXT("a second party snapshot is accepted in global transition order"),
             Membership->ReplacePartyMembers(PartyB, 3, PartyBMembers));
    TestTrue(TEXT("the second party member resolves"),
             Membership->TryResolveParty(Bob, ResolvedParty));
    TestEqual(TEXT("the second party owns only its published member"),
              ResolvedParty, PartyB);
    TestTrue(TEXT("the first party mapping remains intact"),
             Membership->TryResolveParty(Charlie, ResolvedParty));
    TestEqual(TEXT("the first party retains its identity"), ResolvedParty,
              PartyA);

    TestTrue(TEXT("removing party A reports an authoritative mutation"),
             Membership->RemoveParty(PartyA, 4));
    TestFalse(TEXT("removing party A clears all of its members"),
              Membership->TryResolveParty(Charlie, ResolvedParty));
    TestTrue(TEXT("removing party A does not disturb party B"),
             Membership->TryResolveParty(Bob, ResolvedParty));
    TestEqual(TEXT("party B mapping survives party A removal"),
              ResolvedParty, PartyB);
    TestFalse(TEXT("a replayed party tombstone is rejected"),
              Membership->RemoveParty(PartyA, 4));

    TArray<FString> DelayedPartyA = {Alice};
    TestFalse(TEXT("a delayed roster cannot resurrect a removed party"),
              Membership->ReplacePartyMembers(
                  PartyA, 2, DelayedPartyA));
    TestFalse(TEXT("the stale roster installs no member"),
              Membership->TryResolveParty(Alice, ResolvedParty));

    TArray<FString> NewPartyAMembers = {Alice};
    TestTrue(TEXT("a genuinely newer global transition may recreate party A"),
             Membership->ReplacePartyMembers(
                 PartyA, 5, NewPartyAMembers));
    TArray<FString> MoveAliceToPartyB = {Bob, Alice};
    TestTrue(TEXT("a newer global transition moves a player across parties"),
             Membership->ReplacePartyMembers(
                 PartyB, 6, MoveAliceToPartyB));
    TestTrue(TEXT("the moved player resolves to the new party"),
             Membership->TryResolveParty(Alice, ResolvedParty));
    TestEqual(TEXT("cross-party move installs the new identity"),
              ResolvedParty, PartyB);
    TestFalse(TEXT("a delayed old-party snapshot cannot reclaim a moved player"),
              Membership->ReplacePartyMembers(
                  PartyA, 5, NewPartyAMembers));
    TestTrue(TEXT("stale cross-party rejection preserves the newer party"),
             Membership->TryResolveParty(Alice, ResolvedParty));
    TestEqual(TEXT("the newer cross-party assignment remains authoritative"),
              ResolvedParty, PartyB);

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

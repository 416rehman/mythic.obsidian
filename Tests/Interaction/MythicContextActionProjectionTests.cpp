#include "Misc/AutomationTest.h"

#include <limits>

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Algo/Reverse.h"
#include "AI/NPCs/MythicNPCCharacter.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Interaction/ContextActions/MythicContextActionDefinition.h"
#include "Interaction/ContextActions/MythicContextActionProjectionPolicy.h"
#include "Interaction/ContextActions/MythicEntityActionGrantComponent.h"
#include "NativeGameplayTags.h"
#include "Net/UnrealNetwork.h"
#include "UObject/UnrealType.h"
#include "World/Entity/MythicEntityPresentationComponent.h"
#include "World/Entity/MythicEntityPresentationRegistry.h"
#include "World/Entity/MythicEntityPresentationTags.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_ContextActionProjectionTestLow,
                              "Context.Action.Automation.Low");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_ContextActionProjectionTestMid,
                              "Context.Action.Automation.Mid");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_ContextActionProjectionTestHigh,
                              "Context.Action.Automation.High");

namespace MythicContextActionProjectionTests {
class FScopedAuthorityWorld final {
public:
    FScopedAuthorityWorld() {
        Values = UWorld::InitializationValues()
                     .CreatePhysicsScene(false)
                     .ShouldSimulatePhysics(false)
                     .EnableTraceCollision(false)
                     .CreateNavigation(false)
                     .CreateAISystem(false);
        World = UWorld::CreateWorld(
            EWorldType::PIE, false,
            MakeUniqueObjectName(nullptr, UWorld::StaticClass(),
                                 TEXT("ContextActionProjectionTest")),
            nullptr, true, ERHIFeatureLevel::Num, &Values, true);
        if (World) {
            World->SetPlayInEditorInitialNetMode(NM_ListenServer);
            World->InitWorld(Values);
        }
    }

    ~FScopedAuthorityWorld() {
        if (World) {
            World->DestroyWorld(false);
        }
    }

    UWorld *Get() const { return World; }

private:
    UWorld::InitializationValues Values;
    UWorld *World = nullptr;
};

FMythicEntityId MakeEntityId(const uint32 Salt) {
    return FMythicEntityId::FromAuthorityGuid(
        EMythicEntityDomain::Runtime,
        FGuid(0xCA000000u + Salt, 0xCB000000u + Salt,
              0xCC000000u + Salt, 0xCD000000u + Salt));
}

UMythicEntityActionGrantComponent *CreateGrantComponent(AActor &Owner,
                                                         const FName Name) {
    UMythicEntityActionGrantComponent *Component =
        NewObject<UMythicEntityActionGrantComponent>(&Owner, Name);
    Owner.AddInstanceComponent(Component);
    Component->RegisterComponent();
    return Component;
}

FMythicContextActionOffer MakeOffer(UObject &Outer, const FGameplayTag Tag,
                                    const int32 Priority) {
    UMythicContextActionDefinition *Definition =
        NewObject<UMythicContextActionDefinition>(&Outer);
    Definition->ActionTag = Tag;
    Definition->PresentationPriority = Priority;

    FMythicContextActionOffer Offer;
    Offer.Definition = Definition;
    Offer.Availability = EMythicContextActionAvailability::Available;
    Offer.SourceRevision = static_cast<int64>(Priority + 100);
    return Offer;
}

AMythicNPCCharacter *SpawnActivatedProvider(
    UWorld &World, const uint32 Salt,
    FMythicEntityPresentationInstance &OutSubject) {
    OutSubject.Reset();
    AMythicNPCCharacter *Provider =
        World.SpawnActor<AMythicNPCCharacter>();
    UMythicEntityPresentationComponent *Presentation = Provider
        ? Provider->GetEntityPresentationComponent_Implementation() : nullptr;
    if (!Provider || !Presentation) {
        return nullptr;
    }

    FMythicPublicIdentitySnapshot SafeIdentity;
    SafeIdentity.PublicKindTag =
        MythicEntityPresentationTags::EntityKindHumanoid;
    if (!Presentation->AuthorityPrepareEmbodiment(MakeEntityId(Salt),
                                                   SafeIdentity)
        || !Presentation->AuthorityActivateEmbodiment()) {
        return nullptr;
    }
    OutSubject = Presentation->GetPresentationInstance();
    return OutSubject.IsValid() ? Provider : nullptr;
}

FMythicAuthorityContextActionOffer MakeBoundOffer(
    UObject &Provider, const FGameplayTag Tag, const int32 Priority) {
    return FMythicAuthorityContextActionOffer(
        &Provider, MakeOffer(Provider, Tag, Priority));
}
} // namespace MythicContextActionProjectionTests

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicContextActionProjectionRateLimitTest,
    "Mythic.World.Entity.ContextActions.RequestRateLimit",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicContextActionProjectionRateLimitTest::RunTest(
    const FString & /*Parameters*/) {
    using Rules = FMythicContextActionProjectionRules;
    TestEqual(TEXT("a first request has no throttle delay"),
              Rules::GetRequestThrottleDelaySeconds(10.0, -DBL_MAX, 0.10f),
              0.0);
    TestTrue(TEXT("an early repeat is deferred for its remaining interval"),
             FMath::IsNearlyEqual(
                 Rules::GetRequestThrottleDelaySeconds(10.04, 10.0, 0.10f),
                 0.06, 0.0001));
    TestEqual(TEXT("the boundary request is accepted"),
              Rules::GetRequestThrottleDelaySeconds(10.10, 10.0, 0.10f),
              0.0);
    TestFalse(TEXT("an unsafe authored interval fails closed"),
              FMath::IsFinite(Rules::GetRequestThrottleDelaySeconds(
                  10.0, 9.0, 0.0f)));
    TestFalse(TEXT("an interval over the hard maximum fails closed"),
              FMath::IsFinite(Rules::GetRequestThrottleDelaySeconds(
                  10.0, 9.0,
                  Rules::HardMaximumRequestIntervalSeconds + 0.01f)));
    TestFalse(TEXT("a nonfinite authority clock fails closed"),
              FMath::IsFinite(Rules::GetRequestThrottleDelaySeconds(
                  std::numeric_limits<double>::quiet_NaN(), 9.0, 0.1f)));
    TestFalse(TEXT("a nonfinite last-accepted clock fails closed"),
              FMath::IsFinite(Rules::GetRequestThrottleDelaySeconds(
                  10.0, std::numeric_limits<double>::quiet_NaN(), 0.1f)));
    TestFalse(TEXT("a nonfinite interval fails closed"),
              FMath::IsFinite(Rules::GetRequestThrottleDelaySeconds(
                  10.0, 9.0,
                  std::numeric_limits<float>::quiet_NaN())));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicContextActionHoldTimingTest,
    "Mythic.World.Entity.ContextActions.AuthorityHoldTiming",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicContextActionHoldTimingTest::RunTest(
    const FString & /*Parameters*/) {
    using Rules = FMythicContextActionProjectionRules;
    TestTrue(TEXT("zero remains the canonical tap duration"),
             Rules::IsHoldDurationValid(0.0f));
    TestTrue(TEXT("an ordinary authored hold is valid"),
             Rules::IsHoldDurationValid(0.5f));
    TestTrue(TEXT("the minimum deliberate hold is valid"),
             Rules::IsHoldDurationValid(
                 Rules::HardMinimumHoldDurationSeconds));
    TestFalse(TEXT("a positive tap-like duration fails closed"),
              Rules::IsHoldDurationValid(
                  Rules::HardMinimumHoldDurationSeconds - 0.01f));
    TestFalse(TEXT("an excessive authored hold fails closed"),
              Rules::IsHoldDurationValid(
                  Rules::HardMaximumHoldDurationSeconds + 0.01f));
    TestFalse(TEXT("a nonfinite authored hold fails closed"),
              Rules::IsHoldDurationValid(
                  std::numeric_limits<float>::quiet_NaN()));

    TestFalse(TEXT("a tap cannot satisfy the hold handshake"),
              Rules::IsHoldCompletionTimingValid(100.0, 100.0, 0.5f));
    TestFalse(TEXT("an early completion cannot bypass dwell"),
              Rules::IsHoldCompletionTimingValid(100.0, 100.46, 0.5f));
    TestTrue(TEXT("the authored dwell boundary completes"),
             Rules::IsHoldCompletionTimingValid(100.0, 100.5, 0.5f));
    TestFalse(TEXT("an abandoned hold cannot be replayed later"),
              Rules::IsHoldCompletionTimingValid(
                  100.0,
                  100.5 + Rules::HoldCompletionGraceSeconds + 0.01,
                  0.5f));
    TestFalse(TEXT("a reversed authority clock fails closed"),
              Rules::IsHoldCompletionTimingValid(101.0, 100.0, 0.5f));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicContextActionProjectionCapTest,
    "Mythic.World.Entity.ContextActions.DeterministicOfferCap",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicContextActionProjectionCapTest::RunTest(
    const FString & /*Parameters*/) {
    using namespace MythicContextActionProjectionTests;
    UObject *Outer = GetTransientPackage();
    FMythicContextActionOffer Low = MakeOffer(
        *Outer, TAG_ContextActionProjectionTestLow, 1);
    FMythicContextActionOffer High = MakeOffer(
        *Outer, TAG_ContextActionProjectionTestHigh, 10);
    FMythicContextActionOffer Mid = MakeOffer(
        *Outer, TAG_ContextActionProjectionTestMid, 5);

    TArray<FMythicContextActionOffer> Selected;
    TestTrue(TEXT("the first valid offer enters the bounded set"),
             FMythicContextActionProjectionRules::TryInsertBoundedOffer(
                 Low, 2, Selected));
    TestTrue(TEXT("a second valid offer fills the bounded set"),
             FMythicContextActionProjectionRules::TryInsertBoundedOffer(
                 High, 2, Selected));
    TestTrue(TEXT("a higher-priority overflow offer replaces the worst row"),
             FMythicContextActionProjectionRules::TryInsertBoundedOffer(
                 Mid, 2, Selected));
    TestEqual(TEXT("projection never exceeds its fixed cap"), Selected.Num(), 2);
    TestTrue(TEXT("the highest priority action survives"),
             Selected.ContainsByPredicate(
                 [](const FMythicContextActionOffer &Offer) {
                     return Offer.GetActionTag()
                            == TAG_ContextActionProjectionTestHigh;
                 }));
    TestTrue(TEXT("the next-highest priority action survives"),
             Selected.ContainsByPredicate(
                 [](const FMythicContextActionOffer &Offer) {
                     return Offer.GetActionTag()
                            == TAG_ContextActionProjectionTestMid;
                 }));
    TestFalse(TEXT("the displaced low-priority action is absent"),
              Selected.ContainsByPredicate(
                  [](const FMythicContextActionOffer &Offer) {
                      return Offer.GetActionTag()
                             == TAG_ContextActionProjectionTestLow;
                  }));

    UMythicContextActionProjectionPolicy *InvalidPolicy =
        NewObject<UMythicContextActionProjectionPolicy>(Outer);
    InvalidPolicy->MaximumProjectedOffers =
        FMythicContextActionProjectionRules::HardMaximumProjectedOffers + 1;
    TestFalse(TEXT("an authored cap above the native transport ceiling fails closed"),
              FMythicContextActionProjectionRules::BuildRuntimePolicy(
                  InvalidPolicy).bValid);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicContextActionStaleInstanceTest,
    "Mythic.World.Entity.ContextActions.StaleGenerationRejected",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicContextActionStaleInstanceTest::RunTest(
    const FString & /*Parameters*/) {
    using namespace MythicContextActionProjectionTests;
    FScopedAuthorityWorld Fixture;
    UWorld *World = Fixture.Get();
    UMythicEntityPresentationRegistry *Registry =
        World ? World->GetSubsystem<UMythicEntityPresentationRegistry>()
              : nullptr;
    if (!World || !Registry) {
        AddError(TEXT("the stale-instance authority fixture could not initialize"));
        return false;
    }

    const FMythicEntityPresentationInstance Current =
        Registry->AllocateAuthorityInstance(MakeEntityId(1));
    const FMythicEntityPresentationInstance Stale(
        Current.Handle, Current.EmbodimentGeneration + 1);
    TestTrue(TEXT("the fixture allocated a current public embodiment"),
             Current.IsValid());
    TestFalse(TEXT("a stale generation never equals the resolved embodiment"),
              FMythicContextActionProjectionRules::IsExactResolvedSubject(
                  Stale, Current));
    TestTrue(TEXT("the exact handle-generation pair remains eligible"),
             FMythicContextActionProjectionRules::IsExactResolvedSubject(
                 Current, Current));
    TestNull(TEXT("the stale generation cannot resolve through the registry"),
             Registry->ResolvePresentationComponent(Stale));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicContextActionOwnerIsolationRevocationTest,
    "Mythic.World.Entity.ContextActions.OwnerOnlyIsolationAndRevocation",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicContextActionOwnerIsolationRevocationTest::RunTest(
    const FString & /*Parameters*/) {
    using namespace MythicContextActionProjectionTests;

    UClass *GrantClass = UMythicEntityActionGrantComponent::StaticClass();
    GrantClass->SetUpRuntimeReplicationData();
    const FProperty *ReplicatedGrantsProperty = FindFProperty<FProperty>(
        GrantClass, TEXT("ReplicatedGrants"));
    TestNotNull(TEXT("the grant fast array is reflected for replication"),
                ReplicatedGrantsProperty);
    TArray<FLifetimeProperty> LifetimeProperties;
    const UObject *GrantCDO =
        GetDefault<UMythicEntityActionGrantComponent>();
    GrantCDO->GetLifetimeReplicatedProps(LifetimeProperties);
    const FLifetimeProperty *GrantLifetime =
        ReplicatedGrantsProperty
            ? LifetimeProperties.FindByPredicate(
                  [ReplicatedGrantsProperty](const FLifetimeProperty &Entry) {
                      return Entry.RepIndex
                             == ReplicatedGrantsProperty->RepIndex;
                  })
            : nullptr;
    TestNotNull(TEXT("the grant fast array has a lifetime replication rule"),
                GrantLifetime);
    TestEqual(TEXT("grants replicate only to their owning connection"),
              GrantLifetime ? GrantLifetime->Condition : COND_None,
              COND_OwnerOnly);

    FScopedAuthorityWorld Fixture;
    UWorld *World = Fixture.Get();
    UMythicEntityPresentationRegistry *Registry =
        World ? World->GetSubsystem<UMythicEntityPresentationRegistry>()
              : nullptr;
    AActor *OwnerA = World ? World->SpawnActor<AActor>() : nullptr;
    AActor *OwnerB = World ? World->SpawnActor<AActor>() : nullptr;
    if (!World || !Registry || !OwnerA || !OwnerB) {
        AddError(TEXT("the owner-isolation authority fixture could not initialize"));
        return false;
    }

    UMythicEntityActionGrantComponent *GrantsA =
        CreateGrantComponent(*OwnerA, TEXT("OwnerAGrants"));
    UMythicEntityActionGrantComponent *GrantsB =
        CreateGrantComponent(*OwnerB, TEXT("OwnerBGrants"));
    FMythicEntityPresentationInstance Subject;
    AMythicNPCCharacter *Provider =
        SpawnActivatedProvider(*World, 2, Subject);
    if (!GrantsA || !GrantsB || !Provider || !Subject.IsValid()) {
        AddError(TEXT("the owner-isolation provider fixture could not initialize"));
        return false;
    }
    TArray<FMythicAuthorityContextActionOffer> Offers;
    Offers.Add(MakeBoundOffer(
        *Provider, TAG_ContextActionProjectionTestHigh, 10));
    TestTrue(TEXT("authority writes the grant only to owner A"),
             GrantsA->AuthorityReplaceBoundContextActionOffers(
                 Subject, Provider, Offers, 4));

    FMythicReplicatedContextActionGrant Found;
    TestTrue(TEXT("owner A can query its exact current grant"),
             GrantsA->FindActionGrant(
                 Subject, TAG_ContextActionProjectionTestHigh, Found));
    TestFalse(TEXT("owner B has no cross-owner view of A's grant"),
              GrantsB->FindActionGrant(
                  Subject, TAG_ContextActionProjectionTestHigh, Found));
    TestEqual(TEXT("focus loss revokes every row for the exact subject"),
              GrantsA->AuthorityRevokeSubjectGrants(Subject), 1);
    TestFalse(TEXT("a revoked lease is no longer queryable"),
              GrantsA->FindActionGrant(
                  Subject, TAG_ContextActionProjectionTestHigh, Found));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicContextActionIssuerNonceConsumptionTest,
    "Mythic.World.Entity.ContextActions.IssuerNonceAtomicConsumption",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicContextActionIssuerNonceConsumptionTest::RunTest(
    const FString & /*Parameters*/) {
    using namespace MythicContextActionProjectionTests;

    FScopedAuthorityWorld Fixture;
    UWorld *World = Fixture.Get();
    AActor *Owner = World ? World->SpawnActor<AActor>() : nullptr;
    UMythicEntityActionGrantComponent *Grants = Owner
        ? CreateGrantComponent(*Owner, TEXT("NonceBoundGrants")) : nullptr;
    FMythicEntityPresentationInstance Subject;
    AMythicNPCCharacter *Provider = World
        ? SpawnActivatedProvider(*World, 11, Subject) : nullptr;
    if (!World || !Owner || !Grants || !Provider || !Subject.IsValid()) {
        AddError(TEXT("the nonce-bound issuer fixture could not initialize"));
        return false;
    }

    TArray<FMythicAuthorityContextActionOffer> Offers;
    Offers.Add(MakeBoundOffer(
        *Provider, TAG_ContextActionProjectionTestHigh, 10));
    UMythicContextActionDefinition *Definition =
        Offers[0].Offer.Definition;
    TestTrue(TEXT("an exact provider-bound offer mints a lease"),
             Grants->AuthorityReplaceBoundContextActionOffers(
                 Subject, Provider, Offers, 4));

    FMythicReplicatedContextActionGrant Grant;
    TestTrue(TEXT("the minted lease is queryable"),
             Grants->FindActionGrant(
                 Subject, TAG_ContextActionProjectionTestHigh, Grant));
    const uint32 FirstNonce = Grant.OfferRevision;
    TestTrue(TEXT("the wire carries a nonzero opaque nonce"),
             FirstNonce != 0);
    TestTrue(TEXT("the opaque nonce is not the provider revision"),
             FirstNonce != static_cast<uint32>(Offers[0].Offer.SourceRevision));

    const double InitialWorldSeconds = World->GetTimeSeconds();
    TestTrue(TEXT("an unexpired lease refresh updates transport state"),
             Grants->AuthorityReplaceBoundContextActionOffers(
                 Subject, Provider, Offers, 4,
                 InitialWorldSeconds + 100.0));
    TestTrue(TEXT("the refreshed lease remains queryable"),
             Grants->FindActionGrant(
                 Subject, TAG_ContextActionProjectionTestHigh, Grant));
    TestEqual(TEXT("an unchanged unexpired issuer keeps its hold nonce"),
              Grant.OfferRevision, FirstNonce);

    // Advance the authority clock without ticking the expiry timer, reproducing a hitch where refresh wins the next
    // callback. Renewal must mint a new nonce rather than resurrecting the client's already-expired lease.
    World->TimeSeconds = InitialWorldSeconds + 101.0;
    TestTrue(TEXT("an expired but unpruned row is replaced"),
             Grants->AuthorityReplaceBoundContextActionOffers(
                 Subject, Provider, Offers, 4,
                 InitialWorldSeconds + 200.0));
    TestTrue(TEXT("the replacement lease is queryable"),
             Grants->FindActionGrant(
                 Subject, TAG_ContextActionProjectionTestHigh, Grant));
    const uint32 RenewedNonce = Grant.OfferRevision;
    TestTrue(TEXT("expired authority nonces are never resurrected"),
             RenewedNonce != 0 && RenewedNonce != FirstNonce);

    UObject *ResolvedProvider = nullptr;
    UMythicContextActionDefinition *ResolvedDefinition = nullptr;
    uint32 ResolvedProviderRevision = 0;
    TestTrue(TEXT("the exact nonce resolves only to its issuing entry"),
             Grants->AuthorityResolveActionGrantBinding(
                 Subject, TAG_ContextActionProjectionTestHigh, RenewedNonce,
                 ResolvedProvider, ResolvedDefinition,
                 ResolvedProviderRevision));
    TestTrue(TEXT("the retained provider identity is exact"),
             ResolvedProvider == Provider);
    TestTrue(TEXT("the retained definition identity is exact"),
             ResolvedDefinition == Definition);
    TestEqual(TEXT("the private provider revision stays in the ledger"),
              ResolvedProviderRevision,
              static_cast<uint32>(Offers[0].Offer.SourceRevision));

    UObject *ConsumedProvider = nullptr;
    UMythicContextActionDefinition *ConsumedDefinition = nullptr;
    uint32 ConsumedProviderRevision = 0;
    TestTrue(TEXT("the issuing entry is consumed atomically"),
             Grants->AuthorityConsumeActionGrantBinding(
                 Subject, TAG_ContextActionProjectionTestHigh, RenewedNonce,
                 ConsumedProvider, ConsumedDefinition,
                 ConsumedProviderRevision));
    TestTrue(TEXT("consume returns the exact retained issuer"),
             ConsumedProvider == Provider
                 && ConsumedDefinition == Definition);
    TestFalse(TEXT("the consumed public lease is gone"),
              Grants->FindActionGrant(
                  Subject, TAG_ContextActionProjectionTestHigh, Grant));
    TestFalse(TEXT("the exact nonce cannot be consumed twice"),
              Grants->AuthorityConsumeActionGrantBinding(
                  Subject, TAG_ContextActionProjectionTestHigh, RenewedNonce,
                  ConsumedProvider, ConsumedDefinition,
                  ConsumedProviderRevision));

    TestTrue(TEXT("a fresh projection after consume mints a new lease"),
             Grants->AuthorityReplaceBoundContextActionOffers(
                 Subject, Provider, Offers, 4));
    TestTrue(TEXT("the fresh lease is queryable"),
             Grants->FindActionGrant(
                 Subject, TAG_ContextActionProjectionTestHigh, Grant));
    const uint32 SecondNonce = Grant.OfferRevision;
    TestTrue(TEXT("consumed nonces are never reused"),
             SecondNonce != 0 && SecondNonce != RenewedNonce);

    Definition->MaximumRangeCentimeters += 1.0f;
    TestFalse(TEXT("definition drift invalidates the retained signature"),
              Grants->AuthorityResolveActionGrantBinding(
                  Subject, TAG_ContextActionProjectionTestHigh, SecondNonce,
                  ResolvedProvider, ResolvedDefinition,
                  ResolvedProviderRevision));
    TestFalse(TEXT("signature drift revokes the stale public lease"),
              Grants->FindActionGrant(
                  Subject, TAG_ContextActionProjectionTestHigh, Grant));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicContextActionDuplicateIssuerTest,
    "Mythic.World.Entity.ContextActions.DuplicateIssuerFailsClosed",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicContextActionDuplicateIssuerTest::RunTest(
    const FString & /*Parameters*/) {
    using namespace MythicContextActionProjectionTests;

    AddExpectedMessagePlain(
        TEXT("Revoked ambiguous context action"), ELogVerbosity::Warning,
        EAutomationExpectedMessageFlags::Contains, 4);
    AddExpectedMessagePlain(
        TEXT("provider-bound context-action rows because the authority input bound is"),
        ELogVerbosity::Warning,
        EAutomationExpectedMessageFlags::Contains, 1);

    FScopedAuthorityWorld Fixture;
    UWorld *World = Fixture.Get();
    AActor *Owner = World ? World->SpawnActor<AActor>() : nullptr;
    UMythicEntityActionGrantComponent *Grants = Owner
        ? CreateGrantComponent(*Owner, TEXT("DuplicateIssuerGrants")) : nullptr;
    FMythicEntityPresentationInstance Subject;
    AMythicNPCCharacter *Provider = World
        ? SpawnActivatedProvider(*World, 21, Subject) : nullptr;
    if (!World || !Owner || !Grants || !Provider || !Subject.IsValid()) {
        AddError(TEXT("the duplicate-issuer fixture could not initialize"));
        return false;
    }

    TArray<FMythicAuthorityContextActionOffer> DuplicateOffers;
    DuplicateOffers.Add(MakeBoundOffer(
        *Provider, TAG_ContextActionProjectionTestMid, 5));
    DuplicateOffers.Add(MakeBoundOffer(
        *Provider, TAG_ContextActionProjectionTestMid, 6));
    TestFalse(TEXT("duplicate same-tag issuer rows do not mint a lease"),
              Grants->AuthorityReplaceBoundContextActionOffers(
                  Subject, Provider, DuplicateOffers, 4));

    FMythicReplicatedContextActionGrant Grant;
    TestFalse(TEXT("ambiguous provider output fails closed regardless of order"),
              Grants->FindActionGrant(
                  Subject, TAG_ContextActionProjectionTestMid, Grant));
    Algo::Reverse(DuplicateOffers);
    TestFalse(TEXT("reversing duplicate rows cannot select an issuer"),
              Grants->AuthorityReplaceBoundContextActionOffers(
                  Subject, Provider, DuplicateOffers, 4));
    TestFalse(TEXT("the reversed ambiguous set remains absent"),
              Grants->FindActionGrant(
                  Subject, TAG_ContextActionProjectionTestMid, Grant));

    TArray<FMythicAuthorityContextActionOffer> HiddenDuplicateOffers;
    FMythicAuthorityContextActionOffer Hidden = MakeBoundOffer(
        *Provider, TAG_ContextActionProjectionTestMid, 5);
    Hidden.Offer.Availability = EMythicContextActionAvailability::Hidden;
    HiddenDuplicateOffers.Add(MoveTemp(Hidden));
    HiddenDuplicateOffers.Add(MakeBoundOffer(
        *Provider, TAG_ContextActionProjectionTestMid, 6));
    TestFalse(TEXT("a hidden same-tag claim poisons available issuance"),
              Grants->AuthorityReplaceBoundContextActionOffers(
                  Subject, Provider, HiddenDuplicateOffers, 4));
    TestFalse(TEXT("hidden ambiguity never leaves a public lease"),
              Grants->FindActionGrant(
                  Subject, TAG_ContextActionProjectionTestMid, Grant));
    Algo::Reverse(HiddenDuplicateOffers);
    TestFalse(TEXT("hidden ambiguity is independent of provider row order"),
              Grants->AuthorityReplaceBoundContextActionOffers(
                  Subject, Provider, HiddenDuplicateOffers, 4));
    TestFalse(TEXT("reversed hidden ambiguity remains absent"),
              Grants->FindActionGrant(
                  Subject, TAG_ContextActionProjectionTestMid, Grant));

    TArray<FMythicAuthorityContextActionOffer> OversizedOffers;
    OversizedOffers.Init(
        MakeBoundOffer(*Provider, TAG_ContextActionProjectionTestHigh, 10),
        (FMythicContextActionProjectionRules::HardMaximumProviderComponents
         + 1)
                * FMythicContextActionProjectionRules::HardMaximumOffersPerProvider
            + 1);
    TestFalse(TEXT("oversized authority input rejects wholesale instead of truncating"),
              Grants->AuthorityReplaceBoundContextActionOffers(
                  Subject, Provider, OversizedOffers, 4));
    TestFalse(TEXT("an overflow suffix cannot hide an ambiguous claim"),
              Grants->FindActionGrant(
                  Subject, TAG_ContextActionProjectionTestHigh, Grant));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicContextActionRegistryBarrierRevocationTest,
    "Mythic.World.Entity.ContextActions.RegistryAndRestoreBarrierRevocation",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicContextActionRegistryBarrierRevocationTest::RunTest(
    const FString & /*Parameters*/) {
    using namespace MythicContextActionProjectionTests;

    FScopedAuthorityWorld Fixture;
    UWorld *World = Fixture.Get();
    UMythicEntityPresentationRegistry *Registry = World
        ? World->GetSubsystem<UMythicEntityPresentationRegistry>() : nullptr;
    AActor *Owner = World ? World->SpawnActor<AActor>() : nullptr;
    UMythicEntityActionGrantComponent *Grants = Owner
        ? CreateGrantComponent(*Owner, TEXT("RegistryBarrierGrants")) : nullptr;
    FMythicEntityPresentationInstance SubjectA;
    FMythicEntityPresentationInstance SubjectB;
    AMythicNPCCharacter *ProviderA = World
        ? SpawnActivatedProvider(*World, 31, SubjectA) : nullptr;
    AMythicNPCCharacter *ProviderB = World
        ? SpawnActivatedProvider(*World, 32, SubjectB) : nullptr;
    if (!World || !Registry || !Owner || !Grants || !ProviderA || !ProviderB
        || !SubjectA.IsValid() || !SubjectB.IsValid()) {
        AddError(TEXT("the registry-barrier fixture could not initialize"));
        return false;
    }

    TArray<FMythicAuthorityContextActionOffer> OffersA;
    OffersA.Add(MakeBoundOffer(
        *ProviderA, TAG_ContextActionProjectionTestHigh, 10));
    TArray<FMythicAuthorityContextActionOffer> OffersB;
    OffersB.Add(MakeBoundOffer(
        *ProviderB, TAG_ContextActionProjectionTestMid, 5));
    TestTrue(TEXT("subject A receives its exact grant"),
             Grants->AuthorityReplaceBoundContextActionOffers(
                 SubjectA, ProviderA, OffersA, 4));
    TestTrue(TEXT("subject B receives an independent exact grant"),
             Grants->AuthorityReplaceBoundContextActionOffers(
                 SubjectB, ProviderB, OffersB, 4));

    ProviderA->GetEntityPresentationComponent_Implementation()
        ->AuthorityDeactivateEmbodiment();
    FMythicReplicatedContextActionGrant Grant;
    TestFalse(TEXT("unregister revokes subject A immediately"),
              Grants->FindActionGrant(
                  SubjectA, TAG_ContextActionProjectionTestHigh, Grant));
    TestTrue(TEXT("exact-instance unregister preserves subject B"),
             Grants->FindActionGrant(
                 SubjectB, TAG_ContextActionProjectionTestMid, Grant));

    Registry->ResetAuthorityPresentationEpoch();
    TestFalse(TEXT("the restore epoch barrier revokes every remaining exact grant"),
              Grants->FindActionGrant(
                  SubjectB, TAG_ContextActionProjectionTestMid, Grant));
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

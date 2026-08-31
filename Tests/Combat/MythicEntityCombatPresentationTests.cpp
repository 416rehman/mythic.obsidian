#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "AI/MythicTags_AI.h"
#include "AI/NPCs/MythicNPCCharacter.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GAS/Combat/MythicCombatPresentationProjection.h"
#include "GAS/Combat/MythicEntityCombatPresentationComponent.h"
#include "GAS/MythicTags_GAS.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Player/MythicPlayerState.h"
#include "Settings/MythicCombatSettings.h"
#include "UObject/UnrealType.h"
#include "World/Entity/MythicEntityPresentationComponent.h"
#include "World/Entity/MythicEntityPresentationRegistry.h"
#include "World/Entity/MythicEntityPresentationTags.h"

namespace MythicEntityCombatPresentationTests {

    FMythicEntityId MakeEntityId(const uint32 Salt) {
        return FMythicEntityId::FromAuthorityGuid(EMythicEntityDomain::LivingWorld,
                                                  FGuid(0x31000000u + Salt, 0x42000000u + Salt, 0x53000000u + Salt, 0x64000000u + Salt));
    }

    class FScopedCombatPresentationWorld final {
    public:
        FScopedCombatPresentationWorld() {
            Values = UWorld::InitializationValues()
                         .CreatePhysicsScene(false)
                         .ShouldSimulatePhysics(false)
                         .EnableTraceCollision(false)
                         .CreateNavigation(false)
                         .CreateAISystem(false);
            World = UWorld::CreateWorld(EWorldType::PIE, false, MakeUniqueObjectName(nullptr, UWorld::StaticClass(), TEXT("EntityCombatPresentationTest")),
                                        nullptr, true, ERHIFeatureLevel::Num, &Values, true);
            if (World) {
                World->SetPlayInEditorInitialNetMode(NM_ListenServer);
                World->InitWorld(Values);
            }
        }

        ~FScopedCombatPresentationWorld() {
            if (World) {
                World->DestroyWorld(false);
            }
        }

        UWorld *Get() const { return World; }

    private:
        UWorld::InitializationValues Values;
        UWorld *World = nullptr;
    };

    UMythicEntityCombatPresentationComponent *CreateComponent(UWorld *World) {
        AActor *Owner = World ? World->SpawnActor<AActor>() : nullptr;
        if (!Owner) {
            return nullptr;
        }
        UMythicEntityCombatPresentationComponent *Component = NewObject<UMythicEntityCombatPresentationComponent>(Owner);
        Owner->AddInstanceComponent(Component);
        Component->RegisterComponent();
        return Component;
    }

    FMythicEntityCombatPresentationAuthorityRequest MakeRequest(const FMythicEntityPresentationInstance &Subject, const float ViewerPressure,
                                                                const float SubjectPressure, const uint32 Revision, const double Expiry = 0.0) {
        FMythicEntityCombatPresentationAuthorityRequest Request;
        Request.Subject = Subject;
        Request.AssessmentInputs.bAssessmentPermitted = true;
        Request.AssessmentInputs.bCombatCapable = true;
        Request.AssessmentInputs.ViewerEffectivePressure = ViewerPressure;
        Request.AssessmentInputs.SubjectEffectivePressure = SubjectPressure;
        Request.PresentedCombatRank = EMythicPresentedCombatRank::Standard;
        Request.bRankPresentationPermitted = true;
        Request.SourceRevision = Revision;
        Request.ExpiryServerTimeSeconds = Expiry;
        return Request;
    }

    bool SetEnemyTierForTest(AMythicNPCCharacter *NPC,
                             const FGameplayTag EnemyTier) {
        FStructProperty *TierProperty = FindFProperty<FStructProperty>(
            AMythicNPCCharacter::StaticClass(), TEXT("EnemyTier"));
        FGameplayTag *TierValue = TierProperty && NPC
            ? TierProperty->ContainerPtrToValuePtr<FGameplayTag>(NPC)
            : nullptr;
        if (!TierValue) {
            return false;
        }
        *TierValue = EnemyTier;
        return true;
    }

} // namespace MythicEntityCombatPresentationTests

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicEntityCombatPresentationPrivacyTest, "Mythic.Combat.EntityPresentation.OwnerPrivateContract",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicEntityCombatPresentationPrivacyTest::RunTest(const FString & /*Parameters*/) {
    const UScriptStruct *WireStruct = FMythicReplicatedEntityCombatPresentation::StaticStruct();
    TestNotNull(TEXT("the viewer-safe combat DTO is reflected"), WireStruct);
    TestNull(TEXT("canonical entity identity is absent"), FindFProperty<FProperty>(WireStruct, TEXT("EntityId")));
    TestNull(TEXT("viewer raw pressure is absent"), FindFProperty<FProperty>(WireStruct, TEXT("ViewerEffectivePressure")));
    TestNull(TEXT("subject raw pressure is absent"), FindFProperty<FProperty>(WireStruct, TEXT("SubjectEffectivePressure")));
    TestNull(TEXT("raw combat rank is absent"), FindFProperty<FProperty>(WireStruct, TEXT("Rank")));
    TestNull(TEXT("the legacy boss bit is absent"), FindFProperty<FProperty>(WireStruct, TEXT("bBoss")));
    const FProperty *PresentedRank = FindFProperty<FProperty>(WireStruct, TEXT("PresentedCombatRank"));
    TestTrue(TEXT("only the sanitized categorical rank is Blueprint-readable"),
             PresentedRank && PresentedRank->HasAnyPropertyFlags(CPF_BlueprintVisible));

    const FProperty *SourceRevision = FindFProperty<FProperty>(WireStruct, TEXT("SourceRevision"));
    TestNotNull(TEXT("the stale-write revision is represented"), SourceRevision);
    TestFalse(TEXT("the opaque source revision is not Blueprint-visible"), SourceRevision && SourceRevision->HasAnyPropertyFlags(CPF_BlueprintVisible));

    TestNull(TEXT("raw authority projection cannot be invoked from Blueprint"),
             UMythicEntityCombatPresentationComponent::StaticClass()->FindFunctionByName(TEXT("AuthoritySetCombatPresentation")));
    TestNotNull(TEXT("the safe exact-subject query is callable from Blueprint"),
                UMythicEntityCombatPresentationComponent::StaticClass()->FindFunctionByName(TEXT("GetCombatPresentationForSubject")));

    TestNull(TEXT("per-viewer transports cannot override global combat thresholds"),
             FindFProperty<FProperty>(UMythicEntityCombatPresentationComponent::StaticClass(), TEXT("ThreatThresholds")));
    const FProperty *Thresholds = FindFProperty<FProperty>(
        UMythicCombatSettings::StaticClass(),
        TEXT("CombatPresentationThreatThresholds"));
    TestTrue(TEXT("combat threat thresholds are authored once in canonical combat settings"),
             Thresholds && Thresholds->HasAllPropertyFlags(CPF_Config | CPF_Edit));
    const FProperty *ReplicatedView = FindFProperty<FProperty>(UMythicEntityCombatPresentationComponent::StaticClass(), TEXT("ReplicatedPresentations"));
    TestTrue(TEXT("the FastArray transport is transient replicated state"), ReplicatedView && ReplicatedView->HasAllPropertyFlags(CPF_Net | CPF_Transient));
    TestFalse(TEXT("ephemeral combat reads are never SaveGame fields"), ReplicatedView && ReplicatedView->HasAnyPropertyFlags(CPF_SaveGame));

    const AMythicPlayerState *PlayerStateCDO = GetDefault<AMythicPlayerState>();
    TestNotNull(TEXT("PlayerState owns combat presentation as a default subobject"),
                PlayerStateCDO ? PlayerStateCDO->GetEntityCombatPresentationComponent() : nullptr);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicEntityCombatPresentationAuthorityTest, "Mythic.Combat.EntityPresentation.ClassificationLeaseAndRevision",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicEntityCombatPresentationAuthorityTest::RunTest(const FString & /*Parameters*/) {
    using namespace MythicEntityCombatPresentationTests;

    FScopedCombatPresentationWorld Fixture;
    UWorld *World = Fixture.Get();
    UMythicEntityPresentationRegistry *Registry = World ? World->GetSubsystem<UMythicEntityPresentationRegistry>() : nullptr;
    UMythicEntityCombatPresentationComponent *Component = CreateComponent(World);
    if (!World || !Registry || !Component) {
        AddError(TEXT("combat-presentation authority fixture failed"));
        return false;
    }

    const FMythicEntityId EntityId = MakeEntityId(1);
    const FMythicEntityPresentationInstance FirstInstance = Registry->AllocateAuthorityInstance(EntityId);
    const double Expiry = static_cast<double>(World->GetTimeSeconds()) + 30.0;
    FMythicEntityCombatPresentationAuthorityRequest Request = MakeRequest(FirstInstance, 100.0f, 225.0f, 1, Expiry);
    Request.bExactCombatLevelPermitted = true;
    Request.SubjectCombatLevel = 37;

    uint32 LastNativeRevision = 0;
    int32 RevisionEdges = 0;
    Component->OnNativeCombatPresentationRevision().AddLambda([&LastNativeRevision, &RevisionEdges](const uint32 Revision) {
        LastNativeRevision = Revision;
        ++RevisionEdges;
    });

    TestTrue(TEXT("authority accepts a valid classified lease"), Component->AuthoritySetCombatPresentation(Request));
    const FMythicReplicatedEntityCombatPresentation *View = Component->FindCurrentCombatPresentation(FirstInstance);
    TestNotNull(TEXT("the exact embodiment is queryable"), View);
    if (!View) {
        return false;
    }
    TestEqual(TEXT("2.25 pressure is classified by combat as Deadly"), View->ThreatBand, EMythicThreatBand::Deadly);
    TestTrue(TEXT("permitted combat capability is present"), View->bCombatCapable);
    TestEqual(TEXT("a permitted standard rank is preserved"), View->PresentedCombatRank,
              EMythicPresentedCombatRank::Standard);
    TestTrue(TEXT("the separately permitted exact level is present"), View->bHasExactCombatLevel);
    TestEqual(TEXT("the exact combat level is preserved"), View->ExactCombatLevel, 37);
    TestEqual(TEXT("one semantic mutation emits one native revision edge"), RevisionEdges, 1);
    TestEqual(TEXT("the delegate revision matches the query revision"), static_cast<int32>(LastNativeRevision),
              Component->GetLocalCombatPresentationRevision());

    TestTrue(TEXT("an exact replay is idempotently accepted"), Component->AuthoritySetCombatPresentation(Request));
    TestEqual(TEXT("an exact replay emits no invalidation"), RevisionEdges, 1);

    Request.PresentedCombatRank = EMythicPresentedCombatRank::Elite;
    TestFalse(TEXT("a conflicting same-revision presented rank is rejected"),
              Component->AuthoritySetCombatPresentation(Request));
    TestEqual(TEXT("a rejected rank conflict emits no invalidation"), RevisionEdges, 1);
    Request.PresentedCombatRank = EMythicPresentedCombatRank::Standard;

    Request.AssessmentInputs.SubjectEffectivePressure = 400.0f;
    TestFalse(TEXT("a conflicting same-revision write is rejected"), Component->AuthoritySetCombatPresentation(Request));
    TestEqual(TEXT("a rejected conflict emits no invalidation"), RevisionEdges, 1);

    Request.SourceRevision = 2;
    Request.AssessmentInputs.SubjectEffectivePressure = 100.0f;
    Request.AssessmentInputs.Rank = EMythicCombatThreatRank::WorldBoss;
    Request.AssessmentInputs.bRankKnownToViewer = true;
    Request.PresentedCombatRank = EMythicPresentedCombatRank::Boss;
    Request.bRankPresentationPermitted = false;
    Request.bExactCombatLevelPermitted = false;
    TestTrue(TEXT("a conflicting raw rank is ignored while canonical rank presentation is withheld"),
             Component->AuthoritySetCombatPresentation(Request));
    View = Component->FindCurrentCombatPresentation(FirstInstance);
    TestEqual(TEXT("an unentitled canonical boss rank is sanitized to Unknown"),
              View ? View->PresentedCombatRank : EMythicPresentedCombatRank::WorldBoss,
              EMythicPresentedCombatRank::Unknown);
    TestEqual(TEXT("an unentitled private rank cannot impose a warning floor"),
              View ? View->ThreatBand : EMythicThreatBand::Unknown,
              EMythicThreatBand::None);

    Request.SourceRevision = 3;
    Request.bRankPresentationPermitted = true;
    TestTrue(TEXT("a newer authority revision replaces the lease"), Component->AuthoritySetCombatPresentation(Request));
    View = Component->FindCurrentCombatPresentation(FirstInstance);
    TestEqual(TEXT("a separately permitted boss rank is sanitized onto the wire"),
              View ? View->PresentedCombatRank : EMythicPresentedCombatRank::Unknown,
              EMythicPresentedCombatRank::Boss);
    TestEqual(TEXT("the canonical presented boss, not a conflicting raw input, supplies the Deadly floor"),
              View ? View->ThreatBand : EMythicThreatBand::Unknown,
              EMythicThreatBand::Deadly);
    TestFalse(TEXT("withheld exact level is cleared rather than retained"), View && View->bHasExactCombatLevel);
    TestEqual(TEXT("withheld exact value is zeroed"), View ? View->ExactCombatLevel : -1, 0);

    Request.SourceRevision = 4;
    Request.AssessmentInputs.bCombatCapable = false;
    Request.bExactCombatLevelPermitted = true;
    Request.SubjectCombatLevel = 99;
    TestTrue(TEXT("a newer known noncombatant projection is accepted"), Component->AuthoritySetCombatPresentation(Request));
    View = Component->FindCurrentCombatPresentation(FirstInstance);
    TestEqual(TEXT("noncombatants cannot retain a presented combat rank"),
              View ? View->PresentedCombatRank : EMythicPresentedCombatRank::WorldBoss,
              EMythicPresentedCombatRank::Unknown);
    TestFalse(TEXT("noncombatants cannot leak an otherwise permitted combat level"), View && View->bHasExactCombatLevel);
    TestEqual(TEXT("suppressed noncombatant level is zeroed"), View ? View->ExactCombatLevel : -1, 0);

    Request.SourceRevision = 1;
    TestFalse(TEXT("an older authority write cannot roll back the view"), Component->AuthoritySetCombatPresentation(Request));

    const FMythicEntityPresentationInstance ReplacementInstance = Registry->AllocateAuthorityInstance(EntityId);
    TestTrue(TEXT("rebinding creates a distinct exact public instance"), ReplacementInstance.IsValid() && ReplacementInstance != FirstInstance);
    TestNull(TEXT("a new generation cannot alias the old combat lease"), Component->FindCurrentCombatPresentation(ReplacementInstance));
    Request.SourceRevision = 5;
    TestFalse(TEXT("authority rejects writes for a released old embodiment"), Component->AuthoritySetCombatPresentation(Request));

    const int32 Pruned = Component->AuthorityPruneExpiredCombatPresentations(Expiry + 1.0);
    TestEqual(TEXT("expiry prunes the ephemeral lease"), Pruned, 1);
    TestEqual(TEXT("the pruned view is no longer queryable"), Component->GetActiveCombatPresentationCount(), 0);
    TestEqual(TEXT("four accepted sets plus prune emitted five revision edges"), RevisionEdges, 5);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicEntityCombatPresentationReplacementTest, "Mythic.Combat.EntityPresentation.AuthoredThresholdsAndAtomicReplace",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicEntityCombatPresentationReplacementTest::RunTest(const FString & /*Parameters*/) {
    using namespace MythicEntityCombatPresentationTests;

    FScopedCombatPresentationWorld Fixture;
    UWorld *World = Fixture.Get();
    UMythicEntityPresentationRegistry *Registry = World ? World->GetSubsystem<UMythicEntityPresentationRegistry>() : nullptr;
    UMythicEntityCombatPresentationComponent *Component = CreateComponent(World);
    if (!World || !Registry || !Component) {
        AddError(TEXT("combat-presentation replacement fixture failed"));
        return false;
    }

    const FMythicEntityPresentationInstance First = Registry->AllocateAuthorityInstance(MakeEntityId(11));
    const FMythicEntityPresentationInstance Second = Registry->AllocateAuthorityInstance(MakeEntityId(12));
    TArray<FMythicEntityCombatPresentationAuthorityRequest> Requests;
    Requests.Add(MakeRequest(First, 100.0f, 150.0f, 1));
    Requests.Add(MakeRequest(Second, 100.0f, 320.0f, 1));

    TestTrue(TEXT("a valid complete authority view is accepted"), Component->AuthorityReplaceCombatPresentations(1, Requests));
    TestEqual(TEXT("both exact embodiments are present"), Component->GetActiveCombatPresentationCount(), 2);
    const FMythicReplicatedEntityCombatPresentation *FirstView = Component->FindCurrentCombatPresentation(First);
    const FMythicReplicatedEntityCombatPresentation *SecondView = Component->FindCurrentCombatPresentation(Second);
    TestEqual(TEXT("the canonical Risky boundary is used by authority"), FirstView ? FirstView->ThreatBand : EMythicThreatBand::Unknown,
              EMythicThreatBand::Risky);
    TestEqual(TEXT("the canonical Deadly boundary is used by authority"), SecondView ? SecondView->ThreatBand : EMythicThreatBand::Unknown,
              EMythicThreatBand::Deadly);

    TArray<FMythicEntityCombatPresentationAuthorityRequest> Duplicate = {Requests[0], Requests[0]};
    TestFalse(TEXT("duplicate subjects reject the whole replacement"), Component->AuthorityReplaceCombatPresentations(2, Duplicate));
    TestEqual(TEXT("a rejected replacement leaves the prior view intact"), Component->GetActiveCombatPresentationCount(), 2);

    FMythicEntityCombatPresentationAuthorityRequest InvalidExact = Requests[0];
    InvalidExact.SourceRevision = 2;
    InvalidExact.bExactCombatLevelPermitted = true;
    InvalidExact.SubjectCombatLevel = 0;
    TArray<FMythicEntityCombatPresentationAuthorityRequest> Invalid = {InvalidExact};
    TestFalse(TEXT("permission without a valid exact level fails closed"), Component->AuthorityReplaceCombatPresentations(2, Invalid));
    TestEqual(TEXT("invalid exact data cannot partially replace the view"), Component->GetActiveCombatPresentationCount(), 2);

    TArray<FMythicEntityCombatPresentationAuthorityRequest> Reduced = {Requests[1]};
    TestFalse(TEXT("a reused whole-view revision cannot revoke omissions"), Component->AuthorityReplaceCombatPresentations(1, Reduced));
    TestEqual(TEXT("a conflicting replay leaves both subjects intact"), Component->GetActiveCombatPresentationCount(), 2);
    TestTrue(TEXT("a smaller complete view atomically revokes omissions"), Component->AuthorityReplaceCombatPresentations(2, Reduced));
    TestNull(TEXT("the omitted exact embodiment is revoked"), Component->FindCurrentCombatPresentation(First));
    TestNotNull(TEXT("the retained exact embodiment remains"), Component->FindCurrentCombatPresentation(Second));
    TestTrue(TEXT("explicit exact-subject revoke succeeds"), Component->AuthorityRevokeCombatPresentation(Second));
    TestEqual(TEXT("all ephemeral reads are now cleared"), Component->GetActiveCombatPresentationCount(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicCombatPresentationProjectionRulesTest,
    "Mythic.Combat.EntityPresentation.AuthorityProjectionRules",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicCombatPresentationProjectionRulesTest::RunTest(
    const FString & /*Parameters*/) {
    FMythicCombatPresentationProjectionPolicy Policy;
    TestTrue(TEXT("the default authority focus policy is internally valid"),
             Policy.IsValid());

    FMythicCombatPressureSnapshot Baseline;
    Baseline.bCombatCapable = true;
    Baseline.ExpectedDamagePerHit = 10.0f;
    Baseline.AttacksPerSecond = 1.0f;
    Baseline.CriticalDamageMultiplier = 1.0f;
    Baseline.OutgoingDamageMultiplier = 1.0f;
    Baseline.MaximumHealth = 100.0f;
    const double BaselinePressure =
        FMythicCombatPresentationProjectionRules::ComputeEffectivePressure(
            Baseline);
    TestTrue(TEXT("a complete combat sample produces finite pressure"),
             FMath::IsFinite(BaselinePressure) && BaselinePressure > 0.0f);

    FMythicCombatPressureSnapshot Scaled = Baseline;
    Scaled.ExpectedDamagePerHit *= 4.0f;
    Scaled.MaximumHealth *= 4.0f;
    const double ScaledPressure =
        FMythicCombatPresentationProjectionRules::ComputeEffectivePressure(
            Scaled);
    TestTrue(TEXT("four-times offense and survivability produce sixteen-times pressure"),
             FMath::IsNearlyEqual(ScaledPressure,
                                  BaselinePressure * 16.0, 0.001));

    Baseline.bCombatCapable = false;
    TestEqual(TEXT("a noncombatant never receives a numeric pressure"),
              FMythicCombatPresentationProjectionRules::ComputeEffectivePressure(
                  Baseline),
              0.0);
    const float InRange = FMath::Square(
        Policy.MaximumFocusRangeCentimeters - 1.0f);
    TestTrue(TEXT("exact visible in-range aim is eligible"),
             FMythicCombatPresentationProjectionRules::IsSpatiallyEligible(
                 InRange, Policy.MinimumFocusViewDot, true, Policy));
    TestFalse(TEXT("occlusion rejects the authority projection"),
              FMythicCombatPresentationProjectionRules::IsSpatiallyEligible(
                  InRange, 1.0f, false, Policy));
    TestFalse(TEXT("off-axis nominations reject the authority projection"),
              FMythicCombatPresentationProjectionRules::IsSpatiallyEligible(
                  InRange, Policy.MinimumFocusViewDot - 0.01f, true, Policy));
    TestFalse(TEXT("out-of-range nominations reject the authority projection"),
              FMythicCombatPresentationProjectionRules::IsSpatiallyEligible(
                  FMath::Square(Policy.MaximumFocusRangeCentimeters + 1.0f),
                  1.0f, true, Policy));

    TestEqual(TEXT("the first client focus edge is not delayed"),
              FMythicCombatPresentationProjectionRules::
                  GetRequestThrottleDelaySeconds(
                      10.0, -DBL_MAX,
                      Policy.MinimumClientRequestIntervalSeconds),
              0.0);
    TestTrue(TEXT("an early repeated edge is deferred"),
             FMythicCombatPresentationProjectionRules::
                     GetRequestThrottleDelaySeconds(
                         10.04, 10.0,
                         Policy.MinimumClientRequestIntervalSeconds)
                 > 0.0);
    TestEqual(TEXT("revision wrap preserves zero as invalid"),
              FMythicCombatPresentationProjectionRules::
                  AdvanceNonzeroRevision(MAX_uint32),
              1u);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicCombatPresentationRankAndCommitmentTest,
    "Mythic.Combat.EntityPresentation.AuthorityRankAndPublicCommitment",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicCombatPresentationRankAndCommitmentTest::RunTest(
    const FString & /*Parameters*/) {
    using namespace MythicEntityCombatPresentationTests;

    FScopedCombatPresentationWorld Fixture;
    UWorld *World = Fixture.Get();
    AMythicNPCCharacter *NPC = World
        ? World->SpawnActor<AMythicNPCCharacter>() : nullptr;
    APawn *ViewerPawn = World ? World->SpawnActor<APawn>() : nullptr;
    UAbilitySystemComponent *SubjectAbilitySystem =
        FMythicCombatPresentationProjectionRules::ResolveAbilitySystem(NPC);
    if (!World || !NPC || !ViewerPawn || !SubjectAbilitySystem) {
        AddError(TEXT("the authority rank fixture failed"));
        return false;
    }

    struct FExpectedRank {
        FGameplayTag EnemyTier;
        EMythicPresentedCombatRank PresentedRank;
    };
    const FExpectedRank ExpectedRanks[] = {
        {AI_TIER_NORMAL, EMythicPresentedCombatRank::Standard},
        {AI_TIER_SUPERIOR, EMythicPresentedCombatRank::Superior},
        {AI_TIER_ELITE, EMythicPresentedCombatRank::Elite},
        {AI_TIER_CHAMPION, EMythicPresentedCombatRank::Champion},
        {AI_TIER_BOSS, EMythicPresentedCombatRank::Boss},
    };
    for (const FExpectedRank &Expected : ExpectedRanks) {
        TestTrue(TEXT("the fixture can author the canonical enemy tier"),
                 SetEnemyTierForTest(NPC, Expected.EnemyTier));
        TestEqual(
            *FString::Printf(TEXT("%s maps to its player-facing rank"),
                             *Expected.EnemyTier.ToString()),
            FMythicCombatPresentationProjectionRules::
                ResolveAuthorityNpcPresentedCombatRank(NPC),
            Expected.PresentedRank);
    }
    TestTrue(TEXT("the fixture can clear an unknown enemy tier"),
             SetEnemyTierForTest(NPC, FGameplayTag()));
    TestEqual(TEXT("an unknown AI tier fails closed instead of inferring WorldBoss"),
              FMythicCombatPresentationProjectionRules::
                  ResolveAuthorityNpcPresentedCombatRank(NPC),
              EMythicPresentedCombatRank::Unknown);
    TestTrue(TEXT("the fixture can restore boss tier"),
             SetEnemyTierForTest(NPC, AI_TIER_BOSS));
    TestEqual(TEXT("ordinary AI boss tier never escalates to WorldBoss"),
              FMythicCombatPresentationProjectionRules::
                  ResolveAuthorityNpcPresentedCombatRank(NPC),
              EMythicPresentedCombatRank::Boss);

    TestFalse(TEXT("mere focus is not a public combat commitment"),
              FMythicCombatPresentationProjectionRules::
                  HasPublicCombatCommitment(
                      NPC, SubjectAbilitySystem, ViewerPawn));
    NPC->SetEngagedTarget(ViewerPawn);
    TestTrue(TEXT("an NPC visibly engaged with the viewer is committed"),
             FMythicCombatPresentationProjectionRules::
                 HasPublicCombatCommitment(
                     NPC, SubjectAbilitySystem, ViewerPawn));
    NPC->SetEngagedTarget(nullptr);
    SubjectAbilitySystem->AddLooseGameplayTag(GAS_STATE_INCOMBAT);
    TestTrue(TEXT("the replicated in-combat state is a public commitment"),
             FMythicCombatPresentationProjectionRules::
                 HasPublicCombatCommitment(
                     NPC, SubjectAbilitySystem, ViewerPawn));
    SubjectAbilitySystem->RemoveLooseGameplayTag(GAS_STATE_INCOMBAT);
    TestFalse(TEXT("commitment fails closed without a viewer pawn"),
              FMythicCombatPresentationProjectionRules::
                  HasPublicCombatCommitment(
                      NPC, SubjectAbilitySystem, nullptr));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicCombatPresentationDeactivationRevocationTest,
    "Mythic.Combat.EntityPresentation.DeactivationRevokesImmediately",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicCombatPresentationDeactivationRevocationTest::RunTest(
    const FString & /*Parameters*/) {
    using namespace MythicEntityCombatPresentationTests;

    FScopedCombatPresentationWorld Fixture;
    UWorld *World = Fixture.Get();
    UMythicEntityCombatPresentationComponent *Combat = CreateComponent(World);
    AActor *SubjectActor = World ? World->SpawnActor<AActor>() : nullptr;
    UMythicEntityPresentationComponent *Presentation = SubjectActor
        ? NewObject<UMythicEntityPresentationComponent>(SubjectActor)
        : nullptr;
    if (!World || !Combat || !SubjectActor || !Presentation) {
        AddError(TEXT("the deactivation revocation fixture failed"));
        return false;
    }
    SubjectActor->AddInstanceComponent(Presentation);
    Presentation->RegisterComponent();

    FMythicPublicIdentitySnapshot SafeIdentity;
    SafeIdentity.PublicKindTag =
        MythicEntityPresentationTags::EntityKindHumanoid;
    const FMythicEntityId EntityId = MakeEntityId(71);
    TestTrue(TEXT("authority prepares the exact subject embodiment"),
             Presentation->AuthorityPrepareEmbodiment(EntityId, SafeIdentity));
    TestTrue(TEXT("authority activates the exact subject embodiment"),
             Presentation->AuthorityActivateEmbodiment());
    const FMythicEntityPresentationInstance Subject =
        Presentation->GetPresentationInstance();
    TestTrue(TEXT("the activated subject has an exact public instance"),
             Subject.IsValid());

    FMythicEntityCombatPresentationAuthorityRequest Request =
        MakeRequest(Subject, 100.0f, 150.0f, 1, 30.0);
    TestTrue(TEXT("the owner-only combat lease is installed"),
             Combat->AuthoritySetCombatPresentation(Request));
    TestNotNull(TEXT("the lease is queryable before deactivation"),
                Combat->FindCurrentCombatPresentation(Subject));

    Presentation->AuthorityDeactivateEmbodiment();
    TestNull(TEXT("the registry deactivation edge revokes the exact lease immediately"),
             Combat->FindCurrentCombatPresentation(Subject));
    TestEqual(TEXT("no stale combat rows survive pool return"),
              Combat->GetActiveCombatPresentationCount(), 0);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

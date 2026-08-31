#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Player/MythicPlayerState.h"
#include "UObject/UnrealType.h"
#include "World/Entity/MythicEntityKnowledgeTags.h"
#include "World/Entity/MythicEntityPresentationRegistry.h"
#include "World/Entity/MythicEntityViewerKnowledgeComponent.h"
#include "World/Entity/MythicEntityViewerKnowledgeTypes.h"

namespace MythicViewerKnowledgeTests {

FMythicEntityId MakeEntityId(const uint32 Salt,
                             const EMythicEntityDomain Domain =
                                 EMythicEntityDomain::LivingWorld) {
    return FMythicEntityId::FromAuthorityGuid(
        Domain, FGuid(0x51000000u + Salt, 0x62000000u + Salt,
                      0x73000000u + Salt, 0x84000000u + Salt));
}

class FScopedKnowledgeWorld final {
public:
    FScopedKnowledgeWorld() {
        Values = UWorld::InitializationValues()
                     .CreatePhysicsScene(false)
                     .ShouldSimulatePhysics(false)
                     .EnableTraceCollision(false)
                     .CreateNavigation(false)
                     .CreateAISystem(false);
        World = UWorld::CreateWorld(
            EWorldType::PIE, false,
            MakeUniqueObjectName(nullptr, UWorld::StaticClass(),
                                 TEXT("ViewerKnowledgeTest")),
            nullptr, true, ERHIFeatureLevel::Num, &Values, true);
        if (World) {
            World->SetPlayInEditorInitialNetMode(NM_ListenServer);
            World->InitWorld(Values);
        }
    }

    ~FScopedKnowledgeWorld() {
        if (World) {
            World->DestroyWorld(false);
        }
    }

    UWorld *Get() const { return World; }

private:
    UWorld::InitializationValues Values;
    UWorld *World = nullptr;
};

} // namespace MythicViewerKnowledgeTests

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicEntityKnowledgePrivacyTest,
    "Mythic.World.Entity.ViewerKnowledge.BlueprintPrivacy",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicEntityKnowledgePrivacyTest::RunTest(
    const FString & /*Parameters*/) {
    const UScriptStruct *ViewStruct = FMythicEntityKnowledgeView::StaticStruct();
    TestNotNull(TEXT("the safe knowledge DTO is reflected"), ViewStruct);
    TestNull(TEXT("the safe DTO has no canonical entity identity"),
             FindFProperty<FProperty>(ViewStruct, TEXT("EntityId")));
    TestNull(TEXT("the safe DTO has no true faction"),
             FindFProperty<FProperty>(ViewStruct, TEXT("TrueFaction")));
    TestNull(TEXT("the safe DTO has no hidden intent"),
             FindFProperty<FProperty>(ViewStruct, TEXT("Intent")));
    TestNull(TEXT("the safe DTO has no exact relationship score"),
             FindFProperty<FProperty>(ViewStruct,
                                      TEXT("RelationshipScore")));

    const FProperty *DossierId = FindFProperty<FProperty>(
        FMythicEntityLearnedDossier::StaticStruct(), TEXT("EntityId"));
    const FProperty *BindingId = FindFProperty<FProperty>(
        FMythicReplicatedEntityRecognition::StaticStruct(),
        TEXT("EntityId"));
    TestNotNull(TEXT("the durable dossier is typed-ID keyed"), DossierId);
    TestNotNull(TEXT("the recognition binding carries an owner-only typed ID"),
                BindingId);
    TestFalse(TEXT("durable canonical identity is not Blueprint-visible"),
              DossierId
                  && DossierId->HasAnyPropertyFlags(CPF_BlueprintVisible));
    TestFalse(TEXT("bound canonical identity is not Blueprint-visible"),
              BindingId
                  && BindingId->HasAnyPropertyFlags(CPF_BlueprintVisible));

    TestNull(TEXT("authority mutation is not callable from Blueprint"),
             UMythicEntityViewerKnowledgeComponent::StaticClass()->FindFunctionByName(
                 TEXT("AuthorityMergeLearnedKnowledge")));
    TestNotNull(TEXT("safe exact-subject query is callable from Blueprint"),
                UMythicEntityViewerKnowledgeComponent::StaticClass()->FindFunctionByName(
                    TEXT("GetKnowledgeForSubject")));

    const AMythicPlayerState *PlayerStateCDO = GetDefault<AMythicPlayerState>();
    TestNotNull(TEXT("PlayerState owns viewer knowledge as a default subobject"),
                PlayerStateCDO
                    ? PlayerStateCDO->GetEntityViewerKnowledgeComponent()
                    : nullptr);
    TestNotNull(TEXT("viewer knowledge coexists with contextual action grants"),
                PlayerStateCDO
                    ? PlayerStateCDO->GetEntityActionGrantComponent()
                    : nullptr);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicEntityKnowledgeSanitizationTest,
    "Mythic.World.Entity.ViewerKnowledge.SanitizedLearnedDTO",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicEntityKnowledgeSanitizationTest::RunTest(
    const FString & /*Parameters*/) {
    FMythicEntityKnowledgeView Unsafe;
    Unsafe.bRecognitionGranted = true;
    Unsafe.bNameKnown = true;
    Unsafe.RecognizedName = FText::FromString(
        FString::ChrN(FMythicEntityKnowledgeRules::MaxRecognizedNameCharacters
                          + 1,
                      TEXT('X')));
    Unsafe.bFactionKnown = true;
    Unsafe.bRoleKnown = true;
    Unsafe.RelationshipBand = static_cast<EMythicKnownRelationshipBand>(255);
    Unsafe.StandingBand = static_cast<EMythicKnownStandingBand>(255);
    Unsafe.DiscoveredTraits.AddTag(ENTITY_KNOWLEDGE_TRAIT_ROOT);

    const FMythicEntityKnowledgeView Safe =
        FMythicEntityKnowledgeRules::Sanitize(Unsafe);
    TestFalse(TEXT("recognition is never durable knowledge"),
              Safe.bRecognitionGranted);
    TestFalse(TEXT("an unbounded name is rejected"), Safe.bNameKnown);
    TestTrue(TEXT("a rejected name is cleared"), Safe.RecognizedName.IsEmpty());
    TestFalse(TEXT("a missing faction tag cannot be marked known"),
              Safe.bFactionKnown);
    TestFalse(TEXT("a missing role tag cannot be marked known"),
              Safe.bRoleKnown);
    TestEqual(TEXT("an invalid relationship band becomes unknown"),
              Safe.RelationshipBand,
              EMythicKnownRelationshipBand::Unknown);
    TestEqual(TEXT("an invalid standing band becomes unknown"),
              Safe.StandingBand, EMythicKnownStandingBand::Unknown);
    TestTrue(TEXT("knowledge category roots are not displayable facts"),
             Safe.DiscoveredTraits.IsEmpty());

    FMythicEntityKnowledgeView Existing;
    Existing.bNameKnown = true;
    Existing.RecognizedName = FText::FromString(TEXT("Mira"));
    FMythicEntityKnowledgeView Delta;
    Delta.RelationshipBand = EMythicKnownRelationshipBand::Friendly;
    TestTrue(TEXT("new learned information changes the dossier"),
             FMythicEntityKnowledgeRules::MergeLearnedDelta(Existing,
                                                              Delta));
    TestTrue(TEXT("unknown delta fields do not erase the known name"),
             Existing.bNameKnown
                 && Existing.RecognizedName.ToString() == TEXT("Mira"));
    TestEqual(TEXT("the learned coarse relationship is merged"),
              Existing.RelationshipBand,
              EMythicKnownRelationshipBand::Friendly);

    TArray<FMythicEntityLearnedDossier> Dossiers;
    FMythicEntityLearnedDossier &Living = Dossiers.AddDefaulted_GetRef();
    Living.EntityId = MythicViewerKnowledgeTests::MakeEntityId(1);
    FMythicEntityLearnedDossier &Player = Dossiers.AddDefaulted_GetRef();
    Player.EntityId = MythicViewerKnowledgeTests::MakeEntityId(
        1, EMythicEntityDomain::PlayerCharacter);
    TestEqual(TEXT("typed domains cannot alias a dossier key"),
              FMythicEntityKnowledgeRules::FindDossierIndex(
                  Dossiers, Player.EntityId),
              1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicEntityRecognitionLifecycleTest,
    "Mythic.World.Entity.ViewerKnowledge.BoundedRecognitionLifecycle",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicEntityRecognitionLifecycleTest::RunTest(
    const FString & /*Parameters*/) {
    using namespace MythicViewerKnowledgeTests;

    FScopedKnowledgeWorld Fixture;
    UWorld *World = Fixture.Get();
    UMythicEntityPresentationRegistry *Registry =
        World ? World->GetSubsystem<UMythicEntityPresentationRegistry>()
              : nullptr;
    AActor *Owner = World ? World->SpawnActor<AActor>() : nullptr;
    if (!World || !Registry || !Owner) {
        AddError(TEXT("the viewer-knowledge authority fixture could not initialize"));
        return false;
    }

    UMythicEntityViewerKnowledgeComponent *Knowledge =
        NewObject<UMythicEntityViewerKnowledgeComponent>(Owner);
    Owner->AddInstanceComponent(Knowledge);
    Knowledge->RegisterComponent();

    TArray<FMythicEntityPresentationInstance> Instances;
    Instances.Reserve(65);
    for (uint32 Index = 1; Index <= 65; ++Index) {
        const FMythicEntityId EntityId = MakeEntityId(Index);
        const FMythicEntityPresentationInstance Instance =
            Registry->AllocateAuthorityInstance(EntityId);
        FMythicEntityKnowledgeView Learned;
        Learned.bNameKnown = true;
        Learned.RecognizedName =
            FText::FromString(FString::Printf(TEXT("Person %u"), Index));
        if (!Knowledge->AuthorityLearnAndGrantRecognition(
                Instance, EntityId, Learned, 30.0f)) {
            AddError(FString::Printf(
                TEXT("recognition grant %u was unexpectedly rejected"),
                Index));
            return false;
        }
        Instances.Add(Instance);
    }

    TestEqual(TEXT("the owner projection remains at its fixed cap"),
              Knowledge->GetActiveRecognitionCount(), 64);
    FMythicEntityKnowledgeView View;
    TestFalse(TEXT("overflow evicts the least recently refreshed embodiment"),
              Knowledge->GetKnowledgeForSubject(Instances[0], View));
    TestTrue(TEXT("the newest exact embodiment remains recognized"),
             Knowledge->GetKnowledgeForSubject(Instances.Last(), View));
    TestEqual(TEXT("durable learned dossiers survive projection LRU eviction"),
              Knowledge->GetLearnedDossierCount(), 65);
    TestTrue(TEXT("the safe projection carries the localized learned name"),
             View.bNameKnown
                 && View.RecognizedName.ToString() == TEXT("Person 65"));

    FMythicEntityId Resolved;
    TestTrue(TEXT("native owner resolution returns the entitled typed entity"),
             Knowledge->ResolveRecognizedEntity(Instances.Last(), Resolved));
    TestTrue(TEXT("native resolution matches the registry's canonical mapping"),
             Resolved == MakeEntityId(65));

    const int32 Pruned = Knowledge->AuthorityPruneExpiredRecognition(
        static_cast<double>(World->GetTimeSeconds()) + 1000.0);
    TestEqual(TEXT("lease expiry clears every ephemeral binding"), Pruned, 64);
    TestEqual(TEXT("no expired recognition remains queryable"),
              Knowledge->GetActiveRecognitionCount(), 0);
    TestEqual(TEXT("lease expiry never destroys durable learned knowledge"),
              Knowledge->GetLearnedDossierCount(), 65);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

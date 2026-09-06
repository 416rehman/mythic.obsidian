#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "AI/NPCs/MythicNPCCharacter.h"
#include "AI/Creatures/MythicCreatureCharacter.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/WorldSettings.h"
#include "GAS/AttributeSets/Shared/MythicLifeComponent.h"
#include "Tests/Progression/MythicAbilityWorldFixture.h"
#include "UObject/StrongObjectPtr.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "UObject/UnrealType.h"
#include "World/Entity/MythicEntityId.h"
#include "World/Entity/MythicEntityIdentityDefinition.h"
#include "World/Entity/MythicEntityPresentationComponent.h"
#include "World/Entity/MythicEntityPresentationRegistry.h"
#include "World/Entity/MythicEntityPresentationTags.h"
#include "World/Entity/MythicEntityPresentationTypes.h"

struct FMythicDirectEntityPresentationTestAccess {
    static void SetAuthoredIdentity(AMythicNPCCharacter &NPC,
                                    const FGuid &Guid) {
        NPC.AuthoredWorldIdentityGuid = Guid;
    }

    static void Activate(AMythicNPCCharacter &NPC) {
        NPC.ConfigureEntityPresentationAnchor();
        NPC.TryActivateDirectEntityPresentation();
    }
};

namespace {

class FScopedEntityPresentationWorld final {
public:
    explicit FScopedEntityPresentationWorld(const ENetMode NetMode) {
        InitializationValues = UWorld::InitializationValues()
                                   .CreatePhysicsScene(false)
                                   .ShouldSimulatePhysics(false)
                                   .EnableTraceCollision(false)
                                   .CreateNavigation(false)
                                   .CreateAISystem(false);
        World = UWorld::CreateWorld(
            EWorldType::PIE, false,
            MakeUniqueObjectName(nullptr, UWorld::StaticClass(),
                                 TEXT("EntityPresentationRegistryTest")),
            nullptr, true, ERHIFeatureLevel::Num, &InitializationValues,
            true);
        if (World) {
            World->SetPlayInEditorInitialNetMode(NetMode);
            World->InitWorld(InitializationValues);
        }
    }

    ~FScopedEntityPresentationWorld() {
        if (World) {
            World->DestroyWorld(false);
        }
    }

    UWorld *Get() const { return World; }

private:
    UWorld::InitializationValues InitializationValues;
    UWorld *World = nullptr;
};

FMythicEntityId MakeEntityId(const EMythicEntityDomain Domain,
                             const uint32 Salt) {
    return FMythicEntityId::FromAuthorityGuid(
        Domain, FGuid(0x11000000u + Salt, 0x22000000u + Salt,
                      0x33000000u + Salt, 0x44000000u + Salt));
}

template <typename StructType>
bool RoundTripNetStruct(const StructType &Source, StructType &OutResult) {
    TArray<uint8> Bytes;
    {
        FMemoryWriter Writer(Bytes, true);
        StructType Copy = Source;
        bool bSuccess = false;
        Copy.NetSerialize(Writer, nullptr, bSuccess);
        if (!bSuccess || Writer.IsError()) {
            return false;
        }
    }

    FMemoryReader Reader(Bytes, true);
    bool bSuccess = false;
    OutResult.NetSerialize(Reader, nullptr, bSuccess);
    return bSuccess && !Reader.IsError();
}

} // namespace

namespace {
class FPoolStartupCommand final : public IAutomationLatentCommand {
public:
    FPoolStartupCommand(FAutomationTestBase* InTest, UGameInstance* InGame,
        TArray<TWeakObjectPtr<AMythicNPCCharacter>> InWarmed,
        AMythicNPCCharacter* InDirect, AMythicNPCCharacter* InAuthored,
        AMythicNPCCharacter* InPrepared, AMythicNPCCharacter* InActive,
        FMythicEntityPresentationInstance InActiveInstance)
        : Test(InTest), Game(InGame), Warmed(MoveTemp(InWarmed)), Direct(InDirect),
          Authored(InAuthored), Prepared(InPrepared), Active(InActive),
          ActiveInstance(InActiveInstance),
          Deadline(FPlatformTime::Seconds() + 15.0) {}

    ~FPoolStartupCommand() override { DestroyOwnedAbilityTestWorld(Game.Get()); }

    bool Update() override {
        if (FPlatformTime::Seconds() > Deadline) {
            Test->AddError(TEXT("pool startup fixture exceeded its 15-second frame deadline"));
            return true;
        }
        if (LastFrame == GFrameCounter) return false;
        LastFrame = GFrameCounter;
        UWorld* World = Game->GetWorld();
        World->Tick(LEVELTICK_All, 1.0f / 60.0f);
        if (++Frames < 2) return false;
        Frames = 0;

        for (const auto& Weak : Warmed) CheckParked(Weak.Get(), true);
        if (Stage == 0) {
            Test->TestTrue(TEXT("ordinary runtime spawn still activates through its real deferred timer"),
                Presentation(Direct.Get())->GetPublicIdentitySnapshot().IsActive()
                && Presentation(Direct.Get())->GetAuthorityEntityId().GetDomain() == EMythicEntityDomain::Runtime);
            Test->TestTrue(TEXT("ordinary placed spawn retains its authored identity through BeginPlay"),
                Presentation(Authored.Get())->GetPublicIdentitySnapshot().IsActive()
                && Presentation(Authored.Get())->GetAuthorityEntityId().GetDomain() == EMythicEntityDomain::AuthoredWorld);
            Test->TestTrue(TEXT("canonical activation before BeginPlay stays active and collidable"),
                Presentation(Active.Get())->GetPublicIdentitySnapshot().IsActive()
                && Presentation(Active.Get())->GetPresentationInstance() == ActiveInstance
                && Active->GetActorEnableCollision() && !Active->IsHidden()
                && Active->GetCharacterMovement()->MovementMode != MOVE_None);
            CheckParked(Prepared.Get(), false);

            // Spawn into a begun world so a real fallback is already queued, then
            // claim the body without an identity. Wake must not re-enable fallback.
            FActorSpawnParameters Spawn;
            Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            LatePrepared = World->SpawnActor<AMythicNPCCharacter>(FVector(6000, 0, 500), FRotator::ZeroRotator, Spawn);
            if (!Test->TestNotNull(TEXT("post-BeginPlay actor spawns"), LatePrepared.Get())) return true;
            Test->TestTrue(TEXT("late spawn really queued its BeginPlay fallback"), LatePrepared->HasActorBegunPlay());
            LatePrepared->PrepareForEmbodiment();
            auto* LatePooled = World->SpawnActor<AMythicNPCCharacter>(FVector(8000, 0, 500), FRotator::ZeroRotator, Spawn);
            if (!Test->TestNotNull(TEXT("post-BeginPlay pool return control spawns"), LatePooled)) return true;
            Test->TestTrue(TEXT("late pool control has already begun play"), LatePooled->HasActorBegunPlay());
            LatePooled->SleepToPool();
            Warmed.Add(LatePooled);
            Direct->SleepToPool();
            Warmed.Add(Direct.Get());
            Prepared->SleepToPool();
            Prepared->WakeFromPool();
            ++Stage;
            return false;
        }
        if (Stage == 1) {
            CheckParked(Prepared.Get(), false);
            CheckParked(LatePrepared.Get(), false);
            FMythicPublicIdentitySnapshot Identity;
            Identity.PublicKindTag = MythicEntityPresentationTags::EntityKindHumanoid;
            ReuseId = MakeEntityId(EMythicEntityDomain::LivingWorld, 900);
            Test->TestTrue(TEXT("managed body can receive its intended canonical identity after waiting"),
                Presentation(Prepared.Get())->AuthorityPrepareEmbodiment(ReuseId, Identity));
            Test->TestTrue(TEXT("managed preparation activates through the explicit authority path"), Prepared->ActivatePreparedEmbodiment());
            ReuseInstance = Presentation(Prepared.Get())->GetPresentationInstance();
            ++Stage;
            return false;
        }
        if (Stage == 2) {
            Test->TestTrue(TEXT("managed activation survives later frames without a fallback replacement"),
                Presentation(Prepared.Get())->GetAuthorityEntityId() == ReuseId
                && Presentation(Prepared.Get())->GetPresentationInstance() == ReuseInstance
                && Presentation(Prepared.Get())->GetPublicIdentitySnapshot().IsActive()
                && Prepared->GetActorEnableCollision() && Prepared->LifeComponent->IsInitialized());
            Prepared->SleepToPool();
            Warmed.Add(Prepared.Get());
            ++Stage;
            return false;
        }
        Test->TestFalse(TEXT("returned embodiment releases its prior public instance"),
            Presentation(Prepared.Get())->GetPresentationInstance().IsValid());
        return true;
    }

private:
    static UMythicEntityPresentationComponent* Presentation(AMythicNPCCharacter* NPC) {
        return NPC->GetEntityPresentationComponent_Implementation();
    }
    void CheckParked(AMythicNPCCharacter* NPC, bool bPooled) {
        if (!Test->TestNotNull(TEXT("owned inactive body still exists"), NPC)) return;
        Test->TestTrue(TEXT("inactive body stays hidden, intangible and movement-parked across real frames"),
            NPC->IsHidden() && !NPC->GetActorEnableCollision() && !NPC->IsActorTickEnabled()
            && NPC->GetCharacterMovement()->MovementMode == MOVE_None
            && !NPC->GetCharacterMovement()->IsComponentTickEnabled());
        Test->TestFalse(TEXT("inactive body cannot invent a direct identity"), Presentation(NPC)->GetAuthorityEntityId().IsValid());
        Test->TestFalse(TEXT("inactive body cannot publish an active presentation"), Presentation(NPC)->GetPublicIdentitySnapshot().IsActive());
        if (bPooled) Test->TestFalse(TEXT("BeginPlay cannot rebind a pooled body's life component"), NPC->LifeComponent->IsInitialized());
    }

    FAutomationTestBase* Test;
    TStrongObjectPtr<UGameInstance> Game;
    TArray<TWeakObjectPtr<AMythicNPCCharacter>> Warmed;
    TWeakObjectPtr<AMythicNPCCharacter> Direct, Authored, Prepared, Active, LatePrepared;
    FMythicEntityPresentationInstance ActiveInstance, ReuseInstance;
    FMythicEntityId ReuseId;
    uint64 LastFrame = MAX_uint64;
    double Deadline;
    int32 Frames = 0, Stage = 0;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicPoolStartupPresentationTest,
    "Mythic.World.Entity.Identity.PooledStartupPresentation",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicPoolStartupPresentationTest::RunTest(const FString&) {
    auto* Game = NewObject<UGameInstance>(GEngine);
    Game->InitializeStandalone();
    UWorld* World = Game->GetWorld();
    TArray<TWeakObjectPtr<AMythicNPCCharacter>> Warmed;
    // Include actual Living World warm-pool bodies when enabled, plus explicit
    // humanoid/creature warmups so the regression does not depend on that setting.
    for (TActorIterator<AMythicNPCCharacter> It(World); It; ++It) Warmed.Add(*It);
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* WarmNPC = World->SpawnActor<AMythicNPCCharacter>(FVector(1000, 0, 500), FRotator::ZeroRotator, Spawn);
    auto* WarmCreature = World->SpawnActor<AMythicCreatureCharacter>(FVector(2000, 0, 500), FRotator::ZeroRotator, Spawn);
    auto* Direct = World->SpawnActor<AMythicNPCCharacter>(FVector(3000, 0, 500), FRotator::ZeroRotator, Spawn);
    auto* Authored = World->SpawnActor<AMythicNPCCharacter>(FVector(4000, 0, 500), FRotator::ZeroRotator, Spawn);
    auto* Prepared = World->SpawnActor<AMythicNPCCharacter>(FVector(5000, 0, 500), FRotator::ZeroRotator, Spawn);
    auto* Active = World->SpawnActor<AMythicNPCCharacter>(FVector(7000, 0, 500), FRotator::ZeroRotator, Spawn);
    if (!WarmNPC || !WarmCreature || !Direct || !Authored || !Prepared || !Active) {
        AddError(TEXT("pool startup fixture could not spawn its owned actors"));
        DestroyOwnedAbilityTestWorld(Game);
        return false;
    }
    FMythicDirectEntityPresentationTestAccess::SetAuthoredIdentity(*Authored, FGuid::NewGuid());
    WarmNPC->SleepToPool(); WarmCreature->SleepToPool();
    Warmed.Add(WarmNPC); Warmed.Add(WarmCreature);
    World->InitializeActorsForPlay(FURL());
    TestTrue(TEXT("startup routes real actor initialization while still before BeginPlay"),
        World->AreActorsInitialized() && !World->HasBegunPlay());
    Prepared->PrepareForEmbodiment();
    Active->PrepareForEmbodiment();
    FMythicPublicIdentitySnapshot Identity;
    Identity.PublicKindTag = MythicEntityPresentationTags::EntityKindHumanoid;
    if (!Active->GetEntityPresentationComponent_Implementation()->AuthorityPrepareEmbodiment(
            MakeEntityId(EMythicEntityDomain::LivingWorld, 901), Identity)
        || !Active->ActivatePreparedEmbodiment()) {
        AddError(TEXT("pool startup fixture could not activate its pre-BeginPlay control"));
        DestroyOwnedAbilityTestWorld(Game);
        return false;
    }
    const auto ActiveInstance = Active->GetEntityPresentationComponent_Implementation()->GetPresentationInstance();
    World->GetWorldSettings()->NotifyBeginPlay();
    TestTrue(TEXT("pool startup test exercises actual actor BeginPlay"), World->HasBegunPlay() && WarmNPC->HasActorBegunPlay());
    ADD_LATENT_AUTOMATION_COMMAND(FPoolStartupCommand(this, Game, MoveTemp(Warmed), Direct, Authored, Prepared, Active, ActiveInstance));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicDirectEntityPresentationTest,
    "Mythic.World.Entity.Identity.DirectActorPresentation",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicDirectEntityPresentationTest::RunTest(
    const FString & /*Parameters*/) {
    FScopedEntityPresentationWorld Fixture(NM_ListenServer);
    UWorld *World = Fixture.Get();
    AMythicNPCCharacter *NPC = World
        ? World->SpawnActor<AMythicNPCCharacter>() : nullptr;
    UMythicEntityPresentationComponent *Presentation = NPC
        ? NPC->GetEntityPresentationComponent_Implementation() : nullptr;
    if (!World || !NPC || !Presentation) {
        AddError(TEXT("the direct-actor presentation fixture could not spawn"));
        return false;
    }

    const FGuid AuthoredGuid(0x01234567u, 0x89abcdefu,
                             0x10293847u, 0x56aabbccu);
    FMythicDirectEntityPresentationTestAccess::SetAuthoredIdentity(
        *NPC, AuthoredGuid);
    FMythicDirectEntityPresentationTestAccess::Activate(*NPC);

    const FMythicEntityId &EntityId =
        Presentation->GetAuthorityEntityId();
    TestTrue(TEXT("a direct actor receives a valid canonical identity"),
             EntityId.IsValid());
    TestEqual(TEXT("placed actors use the authored-world domain"),
              EntityId.GetDomain(), EMythicEntityDomain::AuthoredWorld);
    TestTrue(TEXT("the baked GUID survives as the private canonical value"),
             EntityId.GetAuthorityGuid() == AuthoredGuid);
    TestTrue(TEXT("the direct actor activates its public embodiment"),
             Presentation->GetPublicIdentitySnapshot().IsActive());
    TestTrue(TEXT("the direct humanoid exposes only its coarse public kind"),
             Presentation->GetPublicIdentitySnapshot().PublicKindTag
                 == MythicEntityPresentationTags::EntityKindHumanoid);
    TestTrue(TEXT("the presentation anchor is above the actor origin"),
             Presentation->GetPresentationAnchorWorldLocation().Z
                 > NPC->GetActorLocation().Z);

    const FProperty *AuthoredIdentityProperty = FindFProperty<FProperty>(
        AMythicNPCCharacter::StaticClass(),
        TEXT("AuthoredWorldIdentityGuid"));
    TestNotNull(TEXT("the baked identity remains serialized on placed actors"),
                AuthoredIdentityProperty);
    TestTrue(TEXT("the baked identity participates in save serialization"),
             AuthoredIdentityProperty
                 && AuthoredIdentityProperty->HasAnyPropertyFlags(
                     CPF_SaveGame));
    TestFalse(TEXT("the baked canonical GUID is not Blueprint-visible"),
              AuthoredIdentityProperty
                  && AuthoredIdentityProperty->HasAnyPropertyFlags(
                      CPF_BlueprintVisible));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicCanonicalEntityIdentityTest,
    "Mythic.World.Entity.Identity.TypedPrivateIdentity",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicCanonicalEntityIdentityTest::RunTest(
    const FString & /*Parameters*/) {
    const FGuid SharedGuid(0x12345678u, 0x90abcdefu, 0x10203040u,
                           0x50607080u);
    const FMythicEntityId LivingWorld =
        FMythicEntityId::FromAuthorityGuid(
            EMythicEntityDomain::LivingWorld, SharedGuid);
    const FMythicEntityId SameLivingWorld =
        FMythicEntityId::FromAuthorityGuid(
            EMythicEntityDomain::LivingWorld, SharedGuid);
    const FMythicEntityId Player = FMythicEntityId::FromAuthorityGuid(
        EMythicEntityDomain::PlayerCharacter, SharedGuid);

    TestTrue(TEXT("a typed canonical identity is valid"),
             LivingWorld.IsValid());
    TestTrue(TEXT("the same domain and GUID compare equal"),
             LivingWorld == SameLivingWorld);
    TestFalse(TEXT("the same GUID in different domains cannot alias"),
              LivingWorld == Player);
    TestFalse(TEXT("an invalid domain cannot construct an identity"),
              FMythicEntityId::FromAuthorityGuid(
                  EMythicEntityDomain::Invalid, SharedGuid)
                  .IsValid());
    TestFalse(TEXT("a zero GUID cannot construct an identity"),
              FMythicEntityId::FromAuthorityGuid(
                  EMythicEntityDomain::LivingWorld, FGuid())
                  .IsValid());

    FMythicEntityId RoundTripped;
    TestTrue(TEXT("canonical identity network serialization succeeds"),
             RoundTripNetStruct(LivingWorld, RoundTripped));
    TestTrue(TEXT("canonical identity survives network serialization"),
             RoundTripped == LivingWorld);

    FMythicEntityId ClearedByInvalid = LivingWorld;
    TestTrue(TEXT("invalid identity network serialization succeeds"),
             RoundTripNetStruct(FMythicEntityId(), ClearedByInvalid));
    TestFalse(TEXT("invalid network identity clears stale destination data"),
              ClearedByInvalid.IsValid());

    const FProperty *DomainProperty = FindFProperty<FProperty>(
        FMythicEntityId::StaticStruct(), TEXT("Domain"));
    const FProperty *ValueProperty = FindFProperty<FProperty>(
        FMythicEntityId::StaticStruct(), TEXT("Value"));
    TestNotNull(TEXT("canonical domain remains reflected for SaveGame"),
                DomainProperty);
    TestNotNull(TEXT("canonical GUID remains reflected for SaveGame"),
                ValueProperty);
    TestFalse(TEXT("canonical domain is not Blueprint-visible"),
              DomainProperty
                  && DomainProperty->HasAnyPropertyFlags(CPF_BlueprintVisible));
    TestFalse(TEXT("canonical GUID is not Blueprint-visible"),
              ValueProperty
                  && ValueProperty->HasAnyPropertyFlags(CPF_BlueprintVisible));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicPresentationRegistryIdentityBoundaryTest,
    "Mythic.World.Entity.PresentationRegistry.IdentityBoundary",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicPresentationRegistryIdentityBoundaryTest::RunTest(
    const FString & /*Parameters*/) {
    FScopedEntityPresentationWorld Fixture(NM_ListenServer);
    UWorld *World = Fixture.Get();
    if (!World) {
        AddError(TEXT("the presentation fixture could not create a world"));
        return false;
    }

    UMythicEntityPresentationRegistry *Registry =
        World->GetSubsystem<UMythicEntityPresentationRegistry>();
    if (!Registry) {
        AddError(TEXT("the presentation registry was not created"));
        return false;
    }

    const FMythicEntityId EntityA =
        MakeEntityId(EMythicEntityDomain::LivingWorld, 1);
    const FMythicEntityId EntityB =
        MakeEntityId(EMythicEntityDomain::LivingWorld, 2);

    const FMythicEntityPresentationInstance FirstA =
        Registry->AllocateAuthorityInstance(EntityA);
    TestTrue(TEXT("authority allocates a valid public instance"),
             FirstA.IsValid());

    FMythicEntityId ResolvedEntity;
    TestTrue(TEXT("authority can privately resolve its public instance"),
             Registry->ResolveAuthorityEntity(FirstA, ResolvedEntity));
    TestTrue(TEXT("private resolution returns the canonical entity"),
             ResolvedEntity == EntityA);

    const FMythicEntityPresentationInstance SecondA =
        Registry->AllocateAuthorityInstance(EntityA);
    TestTrue(TEXT("rebinding allocates a valid replacement instance"),
             SecondA.IsValid());
    TestTrue(TEXT("rebinding changes the opaque public handle"),
             FirstA.Handle != SecondA.Handle);
    TestTrue(TEXT("rebinding advances the world embodiment generation"),
             SecondA.EmbodimentGeneration
                 > FirstA.EmbodimentGeneration);
    TestFalse(TEXT("the released old instance no longer resolves"),
              Registry->ResolveAuthorityEntity(FirstA, ResolvedEntity));
    TestFalse(TEXT("failed private resolution clears its output"),
              ResolvedEntity.IsValid());

    FMythicEntityPresentationInstance FoundA;
    TestTrue(TEXT("authority finds the current embodiment by canonical ID"),
             Registry->FindAuthorityInstance(EntityA, FoundA));
    TestTrue(TEXT("canonical lookup returns only the replacement instance"),
             FoundA == SecondA);

    const FMythicEntityPresentationInstance FirstB =
        Registry->AllocateAuthorityInstance(EntityB);
    TestTrue(TEXT("a second entity receives a valid instance"),
             FirstB.IsValid());
    TestTrue(TEXT("different entities receive different public handles"),
             FirstB.Handle != SecondA.Handle);
    TestTrue(TEXT("generation allocation is world-global, not actor-local"),
             FirstB.EmbodimentGeneration
                 > SecondA.EmbodimentGeneration);

    FMythicEntityPresentationInstance SerializedInstance;
    TestTrue(TEXT("public instance network serialization succeeds"),
             RoundTripNetStruct(FirstB, SerializedInstance));
    TestTrue(TEXT("public instance round-trips handle and generation"),
             SerializedInstance == FirstB);

    Registry->ResetAuthorityPresentationEpoch();
    TestFalse(TEXT("epoch reset invalidates the first entity mapping"),
              Registry->FindAuthorityInstance(EntityA, FoundA));
    TestFalse(TEXT("epoch reset invalidates the second entity mapping"),
              Registry->ResolveAuthorityEntity(FirstB, ResolvedEntity));

    const FMythicEntityPresentationInstance AfterReset =
        Registry->AllocateAuthorityInstance(EntityA);
    TestTrue(TEXT("an entity can bind again after an epoch reset"),
             AfterReset.IsValid());
    TestTrue(TEXT("epoch reset never recycles a prior public handle"),
             AfterReset.Handle != FirstA.Handle
                 && AfterReset.Handle != SecondA.Handle);
    TestTrue(TEXT("generation remains advancing across an epoch reset"),
             AfterReset.EmbodimentGeneration
                 > FirstB.EmbodimentGeneration);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicPresentationRegistryClientAuthorityTest,
    "Mythic.World.Entity.PresentationRegistry.ClientCannotAllocate",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicPresentationRegistryClientAuthorityTest::RunTest(
    const FString & /*Parameters*/) {
    FScopedEntityPresentationWorld Fixture(NM_Client);
    UWorld *World = Fixture.Get();
    if (!World) {
        AddError(TEXT("the client presentation fixture could not create a world"));
        return false;
    }

    UMythicEntityPresentationRegistry *Registry =
        World->GetSubsystem<UMythicEntityPresentationRegistry>();
    if (!Registry) {
        AddError(TEXT("the client presentation registry was not created"));
        return false;
    }

    const FMythicEntityId Entity =
        MakeEntityId(EMythicEntityDomain::LivingWorld, 9);
    const FMythicEntityPresentationInstance Attempt =
        Registry->AllocateAuthorityInstance(Entity);
    TestFalse(TEXT("a client cannot allocate a public presentation nonce"),
              Attempt.IsValid());

    FMythicEntityId ResolvedEntity = Entity;
    TestFalse(TEXT("a client cannot access canonical reverse mappings"),
              Registry->ResolveAuthorityEntity(Attempt, ResolvedEntity));
    TestFalse(TEXT("client canonical resolution clears output"),
              ResolvedEntity.IsValid());

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicPresentationRegistryComponentLifecycleTest,
    "Mythic.World.Entity.PresentationRegistry.ComponentLifecycle",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicPresentationRegistryComponentLifecycleTest::RunTest(
    const FString & /*Parameters*/) {
    FScopedEntityPresentationWorld Fixture(NM_ListenServer);
    UWorld *World = Fixture.Get();
    if (!World) {
        AddError(TEXT("the presentation lifecycle fixture could not create a world"));
        return false;
    }

    UMythicEntityPresentationRegistry *Registry =
        World->GetSubsystem<UMythicEntityPresentationRegistry>();
    AActor *Owner = World->SpawnActor<AActor>();
    if (!Registry || !Owner) {
        AddError(TEXT("the lifecycle registry or actor could not be created"));
        return false;
    }

    UMythicEntityPresentationComponent *Component =
        NewObject<UMythicEntityPresentationComponent>(Owner);
    Owner->AddInstanceComponent(Component);
    Component->RegisterComponent();

    const FMythicEntityId Entity =
        MakeEntityId(EMythicEntityDomain::LivingWorld, 31);
    FMythicPublicIdentitySnapshot SafeIdentity;
    TestFalse(TEXT("an invalid public kind cannot enter the registry"),
              Component->AuthorityPrepareEmbodiment(Entity, SafeIdentity));
    SafeIdentity.PublicKindTag =
        MythicEntityPresentationTags::EntityKindHumanoid;
    TestTrue(TEXT("authority prepares a private-to-public embodiment"),
             Component->AuthorityPrepareEmbodiment(Entity, SafeIdentity));

    const FMythicEntityPresentationInstance Instance =
        Component->GetPublicIdentitySnapshot().Instance;
    const uint64 RevisionBeforeActivation = Registry->GetRegistryRevision();
    TestTrue(TEXT("authority activates and registers the complete embodiment"),
             Component->AuthorityActivateEmbodiment());
    TestTrue(TEXT("registration advances the push-registry revision"),
             Registry->GetRegistryRevision() > RevisionBeforeActivation);
    TestTrue(TEXT("exact public resolution returns the active component"),
             Registry->ResolvePresentationComponent(Instance) == Component);

    TArray<UMythicEntityPresentationComponent *> Registered;
    Registry->GetRegisteredComponents(Registered);
    TestTrue(TEXT("bounded push enumeration contains the active component"),
             Registered.Contains(Component));

    const FMythicEntityInstanceHandle LocalHandle{Instance, Component};
    TestTrue(TEXT("a local weak handle validates its exact embodiment"),
             LocalHandle.IsValid());

    bool bCanonicalResolvedDuringDeactivation = false;
    Component->OnPresentationDeactivated.AddLambda(
        [Registry, &bCanonicalResolvedDuringDeactivation](
            UMythicEntityPresentationComponent & /*Deactivating*/,
            const FMythicEntityPresentationInstance &DeactivatingInstance) {
            FMythicEntityId Resolved;
            bCanonicalResolvedDuringDeactivation =
                Registry->ResolveAuthorityEntity(DeactivatingInstance,
                                                  Resolved)
                && Resolved.IsValid();
        });

    Component->AuthorityDeactivateEmbodiment();
    TestTrue(TEXT("grant-revocation callbacks can resolve identity before release"),
             bCanonicalResolvedDuringDeactivation);
    TestNull(TEXT("a deactivated embodiment no longer resolves publicly"),
             Registry->ResolvePresentationComponent(Instance));
    TestFalse(TEXT("a weak local handle rejects pooled component reuse"),
              LocalHandle.IsValid());

    FMythicEntityId ReleasedEntity;
    TestFalse(TEXT("deactivation releases private canonical reverse mapping"),
              Registry->ResolveAuthorityEntity(Instance, ReleasedEntity));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicPublicIdentitySanitizationTest,
    "Mythic.World.Entity.PublicIdentitySnapshot.SanitizationBoundary",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicPublicIdentitySanitizationTest::RunTest(
    const FString & /*Parameters*/) {
    using namespace MythicEntityPresentationTags;
    FMythicPublicIdentitySnapshot Candidate;
    Candidate.bActive = true;
    Candidate.PublicKindTag = EntityKindHumanoid;
    Candidate.PublicIdentityDefinitionId = FPrimaryAssetId(
        FPrimaryAssetType(TEXT("PrivateSimulationRecord")),
        FName(TEXT("TrueFactionSpyRecord")));

    FMythicPublicIdentitySnapshot Sanitized;
    TestTrue(TEXT("a native visibly classified subject is accepted"),
             UMythicEntityPresentationComponent::BuildSanitizedPublicIdentity(
                 Candidate, Sanitized));
    TestFalse(TEXT("caller cannot publish active state or an instance"),
              Sanitized.bActive || Sanitized.Instance.IsValid());
    TestFalse(TEXT("an untyped private asset key is dropped"),
              Sanitized.PublicIdentityDefinitionId.IsValid());

    Candidate.PublicIdentityDefinitionId = FPrimaryAssetId(
        UMythicEntityIdentityDefinition::PrimaryAssetType,
        FName(TEXT("DA_PublicGuardCover")));
    TestTrue(TEXT("typed public identity is accepted"),
             UMythicEntityPresentationComponent::BuildSanitizedPublicIdentity(
                 Candidate, Sanitized));
    TestEqual(TEXT("typed public identity survives exactly"),
              Sanitized.PublicIdentityDefinitionId,
              Candidate.PublicIdentityDefinitionId);

    Candidate.PublicKindTag = FGameplayTag();
    TestFalse(TEXT("unknown kind fails closed"),
              UMythicEntityPresentationComponent::BuildSanitizedPublicIdentity(
                  Candidate, Sanitized));
    TestFalse(TEXT("failed sanitization returns an empty snapshot"),
              Sanitized.PublicKindTag.IsValid()
                  || Sanitized.PublicIdentityDefinitionId.IsValid());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicPublicIdentitySnapshotPrivacyTest,
    "Mythic.World.Entity.PublicIdentitySnapshot.NoCanonicalLeakage",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicPublicIdentitySnapshotPrivacyTest::RunTest(
    const FString & /*Parameters*/) {
    const UScriptStruct *SnapshotStruct =
        FMythicPublicIdentitySnapshot::StaticStruct();
    TestNotNull(TEXT("the public identity snapshot is reflected"),
                SnapshotStruct);
    TestNull(TEXT("the public snapshot has no canonical EntityId field"),
             FindFProperty<FProperty>(SnapshotStruct, TEXT("EntityId")));
    TestNull(TEXT("the public snapshot has no CanonicalEntityId field"),
             FindFProperty<FProperty>(SnapshotStruct,
                                      TEXT("CanonicalEntityId")));
    TestNull(TEXT("the public snapshot has no name-generation seed"),
             FindFProperty<FProperty>(SnapshotStruct, TEXT("NameSeed")));
    TestNull(TEXT("the public snapshot has no true faction"),
             FindFProperty<FProperty>(SnapshotStruct, TEXT("TrueFaction")));
    TestNull(TEXT("public role exists only in the identity definition"),
             FindFProperty<FProperty>(SnapshotStruct,
                                      TEXT("PublicArchetypeTag")));
    TestNull(TEXT("presented faction exists only in the identity definition"),
             FindFProperty<FProperty>(SnapshotStruct,
                                      TEXT("PresentedFactionTag")));

    FMythicPublicIdentitySnapshot Snapshot;
    Snapshot.bActive = true;
    TestFalse(TEXT("active flag alone cannot make a snapshot resolvable"),
              Snapshot.IsActive());
    Snapshot.Reset();
    TestFalse(TEXT("reset leaves a safely inactive snapshot"),
              Snapshot.bActive || Snapshot.Instance.IsValid()
                  || Snapshot.PublicKindTag.IsValid()
                  || Snapshot.PublicIdentityDefinitionId.IsValid());

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

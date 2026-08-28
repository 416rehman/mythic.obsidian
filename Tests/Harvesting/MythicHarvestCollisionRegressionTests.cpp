#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GAS/Abilities/MythicAnimNotify_SphereOverlap.h"
#include "Resources/MythicResourceISM.h"
#include "UObject/Package.h"
#include "World/Harvesting/MythicHarvestPCGIdentity.h"
#include "World/Harvesting/MythicHarvestableDefinition.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHarvestDestructibleObjectQueryRegressionTest,
    "Mythic.Harvesting.Collision.LiveDestructibleISMQuery",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestDestructibleObjectQueryRegressionTest::RunTest(
    const FString &Parameters) {
    UMythicResourceISM *DefaultResource =
        NewObject<UMythicResourceISM>(GetTransientPackage());
    TestEqual(TEXT("resource ISM defaults to the reviewed collision profile"),
              DefaultResource->GetCollisionProfileName(),
              FName(TEXT("Destructible")));
    TestEqual(TEXT("resource ISM defaults to the queried object channel"),
              DefaultResource->GetCollisionObjectType(), ECC_Destructible);
    TestTrue(TEXT("resource ISM defaults to query-enabled collision"),
             CollisionEnabledHasQuery(
                 DefaultResource->GetCollisionEnabled()));
    TestTrue(TEXT("resource ISM defaults to the complete harvest query contract"),
             DefaultResource->HasValidHarvestCollisionContract());

#if WITH_EDITOR
    UMythicHarvestableDefinition *ValidationDefinition =
        NewObject<UMythicHarvestableDefinition>(DefaultResource);
    ValidationDefinition->MaxWork = 10.0f;
    DefaultResource->HarvestableDefinition = ValidationDefinition;
    DefaultResource->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
    TestFalse(TEXT("PhysicsOnly is never a valid harvest query contract"),
              DefaultResource->HasValidHarvestCollisionContract());
    FDataValidationContext PhysicsOnlyContext;
    TestEqual(TEXT("editor validation rejects PhysicsOnly harvest providers"),
              DefaultResource->IsDataValid(PhysicsOnlyContext),
              EDataValidationResult::Invalid);
    bool bFoundQueryCollisionDiagnostic = false;
    for (const FDataValidationContext::FIssue &Issue :
         PhysicsOnlyContext.GetIssues()) {
        bFoundQueryCollisionDiagnostic |=
            Issue.Message.ToString().Contains(TEXT("query collision"));
    }
    TestTrue(TEXT("PhysicsOnly validation explains the missing query contract"),
             bFoundQueryCollisionDiagnostic);
#endif

    if (!TestNotNull(TEXT("engine is available"), GEngine)) {
        return false;
    }

    UGameInstance *GameInstance = NewObject<UGameInstance>(GEngine);
    GameInstance->InitializeStandalone();
    UWorld *World = GameInstance->GetWorld();
    if (!TestNotNull(TEXT("standalone physics world exists"), World)) {
        GameInstance->Shutdown();
        return false;
    }

    UStaticMesh *CollisionMesh = LoadObject<UStaticMesh>(
        nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (!TestNotNull(TEXT("engine collision fixture mesh is available"),
                     CollisionMesh)) {
        GameInstance->Shutdown();
        return false;
    }

    AActor *Owner = World->SpawnActor<AActor>();
    if (!TestNotNull(TEXT("resource fixture owner spawned"), Owner)) {
        GameInstance->Shutdown();
        return false;
    }

    UMythicHarvestableDefinition *Definition =
        NewObject<UMythicHarvestableDefinition>(Owner);
    Definition->MaxWork = 10.0f;
    int32 NodeSequence = 1;
    auto CreateResource = [Owner, CollisionMesh, Definition, &NodeSequence](
                              const FName Name, const FVector Location,
                              const ECollisionEnabled::Type CollisionMode) {
        UMythicResourceISM *Resource = NewObject<UMythicResourceISM>(
            Owner, Name);
        Owner->AddInstanceComponent(Resource);
        Resource->HarvestableDefinition = Definition;
        Resource->IdentitySource = EMythicHarvestIdentitySource::PCGPacked;
        Resource->IdentityCustomDataStartIndex = 0;
        Resource->SetStaticMesh(CollisionMesh);
        Resource->SetCollisionProfileName(FName(TEXT("Destructible")));
        Resource->SetCollisionEnabled(CollisionMode);
        Resource->SetNumCustomDataFloats(
            MythicHarvestPCGIdentity::PackedFloatCount);
        const FMythicHarvestNodeId NodeId(FGuid(
            0x98000000u + static_cast<uint32>(NodeSequence++),
            0x12345678u, 0xabcdef01u, 0x10203040u));
        TArray<float> PackedNodeId;
        MythicHarvestPCGIdentity::AppendPackedNodeId(NodeId, PackedNodeId);
        Resource->AddInstance(FTransform(Location), true);
        Resource->SetCustomData(0, MakeArrayView(PackedNodeId));
        Resource->RegisterComponent();
        return Resource;
    };

    UMythicResourceISM *Tree = CreateResource(
        TEXT("TreeResourceISM"), FVector(-75.0, 0.0, 0.0),
        ECollisionEnabled::QueryOnly);
    UMythicResourceISM *Rock = CreateResource(
        TEXT("RockResourceISM"), FVector(75.0, 0.0, 0.0),
        ECollisionEnabled::QueryOnly);
    TestTrue(TEXT("tree restores query collision only after full registration"),
             Tree->RefreshHarvestIdentityRegistration());
    TestTrue(TEXT("rock restores query collision only after full registration"),
             Rock->RefreshHarvestIdentityRegistration());

    // Registration suppresses and then restores query collision. Every collision setter clears the profile name, so a
    // contract that read that name would quarantine a healthy provider on its second refresh and never recover.
    TestFalse(TEXT("a registered tree is not left quarantined"),
              Tree->IsHarvestQueryCollisionSuppressedForTests());
    TestTrue(TEXT("a registered tree keeps query collision"),
             CollisionEnabledHasQuery(Tree->GetCollisionEnabled()));
    TestTrue(TEXT("re-registration recovers rather than permanently quarantining"),
             Tree->RefreshHarvestIdentityRegistration());
    TestFalse(TEXT("a re-registered tree is still not quarantined"),
              Tree->IsHarvestQueryCollisionSuppressedForTests());
    TestTrue(TEXT("a re-registered tree still satisfies the harvest collision contract"),
             Tree->HasValidHarvestCollisionContract());

    UMythicResourceISM *PhysicsOnly = CreateResource(
        TEXT("PhysicsOnlyResourceISM"), FVector(0.0, 0.0, 0.0),
        ECollisionEnabled::PhysicsOnly);
    AddExpectedError(TEXT("collision must use the Destructible object channel"),
                     EAutomationExpectedErrorFlags::Contains, 1);
    TestFalse(TEXT("runtime registration rejects PhysicsOnly collision"),
              PhysicsOnly->RefreshHarvestIdentityRegistration());
    TestTrue(TEXT("invalid runtime collision quarantines query participation"),
             PhysicsOnly->IsHarvestQueryCollisionSuppressedForTests());
    TestEqual(TEXT("quarantined collision is forced completely off"),
              PhysicsOnly->GetCollisionEnabled(),
              ECollisionEnabled::NoCollision);
    PhysicsOnly->UnregisterComponent();
    World->Tick(LEVELTICK_All, 0.0f);

    auto QueryContainsBoth = [World, Tree, Rock](
                                 const FCollisionObjectQueryParams &ObjectTypes) {
        TArray<FOverlapResult> Results;
        World->OverlapMultiByObjectType(
            Results, FVector::ZeroVector, FQuat::Identity, ObjectTypes,
            FCollisionShape::MakeSphere(200.0f));
        bool bFoundTree = false;
        bool bFoundRock = false;
        for (const FOverlapResult &Result : Results) {
            bFoundTree |= Result.Component.Get() == Tree;
            bFoundRock |= Result.Component.Get() == Rock;
        }
        return bFoundTree && bFoundRock;
    };

    const FCollisionObjectQueryParams RuntimeObjects =
        UMythicAnimNotify_SphereOverlap::BuildRuntimeObjectQueryParams();
    TestTrue(TEXT("the live attack query reaches Tree and Rock ISM bodies"),
             QueryContainsBoth(RuntimeObjects));

    FCollisionObjectQueryParams WithoutDestructible = RuntimeObjects;
    WithoutDestructible.RemoveObjectTypesToQuery(ECC_Destructible);
    TestFalse(TEXT("removing ECC_Destructible reproduces the regression"),
              QueryContainsBoth(WithoutDestructible));

    GameInstance->Shutdown();
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

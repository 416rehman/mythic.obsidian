#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Player/MythicAbilityIntentComponent.h"
#include "Tests/Progression/MythicAbilityWorldFixture.h"
#include "Components/BoxComponent.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicAbilityCameraParallaxTest,"Mythic.Combat.Abilities.CameraParallax",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::ProductFilter)
bool FMythicAbilityCameraParallaxTest::RunTest(const FString&) {
    auto* Game=NewObject<UGameInstance>(GEngine); Game->InitializeStandalone();
    ON_SCOPE_EXIT { DestroyOwnedAbilityTestWorld(Game); };
    auto* World=Game->GetWorld();
    World->InitializeActorsForPlay(FURL());
    auto* Source=World->SpawnActor<AActor>();
    const FVector Eye(0,0,200),Camera=Eye+FVector(-300,100,150),AimPoint=Eye+FVector(800,0,0);
    const FVector Ray=(AimPoint-Camera).GetSafeNormal();
    const auto AddWall=[&](FVector Location,FVector Extent) {
        auto* Wall=World->SpawnActor<AActor>(); auto* Box=NewObject<UBoxComponent>(Wall);
        Wall->SetRootComponent(Box); Wall->AddInstanceComponent(Box); Box->SetBoxExtent(Extent);
        Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly); Box->SetCollisionObjectType(ECC_WorldStatic);
        Box->SetCollisionResponseToAllChannels(ECR_Ignore); Box->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
        Box->RegisterComponent(); Wall->SetActorLocation(Location); return Wall;
    };
    auto* Target=AddWall(AimPoint,FVector(1,200,200));
    FVector Direction;
    TestTrue(TEXT("offset third-person camera converges on the visible point"),UMythicAbilityIntentComponent::ResolveCameraAim(World,Source,Eye,Camera,Ray,1200,Direction));
    TestTrue(TEXT("wire direction originates at pawn eyes, removing camera parallax"),Direction.Equals(FVector::ForwardVector,0.001));
    TestFalse(TEXT("raw camera direction is measurably different from accepted eye direction"),Ray.Equals(Direction,0.01));
    auto* Obstruction=AddWall(Camera+Ray*50,FVector(10,20,20));
    TestFalse(TEXT("obstacle between camera and player cannot fire backwards"),UMythicAbilityIntentComponent::ResolveCameraAim(World,Source,Eye,Camera,Ray,1200,Direction));
    TestTrue(TEXT("rejected sample clears the output"),Direction.IsZero());
    Obstruction->Destroy(); Target->Destroy();
    TestTrue(TEXT("clear sky supplies one bounded normalized ray"),UMythicAbilityIntentComponent::ResolveCameraAim(World,Source,Eye,Camera,Ray,1200,Direction) && Direction.IsNormalized());
    TestFalse(TEXT("missing local range is rejected"),UMythicAbilityIntentComponent::ResolveCameraAim(World,Source,Eye,Camera,Ray,0,Direction));
    TestFalse(TEXT("camera beyond the fixed local distance limit is rejected"),UMythicAbilityIntentComponent::ResolveCameraAim(World,Source,Eye,Eye+FVector(11000,0,0),-FVector::ForwardVector,1200,Direction));
    return true;
}
#endif

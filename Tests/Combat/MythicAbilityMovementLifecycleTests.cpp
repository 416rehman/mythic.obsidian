#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/Combat/MythicAbilityWindowTestFixture.h"
#include "Tests/Progression/MythicAbilityWorldFixture.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAbilityMovementLifecycleTest,"Mythic.Combat.Abilities.MovementLifecycle",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::ProductFilter)
bool FAbilityMovementLifecycleTest::RunTest(const FString&) {
    auto* Game=NewObject<UGameInstance>(GEngine); Game->InitializeStandalone();
    ON_SCOPE_EXIT { DestroyOwnedAbilityTestWorld(Game); };
    auto* Player=Game->GetWorld()->SpawnActor<AMythicPlayerState>(); InitializeAbilityTestPlayer(Player);
    auto* Avatar=InitializeAbilityTestAvatar(Player);
    if (!TestNotNull(TEXT("production avatar initializes"),Avatar)) return false;
    auto* Move=Avatar->GetCharacterMovement();
    if (!TestNotNull(TEXT("production character movement exists"),Move)) return false;
    for (int32 Count : {1,2}) {
        Move->SetMovementMode(MOVE_Falling);
        Avatar->Jump(); Avatar->JumpCurrentCount=Count; Avatar->JumpForceTimeRemaining=0.2f;
        Move->SetMovementMode(MOVE_Walking);
        TestEqual(TEXT("leaving falling restores the engine jump count"),Avatar->JumpCurrentCount,0);
        TestEqual(TEXT("landing clears held jump force"),Avatar->JumpForceTimeRemaining,0.0f);
        TestFalse(TEXT("landing clears held jump input"),Avatar->bPressedJump);
    }
    return true;
}
#endif

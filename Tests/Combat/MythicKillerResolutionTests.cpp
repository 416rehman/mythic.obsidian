
#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"

#include "GAS/MythicGameplayEffectContext.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicKillerResolutionTest,
    "Mythic.Combat.KillerResolution",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicKillerResolutionTest::RunTest(const FString &Parameters) {
    if (!TestNotNull(TEXT("engine is available"), GEngine)) {
        return false;
    }

    UGameInstance *GameInstance = NewObject<UGameInstance>(GEngine);
    GameInstance->InitializeStandalone();
    UWorld *World = GameInstance->GetWorld();
    if (!TestNotNull(TEXT("standalone world exists"), World)) {
        return false;
    }
    ON_SCOPE_EXIT {
        GameInstance->Shutdown();
    };

    APawn *Pawn = World->SpawnActor<APawn>();
    AController *Controller = World->SpawnActor<APlayerController>();
    APlayerState *PlayerState = World->SpawnActor<APlayerState>();
    if (!TestNotNull(TEXT("pawn spawned"), Pawn) || !TestNotNull(TEXT("controller spawned"), Controller)
        || !TestNotNull(TEXT("player state spawned"), PlayerState)) {
        return false;
    }

    Controller->Possess(Pawn);
    PlayerState->SetOwner(Controller);
    Controller->PlayerState = PlayerState;

    APawn *OutPawn = nullptr;
    AController *OutController = nullptr;
    APlayerState *OutPlayerState = nullptr;

    // An NPC instigates from its pawn.
    UMythicGameplayEffectContextLibrary::ResolveInstigator(Pawn, OutPawn, OutController, OutPlayerState);
    TestEqual(TEXT("pawn instigator resolves the pawn"), OutPawn, Pawn);
    TestEqual(TEXT("pawn instigator resolves the controller"), OutController, Controller);
    TestEqual(TEXT("pawn instigator resolves the player state"), OutPlayerState, PlayerState);

    UMythicGameplayEffectContextLibrary::ResolveInstigator(Controller, OutPawn, OutController, OutPlayerState);
    TestEqual(TEXT("controller instigator resolves the pawn"), OutPawn, Pawn);
    TestEqual(TEXT("controller instigator resolves the controller"), OutController, Controller);
    TestEqual(TEXT("controller instigator resolves the player state"), OutPlayerState, PlayerState);

    // A player's abilities instigate from the PlayerState, because that is what owns their ASC. Every kill
    // reward branch reads the killer from here, so this is the case that must not regress.
    UMythicGameplayEffectContextLibrary::ResolveInstigator(PlayerState, OutPawn, OutController, OutPlayerState);
    TestEqual(TEXT("player state instigator resolves the pawn"), OutPawn, Pawn);
    TestEqual(TEXT("player state instigator resolves the controller"), OutController, Controller);
    TestEqual(TEXT("player state instigator resolves the player state"), OutPlayerState, PlayerState);

    AActor *Unrelated = World->SpawnActor<AActor>();
    UMythicGameplayEffectContextLibrary::ResolveInstigator(Unrelated, OutPawn, OutController, OutPlayerState);
    TestNull(TEXT("an unrelated actor resolves no pawn"), OutPawn);
    TestNull(TEXT("an unrelated actor resolves no controller"), OutController);
    TestNull(TEXT("an unrelated actor resolves no player state"), OutPlayerState);

    UMythicGameplayEffectContextLibrary::ResolveInstigator(nullptr, OutPawn, OutController, OutPlayerState);
    TestNull(TEXT("a null instigator resolves no pawn"), OutPawn);
    TestNull(TEXT("a null instigator resolves no controller"), OutController);
    TestNull(TEXT("a null instigator resolves no player state"), OutPlayerState);

    return true;
}

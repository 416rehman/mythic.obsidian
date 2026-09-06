#pragma once
#if WITH_DEV_AUTOMATION_TESTS
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "UObject/Package.h"

/** Only for the standalone world created by this fixture; never operates on PIE or the editor world. */
inline void DestroyOwnedAbilityTestWorld(UGameInstance* Game) {
    if (!Game) return;
    UWorld* World=Game->GetWorld();
    check(World && World->WorldType==EWorldType::Game && World->GetGameInstance()==Game);
    UPackage* FixturePackage=World->GetPackage();
    const bool bOwnsTemporaryPackage=FixturePackage && FixturePackage->GetName().StartsWith(TEXT("/Temp/"));
    if (World->HasBegunPlay()) { World->BeginTearingDown(); World->EndPlay(EEndPlayReason::Quit); }
    World->RemoveFromRoot();
    Game->Shutdown();
    GEngine->DestroyWorldContext(World);
    World->DestroyWorld(false);
    // The destroyed standalone fixture is never user-authored content. Its empty
    // temporary map package can otherwise keep the editor's save prompt blocked.
    if (bOwnsTemporaryPackage) FixturePackage->SetDirtyFlag(false);
}
#endif

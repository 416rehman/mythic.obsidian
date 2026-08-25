#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Player/MythicPlayerState.h"
#include "Subsystem/SaveSystem/Character/CharacterData.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicCharacterNameTest,
                                 "Mythic.SaveSystem.CharacterName",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/**
 * A name the manifest set has to survive loading a save that predates it.
 *
 * The name is applied on possession from the manifest, then the character load runs. A save written before the
 * character had a name carries an empty string, so restoring it unguarded blanked the name every single load -
 * which meant fixing the manifest read alone would have looked correct for exactly one frame.
 *
 * This drives the real Deserialize path on a real player state, because the previous version of this test
 * asserted properties of the FString it had just constructed and stayed green with the guard deleted.
 */
bool FMythicCharacterNameTest::RunTest(const FString &Parameters) {
    if (!TestNotNull(TEXT("engine is available"), GEngine)) {
        return false;
    }

    UGameInstance *GameInstance = NewObject<UGameInstance>(GEngine);
    GameInstance->InitializeStandalone();
    UWorld *World = GameInstance->GetWorld();
    if (!TestNotNull(TEXT("standalone world exists"), World)) {
        return false;
    }

    AMythicPlayerState *PlayerState = World->SpawnActor<AMythicPlayerState>();
    if (!TestNotNull(TEXT("the player state spawned"), PlayerState)) {
        return false;
    }

    const FString FromManifest = TEXT("Rhoslyn");

    // What OnPostLogin does before the load: the manifest name reaches the player state.
    PlayerState->SetPlayerName(FromManifest);
    TestEqual(TEXT("the manifest name is on the player state before loading"),
              PlayerState->GetPlayerName(), FromManifest);

    // A save written before this character had a name. Deserialize must not apply the empty string.
    FSerializedCharacterData Older;
    Older.CharacterName = FString();
    FSerializedCharacterData::Deserialize(PlayerState, Older);
    TestEqual(TEXT("a save with no name leaves the manifest name standing"),
              PlayerState->GetPlayerName(), FromManifest);

    // A save that does carry a name is still allowed to restore it, or renaming could never persist.
    FSerializedCharacterData Named;
    Named.CharacterName = TEXT("Aldreth");
    FSerializedCharacterData::Deserialize(PlayerState, Named);
    TestEqual(TEXT("a save that carries a name restores it"), PlayerState->GetPlayerName(), TEXT("Aldreth"));

    return true;
}

#endif

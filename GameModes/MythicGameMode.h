// 

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/GameModeBase.h"
#include "MythicGameMode.generated.h"

class UMythicAbilitySystemComponent;
class UMythicSaveGameSubsystem;
/**
 *
 */
UCLASS()
class MYTHIC_API AMythicGameMode : public AGameModeBase {
    GENERATED_BODY()

public:
    // SERVER: respawn the controller's pawn after Delay seconds (0 = immediate). Safe to call on death.
    // A second request for the same controller cancels the first.
    UFUNCTION(BlueprintCallable, Category = "Mythic")
    void RequestRespawn(AController *Controller, float Delay = 5.0f);

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Logout(AController *Exiting) override;
    // CHARACTER load-on-join: the per-player mirror of the world load in AMythicGameState::BeginPlay. Authority
    // side, after possession completes (PlayerController + its proficiency/inventory components + PlayerState are
    // all ready), self-guarded by DoesSaveGameExist inside LoadCharacter.
    virtual void OnPostLogin(AController *NewPlayer) override;

    // ─── Debug accessors (Living World gameplay debugger, World State pane) ───
    // Server-only, game-thread reads of otherwise-protected timer state. Defined in the .cpp where the world timer
    // manager is reachable.

    /** Seconds until the next periodic autosave fires (negative/0 if the timer isn't active). */
    float GetAutosaveTimeRemaining() const;

    /** Number of controllers with a pending respawn timer. */
    int32 GetPendingRespawnCount() const { return RespawnTimers.Num(); }

protected:
    // Timer callback: restart the player and destroy the old (dead) pawn.
    void HandleRespawnTimer(AController *Controller);

    // Pending respawn timers keyed by controller (so a re-request cancels the prior one and Logout clears it).
    TMap<TWeakObjectPtr<AController>, FTimerHandle> RespawnTimers;

    // --- Save orchestration (writes). Persistence existed but only fired via cheat commands; this makes it
    // fire automatically. WORLD load-at-startup is wired in AMythicGameState::BeginPlay (the verified-safe
    // "world ready" point). CHARACTER load-on-join is now wired in OnPostLogin (the per-player mirror), with the
    // proficiency-XP apply race closed by UProficiencyComponent::ApplyLoadedProficiencies (re-applied from
    // CharacterData::Deserialize). The PlayerState(name)/PlayerController(proficiencies+inventory) split is
    // handled by ResolveCharacterActors. Remaining deferral: a per-account/EOS save slot (shared "DebugCharacter"
    // collides in true multiplayer). See Docs/BACKLOG.md.

    // Periodic server autosave: world state + every connected player's character. Concurrency (no two
    // background writes to the same slot) is gated inside the save subsystem, not here.
    void HandleAutosaveTimer();

    // Server-side save subsystem accessor (null off-authority / pre-GameInstance).
    UMythicSaveGameSubsystem *GetSaveSubsystem() const;

    FTimerHandle AutosaveTimerHandle;

    // Periodic autosave cadence (seconds).
    static constexpr float AutosaveIntervalSeconds = 300.0f;
};

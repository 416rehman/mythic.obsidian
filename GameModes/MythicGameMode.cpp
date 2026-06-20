//


#include "MythicGameMode.h"

#include "Mythic.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "Subsystem/SaveSystem/MythicSaveGameSubsystem.h"

namespace {
    // Local FString aliases of the canonical slot names (single source: UMythicSaveGameSubsystem) so cheat-saved,
    // auto-saved, and world-loaded data all share the same slots.
    const FString WorldSaveSlot = UMythicSaveGameSubsystem::DebugWorldSlot;
    const FString CharacterSaveSlot = UMythicSaveGameSubsystem::DebugCharacterSlot;
}

void AMythicGameMode::RequestRespawn(AController *Controller, float Delay) {
    if (!Controller || !HasAuthority()) {
        return;
    }

    // Cancel any pending respawn for this controller so a duplicate request (e.g. two lethal hits) can't
    // queue two restarts.
    if (FTimerHandle *Existing = RespawnTimers.Find(Controller)) {
        GetWorldTimerManager().ClearTimer(*Existing);
        RespawnTimers.Remove(Controller);
    }

    if (Delay <= 0.0f) {
        HandleRespawnTimer(Controller);
        return;
    }

    FTimerHandle &Handle = RespawnTimers.Add(Controller);
    FTimerDelegate Del = FTimerDelegate::CreateUObject(this, &AMythicGameMode::HandleRespawnTimer, Controller);
    GetWorldTimerManager().SetTimer(Handle, Del, Delay, false);
}

void AMythicGameMode::HandleRespawnTimer(AController *Controller) {
    RespawnTimers.Remove(Controller);

    if (!Controller || !HasAuthority()) {
        return;
    }

    // Release the old (dead) pawn so RestartPlayer spawns a fresh one, but DON'T destroy it yet - if the
    // respawn fails (no valid PlayerStart) we must not strand the controller pawnless.
    APawn *OldPawn = Controller->GetPawn();
    if (OldPawn) {
        Controller->UnPossess();
    }

    RestartPlayer(Controller);

    APawn *NewPawn = Controller->GetPawn();
    if (NewPawn && NewPawn != OldPawn) {
        // Respawn succeeded: destroy the old corpse.
        if (IsValid(OldPawn)) {
            OldPawn->Destroy();
        }
        return;
    }

    // Respawn produced no new pawn (missing/blocked PlayerStart). Recover by re-possessing and reviving the
    // old pawn rather than leaving the player stuck spectating, and surface the misconfiguration loudly.
    UE_LOG(Myth, Error, TEXT("MythicGameMode: respawn for %s produced no pawn (missing PlayerStart?); reviving old pawn."),
           *GetNameSafe(Controller));
    if (IsValid(OldPawn)) {
        if (Controller->GetPawn() != OldPawn) {
            Controller->Possess(OldPawn);
        }
        // Clear the death latch so the re-possessed pawn is alive again (otherwise bOutOfHealth keeps it an
        // immortal corpse).
        if (IAbilitySystemInterface *ASI = Cast<IAbilitySystemInterface>(OldPawn)) {
            if (UAbilitySystemComponent *ASC = ASI->GetAbilitySystemComponent()) {
                if (const UMythicAttributeSet_Life *Life = ASC->GetSet<UMythicAttributeSet_Life>()) {
                    const_cast<UMythicAttributeSet_Life *>(Life)->ResetForRespawn();
                }
            }
        }
    }
}

void AMythicGameMode::Logout(AController *Exiting) {
    if (FTimerHandle *Existing = RespawnTimers.Find(Exiting)) {
        GetWorldTimerManager().ClearTimer(*Existing);
        RespawnTimers.Remove(Exiting);
    }

    // Save-on-logout: persist the departing player's character while their PlayerState is still valid
    // (Super::Logout tears it down). Reuses the cheat path's exact call: SaveCharacter(PlayerState, Slot).
    // Single fixed slot until a per-player save key exists (see autosave note / BACKLOG).
    if (HasAuthority()) {
        if (APlayerController *PC = Cast<APlayerController>(Exiting)) {
            if (PC->PlayerState) {
                if (UMythicSaveGameSubsystem *SaveSys = GetSaveSubsystem()) {
                    SaveSys->SaveCharacter(PC->PlayerState, CharacterSaveSlot);
                }
            }
        }
    }

    Super::Logout(Exiting);
}

void AMythicGameMode::OnPostLogin(AController *NewPlayer) {
    // Super first so the default AGameModeBase possession / HUD / welcome flow completes before we touch state.
    Super::OnPostLogin(NewPlayer);

    // CHARACTER load-on-join: the per-player mirror of the world load (AMythicGameState::BeginPlay). At this point
    // possession is done, so the PlayerController + its UProficiencyComponent + inventory components have begun
    // play (slots initialized) and the PlayerState is valid. Pass the PlayerState exactly like the save callers so
    // ResolveCharacterActors handles the PS(name)->PC(proficiencies/inventory) split. LoadCharacter self-guards via
    // DoesSaveGameExist, so a brand-new character with no save is a clean no-op.
    if (!HasAuthority() || !NewPlayer || !NewPlayer->PlayerState) {
        return;
    }
    if (UMythicSaveGameSubsystem *SaveSys = GetSaveSubsystem()) {
        SaveSys->LoadCharacter(NewPlayer->PlayerState, CharacterSaveSlot);
    }
}

void AMythicGameMode::BeginPlay() {
    Super::BeginPlay();

    if (!HasAuthority()) {
        return;
    }

    // Arm the periodic autosave. Per-slot save concurrency is gated inside the save subsystem (it skips a slot
    // that already has a background write in flight), so no GameMode-side in-flight tracking is needed.
    GetWorldTimerManager().SetTimer(AutosaveTimerHandle, this, &AMythicGameMode::HandleAutosaveTimer, AutosaveIntervalSeconds, true);
}

void AMythicGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    GetWorldTimerManager().ClearTimer(AutosaveTimerHandle);

    Super::EndPlay(EndPlayReason);
}

void AMythicGameMode::HandleAutosaveTimer() {
    if (!HasAuthority()) {
        return;
    }
    UMythicSaveGameSubsystem *SaveSys = GetSaveSubsystem();
    if (!SaveSys) {
        return;
    }

    // World autosave. The subsystem skips a slot with a write already in flight, so a slow save can't be
    // stacked by the next tick.
    SaveSys->SaveWorld(WorldSaveSlot);

    // Character autosave for each connected player. Single fixed slot => last-writer-wins in multiplayer until
    // a per-player save key (session / character-select layer) exists; see Docs/BACKLOG.md. The subsystem's
    // per-slot in-flight gate prevents concurrent writes to the shared slot from racing/tearing the file.
    if (GameState) {
        for (APlayerState *PS : GameState->PlayerArray) {
            if (PS) {
                SaveSys->SaveCharacter(PS, CharacterSaveSlot);
            }
        }
    }
}

UMythicSaveGameSubsystem *AMythicGameMode::GetSaveSubsystem() const {
    UGameInstance *GI = GetGameInstance();
    return GI ? GI->GetSubsystem<UMythicSaveGameSubsystem>() : nullptr;
}

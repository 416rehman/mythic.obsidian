

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
#include "EngineUtils.h"
#include "Components/SkeletalMeshComponent.h"
#include "Itemization/Loot/MythicWorldItem.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "PhysicsEngine/PhysicalAnimationComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Subsystem/SaveSystem/MythicSaveGameSubsystem.h"
#include "Player/MythicPlayerState.h"
#include "UObject/Package.h"

namespace {
    const FString WorldSaveSlot = UMythicSaveGameSubsystem::DebugWorldSlot;

    /**
     * Deliberately never the OS login name. The character page treats a name starting with the OS user or the
     * machine name as a leaked identity and prints "Unnamed" instead - so seeding the manifest with it produced
     * a name the game refuses to show, and the header read "Unnamed" exactly as it did before the manifest was
     * ever read. It is also someone's real login name on a character sheet.
     *
     * A placeholder until a character-create screen supplies a chosen one; the manifest carries whatever it is
     * given, so that screen only has to call CreateNewCharacter with the typed name.
     */
    FString DefaultCharacterName(const AController *Player) {
        return TEXT("Adventurer");
    }
}

void AMythicGameMode::RequestRespawn(AController *Controller, float Delay) {
    if (!Controller || !HasAuthority()) {
        return;
    }

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

float AMythicGameMode::GetAutosaveTimeRemaining() const {
    if (const UWorld *W = GetWorld()) {
        return W->GetTimerManager().GetTimerRemaining(AutosaveTimerHandle);
    }
    return -1.0f;
}

void AMythicGameMode::HandleRespawnTimer(AController *Controller) {
    RespawnTimers.Remove(Controller);

    if (!Controller || !HasAuthority()) {
        return;
    }

    APawn *OldPawn = Controller->GetPawn();
    if (OldPawn) {
        Controller->UnPossess();
    }

    RestartPlayer(Controller);

    APawn *NewPawn = Controller->GetPawn();
    if (NewPawn && NewPawn != OldPawn) {
        if (IsValid(OldPawn)) {
            OldPawn->Destroy();
        }
        return;
    }

    UE_LOG(Myth, Error, TEXT("MythicGameMode: respawn for %s produced no pawn (missing PlayerStart?); reviving old pawn."),
           *GetNameSafe(Controller));
    if (IsValid(OldPawn)) {
        if (Controller->GetPawn() != OldPawn) {
            Controller->Possess(OldPawn);
        }
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

    if (HasAuthority() && GetWorld() && Exiting) {
        for (TActorIterator<AMythicWorldItem> It(GetWorld()); It; ++It) {
            AMythicWorldItem *WorldItem = *It;
            if (WorldItem && WorldItem->GetTargetRecipient() == Exiting) {
                WorldItem->SetOwner(nullptr);
                WorldItem->bOnlyRelevantToOwner = false;
                WorldItem->SetTargetRecipient(nullptr);
                WorldItem->FlushNetDormancy();
            }
        }
    }

    if (HasAuthority()) {
        if (APlayerController *PC = Cast<APlayerController>(Exiting)) {
            if (PC->PlayerState) {
                if (UMythicSaveGameSubsystem *SaveSys = GetSaveSubsystem()) {
                    SaveSys->SaveCharacter(PC->PlayerState, GetCharacterSlotForPlayer(PC->PlayerState));
                }
            }
        }
    }

    Super::Logout(Exiting);
}

void AMythicGameMode::OnPostLogin(AController *NewPlayer) {
    Super::OnPostLogin(NewPlayer);

    if (!HasAuthority() || !NewPlayer || !NewPlayer->PlayerState) {
        return;
    }
    if (UMythicSaveGameSubsystem *SaveSys = GetSaveSubsystem()) {
        const FString Slot = GetCharacterSlotForPlayer(NewPlayer->PlayerState);
        if (!UGameplayStatics::DoesSaveGameExist(Slot, 0)) {
            SaveSys->CreateNewCharacter(DefaultCharacterName(NewPlayer), TEXT(""), false, Slot);
        }

        // The manifest holds the name; without this it was written on creation and never read back, so the
        // player state kept the engine default - the machine name - and the header read "Unnamed" forever.
        // Applied before the load so a real saved name still wins, and an empty one no longer blanks it.
        for (const FMythicCharacterMetadata &Metadata : SaveSys->GetCharacterList()) {
            if (Metadata.CharacterID == Slot && !Metadata.DisplayName.IsEmpty()) {
                NewPlayer->PlayerState->SetPlayerName(Metadata.DisplayName);
                break;
            }
        }

        SaveSys->LoadCharacter(NewPlayer->PlayerState, Slot);
    }
}

void AMythicGameMode::BeginPlay() {
    Super::BeginPlay();

    if (!HasAuthority()) {
        return;
    }

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

    SaveSys->SaveWorld(WorldSaveSlot);

    if (GameState) {
        for (APlayerState *PS : GameState->PlayerArray) {
            if (PS) {
                SaveSys->SaveCharacter(PS, GetCharacterSlotForPlayer(PS));
            }
        }
    }
}

UMythicSaveGameSubsystem *AMythicGameMode::GetSaveSubsystem() const {
    UGameInstance *GI = GetGameInstance();
    return GI ? GI->GetSubsystem<UMythicSaveGameSubsystem>() : nullptr;
}

FString AMythicGameMode::GetCharacterSlotForPlayer(const APlayerState *PS) const {
    if (const AMythicPlayerState *MythPS = Cast<AMythicPlayerState>(PS)) {
        const FString &Persistent = MythPS->GetPersistentCharacterId();
        if (!Persistent.IsEmpty()) {
            return Persistent;
        }
    }

    FString StableId;
    if (PS) {
        const FUniqueNetIdRepl &NetId = PS->GetUniqueId();
        if (NetId.IsValid()) {
            StableId = NetId->ToString();
        }
    }
    return UMythicSaveGameSubsystem::ResolvePerPlayerCharacterSlot(StableId);
}

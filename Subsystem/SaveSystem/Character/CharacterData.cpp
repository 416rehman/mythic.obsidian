#include "CharacterData.h"
#include "Mythic/Mythic.h"
#include "Mythic/Player/MythicPlayerState.h"
#include "Mythic/Itemization/InventoryProviderInterface.h"
#include "Mythic/Itemization/Inventory/MythicInventoryComponent.h"
#include "Mythic/Player/Proficiency/ProficiencyComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

namespace {
    // Character data is split across two actors: the PlayerState holds the character name, while the
    // ProficiencyComponent + inventory provider live on the PlayerController. Callers may pass either, so
    // resolve both siblings here. (Previously callers passed only the PlayerState, which silently dropped
    // proficiencies because FindComponentByClass found no ProficiencyComponent on it.)
    void ResolveCharacterActors(AActor *InActor, APlayerState *&OutPS, APlayerController *&OutPC) {
        OutPS = Cast<APlayerState>(InActor);
        OutPC = Cast<APlayerController>(InActor);
        if (OutPS && !OutPC) {
            OutPC = OutPS->GetPlayerController();
        }
        else if (OutPC && !OutPS) {
            OutPS = OutPC->PlayerState;
        }
    }
}

bool FSerializedCharacterData::Serialize(AActor *SourceActor, FSerializedCharacterData &OutData) {
    if (!SourceActor) {
        UE_LOG(MythSaveLoad, Error, TEXT("SerializedCharacterData::Serialize: Invalid SourceActor"));
        return false;
    }

    APlayerState *PS = nullptr;
    APlayerController *PC = nullptr;
    ResolveCharacterActors(SourceActor, PS, PC);
    AActor *ProfHost = PC ? static_cast<AActor *>(PC) : SourceActor; // proficiencies + inventory live on the PC
    AActor *InvHost = PC ? static_cast<AActor *>(PC) : SourceActor;

    // Character Name
    if (PS) {
        OutData.CharacterName = PS->GetPlayerName();
    }

    // Pawn world transform (position + rotation), so a reload restores where the player was. Only captured when a
    // pawn is currently possessed; otherwise bHasSavedTransform stays false and load won't reposition.
    if (PC) {
        if (const APawn *Pawn = PC->GetPawn()) {
            OutData.SavedTransform = Pawn->GetActorTransform();
            OutData.bHasSavedTransform = true;
        }
    }

    // Proficiencies (save first - they track claimed levels)
    if (UProficiencyComponent *ProfComp = ProfHost->FindComponentByClass<UProficiencyComponent>()) {
        UE_LOG(MythSaveLoad, Log, TEXT("SerializedCharacterData::Serialize: Found ProficiencyComponent, serializing %d proficiencies..."),
               ProfComp->Proficiencies.Num());
        FSerializedProficiencyHelper::Serialize(ProfComp, OutData.Proficiencies);
        UE_LOG(MythSaveLoad, Log, TEXT("SerializedCharacterData::Serialize: Serialized %d proficiencies"), OutData.Proficiencies.Num());
    }
    else {
        UE_LOG(MythSaveLoad, Warning, TEXT("SerializedCharacterData::Serialize: No ProficiencyComponent found!"));
    }

    // Inventory - use IInventoryProviderInterface
    if (IInventoryProviderInterface *InvProvider = Cast<IInventoryProviderInterface>(InvHost)) {
        TArray<UMythicInventoryComponent *> InventoryComponents = InvProvider->GetAllInventoryComponents();
        UE_LOG(MythSaveLoad, Log, TEXT("SerializedCharacterData::Serialize: Found %d inventory components"), InventoryComponents.Num());

        for (UMythicInventoryComponent *InventoryComp : InventoryComponents) {
            if (InventoryComp) {
                FSerializedInventoryData InvData;
                FSerializedInventoryData::Serialize(InventoryComp, InvData);
                OutData.Inventories.Add(InvData);
                UE_LOG(MythSaveLoad, Log, TEXT("  - Serialized inventory '%s' with %d slots"), *InventoryComp->GetName(), InvData.Slots.Num());
            }
        }
    }
    else {
        UE_LOG(MythSaveLoad, Warning, TEXT("SerializedCharacterData::Serialize: SourceActor does not implement IInventoryProviderInterface!"));
    }

    // Note: Attributes are NOT saved - they are derived from proficiencies + items on load

    // Stamp the format version this save was actually written with. Was never set → every save recorded 0 (the default),
    // so FixupData spuriously "migrated 0→1" on every load (running migration logic against current-format data) and the
    // version guards were permanently inert.
    OutData.DataVersion = static_cast<int32>(CurrentCharacterSaveVersion);

    UE_LOG(MythSaveLoad, Log, TEXT("SerializedCharacterData::Serialize: Serialized character '%s'"), *OutData.CharacterName);
    return true;
}

bool FSerializedCharacterData::Deserialize(AActor *TargetActor, const FSerializedCharacterData &InData) {
    if (!TargetActor) {
        UE_LOG(MythSaveLoad, Error, TEXT("SerializedCharacterData::Deserialize: Invalid TargetActor"));
        return false;
    }

    // Load is server-authoritative — the restored name/proficiencies/inventory/transform replicate to the owning
    // client. Gate the WHOLE apply on authority (not just the transform sub-block), so the client-reachable cheat
    // path (MythLoadCharacter) can't half-apply state locally and desync. PlayerState/PlayerController both report
    // authority on the server, so this works even when no pawn is currently possessed.
    if (!TargetActor->HasAuthority()) {
        UE_LOG(MythSaveLoad, Warning, TEXT("SerializedCharacterData::Deserialize: ignored on non-authority actor [%s]"),
               *GetNameSafe(TargetActor));
        return false;
    }

    APlayerState *PS = nullptr;
    APlayerController *PC = nullptr;
    ResolveCharacterActors(TargetActor, PS, PC);
    AActor *ProfHost = PC ? static_cast<AActor *>(PC) : TargetActor;
    AActor *InvHost = PC ? static_cast<AActor *>(PC) : TargetActor;

    // Character Name
    if (PS) {
        PS->SetPlayerName(InData.CharacterName);
    }

    // Pawn world transform: move the (already-possessed) pawn back to where it was saved. Server-authoritative —
    // the move replicates to the owning client; TeleportPhysics avoids interpolation smearing across the jump.
    // Skipped for saves without a stored transform (bHasSavedTransform=false) so they keep the spawn position.
    if (InData.bHasSavedTransform && PC) {
        if (APawn *Pawn = PC->GetPawn()) {
            // Whole-function authority gate above already guarantees the server here.
            Pawn->SetActorTransform(InData.SavedTransform, /*bSweep=*/false, /*OutHit=*/nullptr, ETeleportType::TeleportPhysics);
        }
    }

    // Proficiencies. Deserialize only STAGES CurrentXP into FProficiency::SavedXP; the ASC write happens in
    // UProficiencyComponent::ApplyLoadedProficiencies. On load-on-join the component's BeginPlay has already run
    // (async load completes later), so we must re-apply here or the loaded XP never reaches the ASC. Idempotent.
    if (UProficiencyComponent *ProfComp = ProfHost->FindComponentByClass<UProficiencyComponent>()) {
        FSerializedProficiencyHelper::Deserialize(ProfComp, InData.Proficiencies);
        ProfComp->ApplyLoadedProficiencies();
        UE_LOG(MythSaveLoad, Log, TEXT("SerializedCharacterData::Deserialize: Restored %d proficiencies"), ProfComp->Proficiencies.Num());
    }

    // Inventory (items will apply their stats when equipped)
    if (IInventoryProviderInterface *InvProvider = Cast<IInventoryProviderInterface>(InvHost)) {
        TArray<UMythicInventoryComponent *> InventoryComponents = InvProvider->GetAllInventoryComponents();
        for (int32 i = 0; i < InventoryComponents.Num() && i < InData.Inventories.Num(); ++i) {
            if (InventoryComponents[i]) {
                FSerializedInventoryData::Deserialize(InventoryComponents[i], InData.Inventories[i]);
            }
        }
    }

    // Note: Attributes are derived - proficiencies reapply rewards, items apply stats when equipped

    UE_LOG(MythSaveLoad, Log, TEXT("SerializedCharacterData::Deserialize: Applied character '%s'"), *InData.CharacterName);
    return true;
}

#include "MythicSaveGameSubsystem.h"
#include "Mythic/Subsystem/SaveSystem/MythicSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Misc/SecureHash.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

#include "Mythic/Mythic.h"
#include "Mythic/Player/MythicPlayerState.h"
#include "Mythic/Itemization/Inventory/MythicInventoryComponent.h"
#include "Mythic/Player/Proficiency/ProficiencyComponent.h"
#include "Mythic/GameModes/GameState/MythicGameState.h"
#include "Mythic/Resources/MythicResourceManagerComponent.h"

FString UMythicSaveGameSubsystem::SanitizeSlotName(const FString &Input) {
    FString Safe = Input;

    // Remove path traversal attempts
    Safe.ReplaceInline(TEXT(".."), TEXT(""));
    Safe.ReplaceInline(TEXT("/"), TEXT("_"));
    Safe.ReplaceInline(TEXT("\\"), TEXT("_"));
    Safe.ReplaceInline(TEXT(":"), TEXT("_"));
    Safe.ReplaceInline(TEXT("*"), TEXT("_"));
    Safe.ReplaceInline(TEXT("?"), TEXT("_"));
    Safe.ReplaceInline(TEXT("\""), TEXT("_"));
    Safe.ReplaceInline(TEXT("<"), TEXT("_"));
    Safe.ReplaceInline(TEXT(">"), TEXT("_"));
    Safe.ReplaceInline(TEXT("|"), TEXT("_"));

    // Limit length
    if (Safe.Len() > 64) {
        Safe = Safe.Left(64);
    }

    // Ensure not empty
    if (Safe.IsEmpty()) {
        Safe = TEXT("InvalidSlot");
    }

    return Safe;
}

FString UMythicSaveGameSubsystem::ComputeChecksum(const TArray<uint8> &Data) {
    FSHAHash Hash;
    FSHA1::HashBuffer(Data.GetData(), Data.Num(), Hash.Hash);
    return Hash.ToString();
}

bool UMythicSaveGameSubsystem::ValidateChecksum(const TArray<uint8> &Data, const FString &ExpectedChecksum) {
    const FString ComputedChecksum = ComputeChecksum(Data);
    return ComputedChecksum.Equals(ExpectedChecksum, ESearchCase::IgnoreCase);
}

// ============================================================================

void UMythicSaveGameSubsystem::SaveCharacter(AActor *SourceActor, const FString &CharacterID) {
    if (!SourceActor || CharacterID.IsEmpty()) {
        UE_LOG(MythSaveLoad, Error, TEXT("SaveCharacter: Invalid SourceActor or CharacterID"));
        OnSaveGameActionFinished.Broadcast(CharacterID, false);
        return;
    }

    const FString SafeSlotName = SanitizeSlotName(CharacterID);

    // Notify start
    OnSaveGameActionStarted.Broadcast(SafeSlotName);

    // SERIALIZE ON GAME THREAD (CRITICAL)
    // We must read actor data while on the main thread to avoid race conditions/crashes
    FSerializedCharacterData CharacterData;
    if (!FSerializedCharacterData::Serialize(SourceActor, CharacterData)) {
        UE_LOG(MythSaveLoad, Error, TEXT("SaveCharacter: Failed to serialize character data"));
        OnSaveGameActionFinished.Broadcast(SafeSlotName, false);
        return;
    }
    CharacterData.CharacterID = SafeSlotName;

    // Create save object
    UMythicSaveGame *SaveObj = Cast<UMythicSaveGame>(UGameplayStatics::CreateSaveGameObject(UMythicSaveGame::StaticClass()));
    if (!SaveObj) {
        OnSaveGameActionFinished.Broadcast(SafeSlotName, false);
        return;
    }

    SaveObj->CharacterData = CharacterData;
    SaveObj->SaveSlotName = SafeSlotName;
    SaveObj->CreationTime = FDateTime::Now();

    // Compute checksum (Can be heavy, maybe move to async task if needed, but serialization requires Object access so safer here)
    TArray<uint8> TempBuffer;
    FMemoryWriter MemWriter(TempBuffer);
    FObjectAndNameAsStringProxyArchive Ar(MemWriter, false);
    SaveObj->Serialize(Ar);
    SaveObj->DataChecksum = ComputeChecksum(TempBuffer);

    // Define callback
    FAsyncSaveGameToSlotDelegate SavedDelegate;
    SavedDelegate.BindUObject(this, &UMythicSaveGameSubsystem::HandleAsyncSaveFinished);

    // Start Async Save
    UGameplayStatics::AsyncSaveGameToSlot(SaveObj, SafeSlotName, 0, SavedDelegate);
}

void UMythicSaveGameSubsystem::LoadCharacter(AActor *TargetActor, const FString &CharacterID) {
    if (!TargetActor || CharacterID.IsEmpty()) {
        UE_LOG(MythSaveLoad, Error, TEXT("LoadCharacter: Invalid TargetActor or CharacterID"));
        OnSaveGameActionFinished.Broadcast(CharacterID, false);
        return;
    }

    // Sanitize
    const FString SafeSlotName = SanitizeSlotName(CharacterID);

    // Notify start
    OnSaveGameActionStarted.Broadcast(SafeSlotName);

    if (!UGameplayStatics::DoesSaveGameExist(SafeSlotName, 0)) {
        UE_LOG(MythSaveLoad, Warning, TEXT("LoadCharacter: Save slot %s does not exist"), *SafeSlotName);
        OnSaveGameActionFinished.Broadcast(SafeSlotName, false);
        return;
    }

    // Track target actor for when load finishes
    PendingLoadTargets.Add(SafeSlotName, TargetActor);

    // Define callback
    FAsyncLoadGameFromSlotDelegate LoadedDelegate;
    LoadedDelegate.BindUObject(this, &UMythicSaveGameSubsystem::HandleAsyncLoadFinished);

    // Start Async Load
    UGameplayStatics::AsyncLoadGameFromSlot(SafeSlotName, 0, LoadedDelegate);
}

void UMythicSaveGameSubsystem::HandleAsyncSaveFinished(const FString &SlotName, const int32 UserIndex, bool bSuccess) {
    UE_LOG(MythSaveLoad, Log, TEXT("Async Save Finished for %s: %s"), *SlotName, bSuccess ? TEXT("Success") : TEXT("Failed"));
    OnSaveGameActionFinished.Broadcast(SlotName, bSuccess);
}

void UMythicSaveGameSubsystem::HandleAsyncLoadFinished(const FString &SlotName, const int32 UserIndex, USaveGame *LoadedSaveGame) {
    // Check if we still have a valid target actor
    TWeakObjectPtr<AActor> *TargetPtr = PendingLoadTargets.Find(SlotName);
    AActor *TargetActor = TargetPtr ? TargetPtr->Get() : nullptr;
    PendingLoadTargets.Remove(SlotName); // Clean up

    if (!TargetActor) {
        UE_LOG(MythSaveLoad, Warning, TEXT("AsyncLoadFinished: Target Actor for %s is no longer valid or was cancelled"), *SlotName);
        OnSaveGameActionFinished.Broadcast(SlotName, false);
        return;
    }

    UMythicSaveGame *SaveObj = Cast<UMythicSaveGame>(LoadedSaveGame);
    if (!SaveObj) {
        UE_LOG(MythSaveLoad, Error, TEXT("AsyncLoadFinished: Failed to cast loaded object"));
        OnSaveGameActionFinished.Broadcast(SlotName, false);
        return;
    }

    // Validate (Compute checksum on Game Thread - acceptable for now)
    if (SaveObj->DataChecksum.IsEmpty()) {
        UE_LOG(MythSaveLoad, Error, TEXT("AsyncLoadFinished: No checksum"));
        OnSaveGameActionFinished.Broadcast(SlotName, false);
        return;
    }

    TArray<uint8> TempBuffer;
    FString StoredChecksum = SaveObj->DataChecksum;
    SaveObj->DataChecksum = TEXT("");

    FMemoryWriter MemWriter(TempBuffer);
    FObjectAndNameAsStringProxyArchive Ar(MemWriter, false);
    SaveObj->Serialize(Ar);

    if (!ValidateChecksum(TempBuffer, StoredChecksum)) {
        UE_LOG(MythSaveLoad, Error, TEXT("AsyncLoadFinished: Checksum failed for %s"), *SlotName);
        OnSaveGameActionFinished.Broadcast(SlotName, false);
        return;
    }

    SaveObj->DataChecksum = StoredChecksum;
    SaveObj->FixupData();

    // Deserialize to Actor
    // This MUST happen on GameThread (which we are on in this callback)
    if (!FSerializedCharacterData::Deserialize(TargetActor, SaveObj->CharacterData)) {
        UE_LOG(MythSaveLoad, Error, TEXT("AsyncLoadFinished: Failed to deserialize"));
        OnSaveGameActionFinished.Broadcast(SlotName, false);
        return;
    }

    UE_LOG(MythSaveLoad, Log, TEXT("AsyncLoadFinished: Successfully loaded %s"), *SlotName);
    OnSaveGameActionFinished.Broadcast(SlotName, true);
}


TArray<FString> UMythicSaveGameSubsystem::GetLocalSaveFiles() const {
    TArray<FString> Result;
    FindSaveGames(Result);
    return Result;
}

// ============================================================================
// CHARACTER DATA SERIALIZATION (FOR NETWORK TRANSFER)
// ============================================================================

bool UMythicSaveGameSubsystem::SerializeCharacterToStruct(AActor *SourceActor, FSerializedCharacterData &OutData) {
    return FSerializedCharacterData::Serialize(SourceActor, OutData);
}

bool UMythicSaveGameSubsystem::DeserializeCharacterFromStruct(AActor *TargetActor, const FSerializedCharacterData &InData) {
    return FSerializedCharacterData::Deserialize(TargetActor, InData);
}

bool UMythicSaveGameSubsystem::ValidateCharacterData(const FSerializedCharacterData &InData, FString &OutError) {
    // Check version
    if (InData.DataVersion > static_cast<int32>(CurrentCharacterSaveVersion)) {
        OutError = FString::Printf(TEXT("Data version %d is newer than current %d"),
                                   InData.DataVersion, static_cast<int32>(CurrentCharacterSaveVersion));
        return false;
    }

    // Check character name length
    if (InData.CharacterName.Len() > 64) {
        OutError = TEXT("Character name too long (max 64 chars)");
        return false;
    }

    // Check inventory slot counts
    int32 TotalSlots = 0;
    for (const FSerializedInventoryData &InvData : InData.Inventories) {
        TotalSlots += InvData.Slots.Num();
    }
    if (TotalSlots > 1000) {
        OutError = TEXT("Too many total inventory slots (max 1000)");
        return false;
    }

    return true;
}

// ============================================================================

// ============================================================================

// ============================================================================

// ============================================================================

// WORLD SAVE/LOAD
// ============================================================================

void UMythicSaveGameSubsystem::SaveWorld(const FString &SlotName) {
    if (SlotName.IsEmpty()) {
        UE_LOG(MythSaveLoad, Error, TEXT("SaveWorld: SlotName is empty"));
        OnSaveGameActionFinished.Broadcast(SlotName, false);
        return;
    }

    const FString SafeSlotName = SanitizeSlotName(SlotName);
    OnSaveGameActionStarted.Broadcast(SafeSlotName);

    UWorld *World = GetWorld();
    if (!World) {
        OnSaveGameActionFinished.Broadcast(SafeSlotName, false);
        return;
    }

    // SERIALIZE ON GAME THREAD
    FSerializedWorldData WorldData;
    WorldData.TimeOfDay = World->GetTimeSeconds();

    // GameState Resources
    if (AMythicGameState *GameState = World->GetGameState<AMythicGameState>()) {
        if (UMythicResourceManagerComponent *ResMgr = GameState->FindComponentByClass<UMythicResourceManagerComponent>()) {
            FSerializedDestructibleHelper::Serialize(ResMgr, WorldData.DestroyedResources);
        }
    }

    // Saveable Actors
    FSerializedWorldActorHelper::SerializeAll(World, WorldData.SavedActors);

    // Create save object
    UMythicSaveGame *SaveObj = Cast<UMythicSaveGame>(UGameplayStatics::CreateSaveGameObject(UMythicSaveGame::StaticClass()));
    if (!SaveObj) {
        OnSaveGameActionFinished.Broadcast(SafeSlotName, false);
        return;
    }

    SaveObj->WorldData = WorldData;
    SaveObj->SaveSlotName = SafeSlotName;
    SaveObj->CreationTime = FDateTime::Now();

    // Checksum
    TArray<uint8> TempBuffer;
    FMemoryWriter MemWriter(TempBuffer);
    FObjectAndNameAsStringProxyArchive Ar(MemWriter, false);
    SaveObj->Serialize(Ar);
    SaveObj->DataChecksum = ComputeChecksum(TempBuffer);

    // Define callback (reuse same handler as character, logic is generic)
    FAsyncSaveGameToSlotDelegate SavedDelegate;
    SavedDelegate.BindUObject(this, &UMythicSaveGameSubsystem::HandleAsyncSaveFinished);

    UGameplayStatics::AsyncSaveGameToSlot(SaveObj, SafeSlotName, 0, SavedDelegate);
}

void UMythicSaveGameSubsystem::LoadWorld(const FString &SlotName) {
    if (SlotName.IsEmpty()) {
        UE_LOG(MythSaveLoad, Error, TEXT("LoadWorld: SlotName is empty"));
        OnSaveGameActionFinished.Broadcast(SlotName, false);
        return;
    }

    const FString SafeSlotName = SanitizeSlotName(SlotName);
    OnSaveGameActionStarted.Broadcast(SafeSlotName);

    if (!UGameplayStatics::DoesSaveGameExist(SafeSlotName, 0)) {
        UE_LOG(MythSaveLoad, Warning, TEXT("LoadWorld: Slot %s does not exist"), *SafeSlotName);
        OnSaveGameActionFinished.Broadcast(SafeSlotName, false);
        return;
    }

    // Define callback
    FAsyncLoadGameFromSlotDelegate LoadedDelegate;
    LoadedDelegate.BindUObject(this, &UMythicSaveGameSubsystem::HandleAsyncWorldLoadFinished);

    UGameplayStatics::AsyncLoadGameFromSlot(SafeSlotName, 0, LoadedDelegate);
}

void UMythicSaveGameSubsystem::HandleAsyncWorldLoadFinished(const FString &SlotName, const int32 UserIndex, USaveGame *LoadedSaveGame) {
    UMythicSaveGame *SaveObj = Cast<UMythicSaveGame>(LoadedSaveGame);
    if (!SaveObj) {
        UE_LOG(MythSaveLoad, Error, TEXT("AsyncWorldLoadFinished: Failed to cast"));
        OnSaveGameActionFinished.Broadcast(SlotName, false);
        return;
    }

    // Validate
    if (SaveObj->DataChecksum.IsEmpty()) {
        OnSaveGameActionFinished.Broadcast(SlotName, false);
        return;
    }

    TArray<uint8> TempBuffer;
    FString StoredChecksum = SaveObj->DataChecksum;
    SaveObj->DataChecksum = TEXT("");
    FMemoryWriter MemWriter(TempBuffer);
    FObjectAndNameAsStringProxyArchive Ar(MemWriter, false);
    SaveObj->Serialize(Ar);

    if (!ValidateChecksum(TempBuffer, StoredChecksum)) {
        UE_LOG(MythSaveLoad, Error, TEXT("AsyncWorldLoadFinished: Checksum failed"));
        OnSaveGameActionFinished.Broadcast(SlotName, false);
        return;
    }
    SaveObj->DataChecksum = StoredChecksum;
    SaveObj->FixupData();

    // DESERIALIZE ON GAME THREAD
    UWorld *World = GetWorld();
    if (World) {
        const FSerializedWorldData &Data = SaveObj->WorldData;

        // GameState Resources
        if (AMythicGameState *GameState = World->GetGameState<AMythicGameState>()) {
            if (UMythicResourceManagerComponent *ResMgr = GameState->FindComponentByClass<UMythicResourceManagerComponent>()) {
                FSerializedDestructibleHelper::Deserialize(ResMgr, Data.DestroyedResources);
            }
        }

        // Saveable Actors
        FSerializedWorldActorHelper::DeserializeAll(World, Data.SavedActors);
    }

    UE_LOG(MythSaveLoad, Log, TEXT("AsyncWorldLoadFinished: Success for %s"), *SlotName);
    OnSaveGameActionFinished.Broadcast(SlotName, true);
}


void UMythicSaveGameSubsystem::FindSaveGames(TArray<FString> &OutSaveFiles) const {
    const FString SavesFolder = FPaths::ProjectSavedDir() / TEXT("SaveGames");
    const FString Extension = TEXT("*.sav");

    IFileManager::Get().FindFiles(OutSaveFiles, *(SavesFolder / Extension), true, false);

    for (FString &File : OutSaveFiles) {
        File = FPaths::GetBaseFilename(File);
    }
}

// ============================================================================
// CHARACTER MANIFEST (MENU SYSTEM)
// ============================================================================

TArray<FMythicCharacterMetadata> UMythicSaveGameSubsystem::GetCharacterList() {
    TArray<FMythicCharacterMetadata> Result;

    if (UMythicSaveGameManifest *Manifest = Cast<UMythicSaveGameManifest>(UGameplayStatics::LoadGameFromSlot(TEXT("MythicCharacterManifest"), 0))) {
        Manifest->CharacterSlots.GenerateValueArray(Result);
    }

    // Sort by LastPlayed (Newest first)
    Result.Sort([](const FMythicCharacterMetadata &A, const FMythicCharacterMetadata &B) {
        return A.LastPlayed > B.LastPlayed;
    });

    return Result;
}

FString UMythicSaveGameSubsystem::CreateNewCharacter(const FString &DisplayName, const FString &ClassName, bool bHardcore) {
    // Generate unique ID
    FString NewSlotName = FGuid::NewGuid().ToString();

    // Create Initial Metadata
    FMythicCharacterMetadata NewChar;
    NewChar.CharacterID = NewSlotName;
    NewChar.DisplayName = DisplayName;
    NewChar.ClassName = ClassName;
    NewChar.bIsHardcore = bHardcore;
    NewChar.Level = 1;
    NewChar.LastPlayed = FDateTime::Now();

    // Create Empty Save File immediately to reserve the slot
    UMythicSaveGame *NewSave = Cast<UMythicSaveGame>(UGameplayStatics::CreateSaveGameObject(UMythicSaveGame::StaticClass()));
    if (NewSave) {
        NewSave->SaveSlotName = NewSlotName;
        NewSave->CreationTime = NewChar.LastPlayed;
        UGameplayStatics::SaveGameToSlot(NewSave, NewSlotName, 0);
    }

    // Update Manifest
    UpdateManifestInternal(NewChar);

    return NewSlotName;
}

bool UMythicSaveGameSubsystem::DeleteCharacter(const FString &CharacterID) {
    if (CharacterID.IsEmpty()) {
        return false;
    }

    // Remove from Manifest
    FMythicCharacterMetadata Dummy;
    Dummy.CharacterID = CharacterID;
    UpdateManifestInternal(Dummy, true);

    // Delete Physical File
    const FString SafeSlot = SanitizeSlotName(CharacterID);
    if (UGameplayStatics::DoesSaveGameExist(SafeSlot, 0)) {
        return UGameplayStatics::DeleteGameInSlot(SafeSlot, 0);
    }

    return true;
}

void UMythicSaveGameSubsystem::UpdateManifestInternal(const FMythicCharacterMetadata &Metadata, bool bRemove) {
    UMythicSaveGameManifest *Manifest = Cast<UMythicSaveGameManifest>(UGameplayStatics::LoadGameFromSlot(TEXT("MythicCharacterManifest"), 0));

    if (!Manifest) {
        Manifest = Cast<UMythicSaveGameManifest>(UGameplayStatics::CreateSaveGameObject(UMythicSaveGameManifest::StaticClass()));
    }

    if (!Manifest) {
        return;
    }

    if (bRemove) {
        Manifest->CharacterSlots.Remove(Metadata.CharacterID);
    }
    else {
        Manifest->CharacterSlots.Add(Metadata.CharacterID, Metadata);
    }

    UGameplayStatics::SaveGameToSlot(Manifest, TEXT("MythicCharacterManifest"), 0);
}

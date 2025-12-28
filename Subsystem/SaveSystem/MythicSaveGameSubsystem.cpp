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

bool UMythicSaveGameSubsystem::SaveCharacter(AActor *SourceActor, const FString &CharacterID) {
    if (!SourceActor || CharacterID.IsEmpty()) {
        UE_LOG(MythSaveLoad, Error, TEXT("SaveCharacter: Invalid SourceActor or CharacterID"));
        return false;
    }

    // Sanitize the slot name for security
    const FString SafeSlotName = SanitizeSlotName(CharacterID);

    // Serialize character data using shared helper
    FSerializedCharacterData CharacterData;
    if (!FSerializedCharacterData::Serialize(SourceActor, CharacterData)) {
        {
            UE_LOG(MythSaveLoad, Error, TEXT("SaveCharacter: Failed to serialize character data"));
            return false;
        }
    }

    CharacterData.CharacterID = SafeSlotName;

    // Create save object
    UMythicSaveGame *SaveObj = Cast<UMythicSaveGame>(UGameplayStatics::CreateSaveGameObject(UMythicSaveGame::StaticClass()));
    if (!SaveObj) {
        UE_LOG(MythSaveLoad, Error, TEXT("SaveCharacter: Failed to create SaveGame Object"));
        return false;
    }

    SaveObj->CharacterData = CharacterData;
    SaveObj->SaveSlotName = SafeSlotName;
    SaveObj->CreationTime = FDateTime::Now();

    // Compute checksum for data integrity
    TArray<uint8> TempBuffer;
    FMemoryWriter MemWriter(TempBuffer);
    FObjectAndNameAsStringProxyArchive Ar(MemWriter, false);
    SaveObj->Serialize(Ar);
    SaveObj->DataChecksum = ComputeChecksum(TempBuffer);

    const bool bSuccess = UGameplayStatics::SaveGameToSlot(SaveObj, SafeSlotName, 0);
    UE_LOG(MythSaveLoad, Log, TEXT("SaveCharacter: %s for %s"), bSuccess ? TEXT("Success") : TEXT("Failed"), *SafeSlotName);
    return bSuccess;
}

bool UMythicSaveGameSubsystem::LoadCharacter(AActor *TargetActor, const FString &CharacterID) {
    if (!TargetActor || CharacterID.IsEmpty()) {
        UE_LOG(MythSaveLoad, Error, TEXT("LoadCharacter: Invalid TargetActor or CharacterID"));
        return false;
    }

    // Sanitize the slot name for security
    const FString SafeSlotName = SanitizeSlotName(CharacterID);

    if (!UGameplayStatics::DoesSaveGameExist(SafeSlotName, 0)) {
        UE_LOG(MythSaveLoad, Warning, TEXT("LoadCharacter: Save slot %s does not exist"), *SafeSlotName);
        return false;
    }

    UMythicSaveGame *SaveObj = Cast<UMythicSaveGame>(UGameplayStatics::LoadGameFromSlot(SafeSlotName, 0));
    if (!SaveObj) {
        UE_LOG(MythSaveLoad, Error, TEXT("LoadCharacter: Failed to load SaveGame Object"));
        return false;
    }

    // Validate checksum
    if (SaveObj->DataChecksum.IsEmpty()) {
        UE_LOG(MythSaveLoad, Error, TEXT("LoadCharacter: Save slot %s does not have a checksum"), *SafeSlotName);
        return false;
    }

    TArray<uint8> TempBuffer;
    FString StoredChecksum = SaveObj->DataChecksum;
    SaveObj->DataChecksum = TEXT(""); // Clear for comparison

    FMemoryWriter MemWriter(TempBuffer);
    FObjectAndNameAsStringProxyArchive Ar(MemWriter, false);
    SaveObj->Serialize(Ar);

    if (!ValidateChecksum(TempBuffer, StoredChecksum)) {
        UE_LOG(MythSaveLoad, Error, TEXT("LoadCharacter: Checksum validation FAILED for %s - possible data tampering!"), *SafeSlotName);
        return false;
    }

    SaveObj->DataChecksum = StoredChecksum; // Restore
    UE_LOG(MythSaveLoad, Log, TEXT("LoadCharacter: Checksum validated for %s"), *SafeSlotName);

    SaveObj->FixupData();

    // Use shared deserialization helper
    if (!FSerializedCharacterData::Deserialize(TargetActor, SaveObj->CharacterData)) {
        UE_LOG(MythSaveLoad, Error, TEXT("LoadCharacter: Failed to deserialize character data"));
        return false;
    }

    UE_LOG(MythSaveLoad, Log, TEXT("LoadCharacter: Successfully loaded %s"), *SafeSlotName);
    return true;
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

// WORLD SAVE/LOAD
// ============================================================================

bool UMythicSaveGameSubsystem::SaveWorldState(const FString &SlotName) {
    if (SlotName.IsEmpty()) {
        UE_LOG(MythSaveLoad, Error, TEXT("SaveWorldState: SlotName is empty"));
        return false;
    }

    const FString SafeSlotName = SanitizeSlotName(SlotName);

    UWorld *World = GetWorld();
    if (!World) {
        UE_LOG(MythSaveLoad, Error, TEXT("SaveWorldState: No World"));
        return false;
    }

    FSerializedWorldData WorldData;
    WorldData.TimeOfDay = World->GetTimeSeconds();

    // GameState Resources
    if (AMythicGameState *GameState = World->GetGameState<AMythicGameState>()) {
        if (UMythicResourceManagerComponent *ResMgr = GameState->FindComponentByClass<UMythicResourceManagerComponent>()) {
            FSerializedDestructibleHelper::Serialize(ResMgr, WorldData.DestroyedResources);
        }
    }

    // Saveable Actors (both placed and runtime)
    FSerializedWorldActorHelper::SerializeAll(World, WorldData.SavedActors);

    // Create save object
    UMythicSaveGame *SaveObj = Cast<UMythicSaveGame>(UGameplayStatics::CreateSaveGameObject(UMythicSaveGame::StaticClass()));
    if (!SaveObj) {
        UE_LOG(MythSaveLoad, Error, TEXT("SaveWorldState: Failed to create SaveGame Object"));
        return false;
    }

    SaveObj->WorldData = WorldData;
    SaveObj->SaveSlotName = SafeSlotName;
    SaveObj->CreationTime = FDateTime::Now();

    // Compute checksum
    TArray<uint8> TempBuffer;
    FMemoryWriter MemWriter(TempBuffer);
    FObjectAndNameAsStringProxyArchive Ar(MemWriter, false);
    SaveObj->Serialize(Ar);
    SaveObj->DataChecksum = ComputeChecksum(TempBuffer);

    const bool bSuccess = UGameplayStatics::SaveGameToSlot(SaveObj, SafeSlotName, 0);
    UE_LOG(MythSaveLoad, Log, TEXT("SaveWorldState: %s for %s"), bSuccess ? TEXT("Success") : TEXT("Failed"), *SafeSlotName);
    return bSuccess;
}

bool UMythicSaveGameSubsystem::LoadWorldState(const FString &SlotName) {
    if (SlotName.IsEmpty()) {
        UE_LOG(MythSaveLoad, Error, TEXT("LoadWorldState: SlotName is empty"));
        return false;
    }

    // Sanitize the slot name for security
    const FString SafeSlotName = SanitizeSlotName(SlotName);

    if (!UGameplayStatics::DoesSaveGameExist(SafeSlotName, 0)) {
        UE_LOG(MythSaveLoad, Warning, TEXT("LoadWorldState: Slot %s does not exist"), *SafeSlotName);
        return false;
    }

    UMythicSaveGame *SaveObj = Cast<UMythicSaveGame>(UGameplayStatics::LoadGameFromSlot(SafeSlotName, 0));
    if (!SaveObj) {
        UE_LOG(MythSaveLoad, Error, TEXT("LoadWorldState: Failed to load SaveGame Object"));
        return false;
    }

    // Validate checksum (if present)
    if (!SaveObj->DataChecksum.IsEmpty()) {
        TArray<uint8> TempBuffer;
        FString StoredChecksum = SaveObj->DataChecksum;
        SaveObj->DataChecksum = TEXT("");

        FMemoryWriter MemWriter(TempBuffer);
        FObjectAndNameAsStringProxyArchive Ar(MemWriter, false);
        SaveObj->Serialize(Ar);

        if (!ValidateChecksum(TempBuffer, StoredChecksum)) {
            UE_LOG(MythSaveLoad, Error, TEXT("LoadWorldState: Checksum validation FAILED for %s - possible data tampering!"), *SafeSlotName);
            return false;
        }

        SaveObj->DataChecksum = StoredChecksum;
        UE_LOG(MythSaveLoad, Log, TEXT("LoadWorldState: Checksum validated for %s"), *SafeSlotName);
    }

    UWorld *World = GetWorld();
    if (!World) {
        UE_LOG(MythSaveLoad, Error, TEXT("LoadWorldState: No World"));
        return false;
    }

    SaveObj->FixupData();
    const FSerializedWorldData &Data = SaveObj->WorldData;

    // GameState Resources
    if (AMythicGameState *GameState = World->GetGameState<AMythicGameState>()) {
        if (UMythicResourceManagerComponent *ResMgr = GameState->FindComponentByClass<UMythicResourceManagerComponent>()) {
            FSerializedDestructibleHelper::Deserialize(ResMgr, Data.DestroyedResources);
        }
    }

    // Saveable Actors (both placed and runtime)
    FSerializedWorldActorHelper::DeserializeAll(World, Data.SavedActors);

    UE_LOG(MythSaveLoad, Log, TEXT("LoadWorldState: Successfully loaded %s"), *SafeSlotName);
    return true;
}

void UMythicSaveGameSubsystem::FindSaveGames(TArray<FString> &OutSaveFiles) const {
    const FString SavesFolder = FPaths::ProjectSavedDir() / TEXT("SaveGames");
    const FString Extension = TEXT("*.sav");

    IFileManager::Get().FindFiles(OutSaveFiles, *(SavesFolder / Extension), true, false);

    for (FString &File : OutSaveFiles) {
        File = FPaths::GetBaseFilename(File);
    }
}

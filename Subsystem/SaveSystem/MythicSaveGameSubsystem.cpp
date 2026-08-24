#include "MythicSaveGameSubsystem.h"
#include "Mythic/Subsystem/SaveSystem/MythicSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Misc/SecureHash.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

#include "Mythic/Mythic.h"
#include "Mythic/Player/MythicPlayerState.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Mythic/Itemization/Inventory/MythicInventoryComponent.h"
#include "Mythic/Player/Proficiency/ProficiencyComponent.h"
#include "Mythic/GameModes/GameState/MythicGameState.h"
#include "Mythic/Resources/MythicResourceManagerComponent.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/POI/MythicPOIDiscoverySubsystem.h"
#include "World/Digging/MythicDiggingSubsystem.h"
#include "World/LivingWorld/MythicWorldStateSubsystem.h"
#include "AI/Party/PartySubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Serialization/MemoryReader.h"

FString UMythicSaveGameSubsystem::SanitizeSlotName(const FString &Input) {
    FString Safe = Input;

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

    if (Safe.Len() > 64) {
        Safe = Safe.Left(64);
    }

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


void UMythicSaveGameSubsystem::SaveCharacter(AActor *SourceActor, const FString &CharacterID) {
    if (!SourceActor || CharacterID.IsEmpty()) {
        UE_LOG(MythSaveLoad, Error, TEXT("SaveCharacter: Invalid SourceActor or CharacterID"));
        OnSaveGameActionFinished.Broadcast(CharacterID, false);
        return;
    }

    const FString SafeSlotName = SanitizeSlotName(CharacterID);

    if (InFlightSaveSlots.Contains(SafeSlotName)) {
        UE_LOG(MythSaveLoad, Warning, TEXT("SaveCharacter: a save to slot '%s' is already in flight; skipping concurrent write."), *SafeSlotName);
        OnSaveGameActionFinished.Broadcast(SafeSlotName, false);
        return;
    }

    OnSaveGameActionStarted.Broadcast(SafeSlotName);

    FSerializedCharacterData CharacterData;
    if (!FSerializedCharacterData::Serialize(SourceActor, CharacterData)) {
        UE_LOG(MythSaveLoad, Error, TEXT("SaveCharacter: Failed to serialize character data"));
        OnSaveGameActionFinished.Broadcast(SafeSlotName, false);
        return;
    }
    CharacterData.CharacterID = SafeSlotName;

    UMythicSaveGame *SaveObj = Cast<UMythicSaveGame>(UGameplayStatics::CreateSaveGameObject(UMythicSaveGame::StaticClass()));
    if (!SaveObj) {
        OnSaveGameActionFinished.Broadcast(SafeSlotName, false);
        return;
    }

    SaveObj->CharacterData = CharacterData;
    SaveObj->SaveSlotName = SafeSlotName;
    SaveObj->CreationTime = FDateTime::Now();

    TArray<uint8> TempBuffer;
    FMemoryWriter MemWriter(TempBuffer);
    FObjectAndNameAsStringProxyArchive Ar(MemWriter, false);
    SaveObj->Serialize(Ar);
    SaveObj->DataChecksum = ComputeChecksum(TempBuffer);

    FAsyncSaveGameToSlotDelegate SavedDelegate;
    SavedDelegate.BindUObject(this, &UMythicSaveGameSubsystem::HandleAsyncSaveFinished);

    InFlightSaveSlots.Add(SafeSlotName);
    UGameplayStatics::AsyncSaveGameToSlot(SaveObj, SafeSlotName, 0, SavedDelegate);
}

void UMythicSaveGameSubsystem::LoadCharacter(AActor *TargetActor, const FString &CharacterID) {
    if (!TargetActor || CharacterID.IsEmpty()) {
        UE_LOG(MythSaveLoad, Error, TEXT("LoadCharacter: Invalid TargetActor or CharacterID"));
        OnSaveGameActionFinished.Broadcast(CharacterID, false);
        return;
    }

    const FString SafeSlotName = SanitizeSlotName(CharacterID);

    OnSaveGameActionStarted.Broadcast(SafeSlotName);

    if (!UGameplayStatics::DoesSaveGameExist(SafeSlotName, 0)) {
        UE_LOG(MythSaveLoad, Log, TEXT("LoadCharacter: Save slot %s does not exist (first run for this character)"), *SafeSlotName);
        OnSaveGameActionFinished.Broadcast(SafeSlotName, false);
        return;
    }

    PendingLoadTargets.Add(SafeSlotName, TargetActor);

    FAsyncLoadGameFromSlotDelegate LoadedDelegate;
    LoadedDelegate.BindUObject(this, &UMythicSaveGameSubsystem::HandleAsyncLoadFinished);

    UGameplayStatics::AsyncLoadGameFromSlot(SafeSlotName, 0, LoadedDelegate);
}

void UMythicSaveGameSubsystem::HandleAsyncSaveFinished(const FString &SlotName, const int32 UserIndex, bool bSuccess) {
    InFlightSaveSlots.Remove(SlotName);
    UE_LOG(MythSaveLoad, Log, TEXT("Async Save Finished for %s: %s"), *SlotName, bSuccess ? TEXT("Success") : TEXT("Failed"));
    OnSaveGameActionFinished.Broadcast(SlotName, bSuccess);
}

void UMythicSaveGameSubsystem::HandleAsyncLoadFinished(const FString &SlotName, const int32 UserIndex, USaveGame *LoadedSaveGame) {
    TWeakObjectPtr<AActor> *TargetPtr = PendingLoadTargets.Find(SlotName);
    AActor *TargetActor = TargetPtr ? TargetPtr->Get() : nullptr;
    PendingLoadTargets.Remove(SlotName);

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

    FString ValidationError;
    if (!ValidateCharacterData(SaveObj->CharacterData, ValidationError)) {
        UE_LOG(MythSaveLoad, Error, TEXT("AsyncLoadFinished: Invalid character data for %s: %s"), *SlotName, *ValidationError);
        OnSaveGameActionFinished.Broadcast(SlotName, false);
        return;
    }

    if (!FSerializedCharacterData::Deserialize(TargetActor, SaveObj->CharacterData)) {
        UE_LOG(MythSaveLoad, Error, TEXT("AsyncLoadFinished: Failed to deserialize"));
        OnSaveGameActionFinished.Broadcast(SlotName, false);
        return;
    }

    AMythicPlayerState *MythPS = nullptr;
    if (const APawn *Pawn = Cast<APawn>(TargetActor)) {
        MythPS = Pawn->GetPlayerState<AMythicPlayerState>();
    }
    else if (const APlayerController *PC = Cast<APlayerController>(TargetActor)) {
        MythPS = PC->GetPlayerState<AMythicPlayerState>();
    }
    else {
        MythPS = Cast<AMythicPlayerState>(TargetActor);
    }
    if (MythPS) {
        MythPS->SetPersistentCharacterId(SlotName);
    }

    UE_LOG(MythSaveLoad, Log, TEXT("AsyncLoadFinished: Successfully loaded %s"), *SlotName);
    OnSaveGameActionFinished.Broadcast(SlotName, true);
}


TArray<FString> UMythicSaveGameSubsystem::GetLocalSaveFiles() const {
    TArray<FString> Result;
    FindSaveGames(Result);
    return Result;
}

FString UMythicSaveGameSubsystem::ResolvePerPlayerCharacterSlot(const FString &StablePlayerId) {
    return StablePlayerId.IsEmpty() ? FString(DebugCharacterSlot) : FString::Printf(TEXT("Character_%s"), *StablePlayerId);
}


bool UMythicSaveGameSubsystem::SerializeCharacterToStruct(AActor *SourceActor, FSerializedCharacterData &OutData) {
    return FSerializedCharacterData::Serialize(SourceActor, OutData);
}

bool UMythicSaveGameSubsystem::DeserializeCharacterFromStruct(AActor *TargetActor, const FSerializedCharacterData &InData) {
    return FSerializedCharacterData::Deserialize(TargetActor, InData);
}

bool UMythicSaveGameSubsystem::ValidateCharacterData(const FSerializedCharacterData &InData, FString &OutError) {
    if (InData.DataVersion > static_cast<int32>(CurrentCharacterSaveVersion)) {
        OutError = FString::Printf(TEXT("Data version %d is newer than current %d"),
                                   InData.DataVersion, static_cast<int32>(CurrentCharacterSaveVersion));
        return false;
    }

    if (InData.CharacterName.Len() > 64) {
        OutError = TEXT("Character name too long (max 64 chars)");
        return false;
    }

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


void UMythicSaveGameSubsystem::SaveWorld(const FString &SlotName) {
    if (SlotName.IsEmpty()) {
        UE_LOG(MythSaveLoad, Error, TEXT("SaveWorld: SlotName is empty"));
        OnSaveGameActionFinished.Broadcast(SlotName, false);
        return;
    }

    const FString SafeSlotName = SanitizeSlotName(SlotName);

    if (InFlightSaveSlots.Contains(SafeSlotName)) {
        UE_LOG(MythSaveLoad, Warning, TEXT("SaveWorld: a save to slot '%s' is already in flight; skipping concurrent write."), *SafeSlotName);
        OnSaveGameActionFinished.Broadcast(SafeSlotName, false);
        return;
    }

    OnSaveGameActionStarted.Broadcast(SafeSlotName);

    UWorld *World = GetWorld();
    if (!World) {
        OnSaveGameActionFinished.Broadcast(SafeSlotName, false);
        return;
    }

    FSerializedWorldData WorldData;

    if (AMythicGameState *GameState = World->GetGameState<AMythicGameState>()) {
        if (UMythicResourceManagerComponent *ResMgr = GameState->FindComponentByClass<UMythicResourceManagerComponent>()) {
            FSerializedDestructibleHelper::Serialize(ResMgr, WorldData.DestroyedResources);
        }
    }

    FSerializedWorldActorHelper::SerializeAll(World, WorldData.SavedActors);

    if (UMythicLivingWorldSubsystem *LWS = GetGameInstance()->GetSubsystem<UMythicLivingWorldSubsystem>()) {
        FMemoryWriter LWWriter(WorldData.LivingWorldBlob);
        LWS->SaveLivingWorld(LWWriter);
    }

    if (const UMythicPOIDiscoverySubsystem *POI = GetGameInstance()->GetSubsystem<UMythicPOIDiscoverySubsystem>()) {
        TArray<TPair<int32, FMythicPOIRegistryEntry>> Entries;
        POI->GetUnlockedPOIsForSave(Entries);
        WorldData.UnlockedPOIs.Reset();
        WorldData.UnlockedPOIs.Reserve(Entries.Num());
        for (const TPair<int32, FMythicPOIRegistryEntry> &Pair : Entries) {
            FSerializedPOIUnlock Row;
            Row.POIId = Pair.Key;
            Row.Anchor = Pair.Value.Anchor;
            Row.POITag = Pair.Value.Tag;
            Row.DisplayName = Pair.Value.Name;
            Row.Radius = Pair.Value.Radius;
            WorldData.UnlockedPOIs.Add(MoveTemp(Row));
        }
    }
    if (const UMythicDiggingSubsystem *Digging = GetGameInstance()->GetSubsystem<UMythicDiggingSubsystem>()) {
        Digging->GetConsumedSiteIds(WorldData.ConsumedDigSiteIds);
    }
    if (const UWorld *FlagWorld = GetWorld()) {
        if (const UMythicWorldStateSubsystem *WorldState = FlagWorld->GetSubsystem<UMythicWorldStateSubsystem>()) {
            WorldData.WorldFlags = WorldState->GetWorldFlags();
        }
    }

    UMythicSaveGame *SaveObj = Cast<UMythicSaveGame>(UGameplayStatics::CreateSaveGameObject(UMythicSaveGame::StaticClass()));
    if (!SaveObj) {
        OnSaveGameActionFinished.Broadcast(SafeSlotName, false);
        return;
    }

    SaveObj->WorldData = WorldData;
    SaveObj->SaveSlotName = SafeSlotName;
    SaveObj->CreationTime = FDateTime::Now();

    TArray<uint8> TempBuffer;
    FMemoryWriter MemWriter(TempBuffer);
    FObjectAndNameAsStringProxyArchive Ar(MemWriter, false);
    SaveObj->Serialize(Ar);
    SaveObj->DataChecksum = ComputeChecksum(TempBuffer);

    FAsyncSaveGameToSlotDelegate SavedDelegate;
    SavedDelegate.BindUObject(this, &UMythicSaveGameSubsystem::HandleAsyncSaveFinished);

    InFlightSaveSlots.Add(SafeSlotName);
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

    UWorld *World = GetWorld();
    if (World) {
        const FSerializedWorldData &Data = SaveObj->WorldData;

        if (AMythicGameState *GameState = World->GetGameState<AMythicGameState>()) {
            if (UMythicResourceManagerComponent *ResMgr = GameState->FindComponentByClass<UMythicResourceManagerComponent>()) {
                FSerializedDestructibleHelper::Deserialize(ResMgr, Data.DestroyedResources);
            }
        }

        FSerializedWorldActorHelper::DeserializeAll(World, Data.SavedActors);

        if (Data.LivingWorldBlob.Num() > 0) {
            if (UMythicLivingWorldSubsystem *LWS = GetGameInstance()->GetSubsystem<UMythicLivingWorldSubsystem>()) {
                FMemoryReader LWReader(Data.LivingWorldBlob, true);
                LWS->LoadLivingWorld(LWReader);
            }
        }

        if (Data.UnlockedPOIs.Num() > 0) {
            if (UMythicPOIDiscoverySubsystem *POI = GetGameInstance()->GetSubsystem<UMythicPOIDiscoverySubsystem>()) {
                for (const FSerializedPOIUnlock &Row : Data.UnlockedPOIs) {
                    POI->ServerUnlockPOI(Row.POIId, Row.Anchor, Row.POITag, Row.DisplayName, Row.Radius);
                }
            }
        }
        if (Data.ConsumedDigSiteIds.Num() > 0) {
            if (UMythicDiggingSubsystem *Digging = GetGameInstance()->GetSubsystem<UMythicDiggingSubsystem>()) {
                Digging->LoadConsumedSiteIds(Data.ConsumedDigSiteIds);
            }
        }
        if (!Data.WorldFlags.IsEmpty()) {
            if (UMythicWorldStateSubsystem *WorldState = World->GetSubsystem<UMythicWorldStateSubsystem>()) {
                for (const FGameplayTag &Flag : Data.WorldFlags) {
                    WorldState->ServerSetFlag(Flag);
                }
            }
        }
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


TArray<FMythicCharacterMetadata> UMythicSaveGameSubsystem::GetCharacterList() {
    TArray<FMythicCharacterMetadata> Result;

    if (UMythicSaveGameManifest *Manifest = Cast<UMythicSaveGameManifest>(UGameplayStatics::LoadGameFromSlot(TEXT("MythicCharacterManifest"), 0))) {
        Manifest->CharacterSlots.GenerateValueArray(Result);
    }

    Result.Sort([](const FMythicCharacterMetadata &A, const FMythicCharacterMetadata &B) {
        return A.LastPlayed > B.LastPlayed;
    });

    return Result;
}

FString UMythicSaveGameSubsystem::CreateNewCharacter(const FString &DisplayName, const FString &ClassName, bool bHardcore, const FString &ExplicitCharacterID) {
    const FString NewSlotName = ExplicitCharacterID.IsEmpty() ? FGuid::NewGuid().ToString() : SanitizeSlotName(ExplicitCharacterID);

    FMythicCharacterMetadata NewChar;
    NewChar.CharacterID = NewSlotName;
    NewChar.DisplayName = DisplayName;
    NewChar.ClassName = ClassName;
    NewChar.bIsHardcore = bHardcore;
    NewChar.Level = 1;
    NewChar.LastPlayed = FDateTime::Now();

    UMythicSaveGame *NewSave = Cast<UMythicSaveGame>(UGameplayStatics::CreateSaveGameObject(UMythicSaveGame::StaticClass()));
    if (NewSave) {
        NewSave->SaveSlotName = NewSlotName;
        NewSave->CreationTime = NewChar.LastPlayed;
        NewSave->CharacterData.CharacterID = NewSlotName;
        NewSave->CharacterData.CharacterName = DisplayName;
        NewSave->CharacterData.DataVersion = static_cast<int32>(CurrentCharacterSaveVersion);

        TArray<uint8> TempBuffer;
        FMemoryWriter MemWriter(TempBuffer);
        FObjectAndNameAsStringProxyArchive Ar(MemWriter, false);
        NewSave->Serialize(Ar);
        NewSave->DataChecksum = ComputeChecksum(TempBuffer);

        UGameplayStatics::SaveGameToSlot(NewSave, NewSlotName, 0);
    }

    UpdateManifestInternal(NewChar);

    return NewSlotName;
}

bool UMythicSaveGameSubsystem::DeleteCharacter(const FString &CharacterID) {
    if (CharacterID.IsEmpty()) {
        return false;
    }

    FMythicCharacterMetadata Dummy;
    Dummy.CharacterID = CharacterID;
    UpdateManifestInternal(Dummy, true);

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

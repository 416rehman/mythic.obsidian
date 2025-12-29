#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Mythic/Subsystem/SaveSystem/Character/CharacterData.h"
#include "Mythic/Subsystem/SaveSystem/World/WorldData.h"
#include "Mythic/Subsystem/SaveSystem/MythicSaveGameManifest.h"
#include "MythicSaveGameSubsystem.generated.h"

class UMythicSaveGame;
class USaveGame;

/**
 * Centralized Save/Load Subsystem.
 * Orchestrates calls to type-specific serialization helpers.
 */
// Delegates for UI feedback
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSaveGameActionStarted, const FString&, SlotName);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSaveGameActionFinished, const FString&, SlotName, bool, bSuccess);

UCLASS()
class MYTHIC_API UMythicSaveGameSubsystem : public UGameInstanceSubsystem {
    GENERATED_BODY()

public:
    // --- Delegates ---

    UPROPERTY(BlueprintAssignable, Category = "Save System | Events")
    FOnSaveGameActionStarted OnSaveGameActionStarted;

    UPROPERTY(BlueprintAssignable, Category = "Save System | Events")
    FOnSaveGameActionFinished OnSaveGameActionFinished;

    // --- Character Save/Load (Async) ---

    // Asynchronously saves character data. Validates source actor on GameThread, then writes to disk on background thread.
    UFUNCTION(BlueprintCallable, Category = "Save System")
    void SaveCharacter(AActor *SourceActor, const FString &CharacterID);

    UFUNCTION(BlueprintCallable, Category = "Save System")
    void LoadCharacter(AActor *TargetActor, const FString &CharacterID);

    UFUNCTION(BlueprintCallable, Category = "Save System")
    TArray<FString> GetLocalSaveFiles() const;

    // --- World Save (Async) ---

    UFUNCTION(BlueprintCallable, Category = "Save System")
    void SaveWorld(const FString &SlotName);

    UFUNCTION(BlueprintCallable, Category = "Save System")
    void LoadWorld(const FString &SlotName);

    // --- Character Data Serialization (for Network Transfer) ---
    static bool SerializeCharacterToStruct(AActor *SourceActor, FSerializedCharacterData &OutData);
    static bool DeserializeCharacterFromStruct(AActor *TargetActor, const FSerializedCharacterData &InData);
    static bool ValidateCharacterData(const FSerializedCharacterData &InData, FString &OutError);

    // --- Character Manifest (Menu System) ---

    // Returns list of all available characters from the central manifest.
    UFUNCTION(BlueprintCallable, Category = "Save System | Manifest")
    TArray<FMythicCharacterMetadata> GetCharacterList();

    // Creates a new character slot and updates the manifest. Returns the generated SlotName (CharacterID).
    UFUNCTION(BlueprintCallable, Category = "Save System | Manifest")
    FString CreateNewCharacter(const FString &DisplayName, const FString &ClassName, bool bHardcore);

    // Deletes a character and their save file.
    UFUNCTION(BlueprintCallable, Category = "Save System | Manifest")
    bool DeleteCharacter(const FString &CharacterID);

private:
    // --- Internal Callbacks ---

    void HandleAsyncSaveFinished(const FString &SlotName, const int32 UserIndex, bool bSuccess);
    void HandleAsyncLoadFinished(const FString &SlotName, const int32 UserIndex, USaveGame *LoadedSaveGame);
    void HandleAsyncWorldLoadFinished(const FString &SlotName, const int32 UserIndex, USaveGame *LoadedSaveGame);

    // --- State helpers ---

    // Tracks pending load target actors to ensure valid references when async load finishes
    // Map<SlotName, WeakPointer<Actor>>
    TMap<FString, TWeakObjectPtr<AActor>> PendingLoadTargets;

    // --- Security Utilities ---

    // Sanitizes slot names to prevent path traversal attacks
    static FString SanitizeSlotName(const FString &Input);

    // Computes SHA1 checksum for data integrity
    static FString ComputeChecksum(const TArray<uint8> &Data);

    // Validates checksum against expected value
    static bool ValidateChecksum(const TArray<uint8> &Data, const FString &ExpectedChecksum);

    void FindSaveGames(TArray<FString> &OutSaveFiles) const;
    void UpdateManifestInternal(const FMythicCharacterMetadata &Metadata, bool bRemove = false);
};

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Mythic/Subsystem/SaveSystem/Character/CharacterData.h"
#include "Mythic/Subsystem/SaveSystem/World/WorldData.h"
#include "MythicSaveGameSubsystem.generated.h"

class UMythicSaveGame;

/**
 * Centralized Save/Load Subsystem.
 * Orchestrates calls to type-specific serialization helpers.
 */
UCLASS()
class MYTHIC_API UMythicSaveGameSubsystem : public UGameInstanceSubsystem {
    GENERATED_BODY()

public:
    // --- Character Save/Load (Local Files) ---

    UFUNCTION(BlueprintCallable, Category = "Save System")
    bool SaveCharacter(AActor *SourceActor, const FString &CharacterID);

    UFUNCTION(BlueprintCallable, Category = "Save System")
    bool LoadCharacter(AActor *TargetActor, const FString &CharacterID);

    UFUNCTION(BlueprintCallable, Category = "Save System")
    TArray<FString> GetLocalSaveFiles() const;

    // --- Character Data Serialization (for Network Transfer) ---

    // Serializes character data to a struct (for sending over network)
    UFUNCTION(BlueprintCallable, Category = "Save System")
    bool SerializeCharacterToStruct(AActor *SourceActor, FSerializedCharacterData &OutData);

    // Deserializes character data from a struct (received from network)
    UFUNCTION(BlueprintCallable, Category = "Save System")
    bool DeserializeCharacterFromStruct(AActor *TargetActor, const FSerializedCharacterData &InData);

    // Validates character data from a client (host should call this before applying)
    UFUNCTION(BlueprintCallable, Category = "Save System")
    bool ValidateCharacterData(const FSerializedCharacterData &InData, FString &OutError);

    // --- World Save (Host Only) ---

    UFUNCTION(BlueprintCallable, Category = "Save System")
    bool SaveWorldState(const FString &SlotName);

    UFUNCTION(BlueprintCallable, Category = "Save System")
    bool LoadWorldState(const FString &SlotName);

private:
    // --- Security Utilities ---

    // Sanitizes slot names to prevent path traversal attacks
    static FString SanitizeSlotName(const FString &Input);

    // Computes SHA1 checksum for data integrity
    static FString ComputeChecksum(const TArray<uint8> &Data);

    // Validates checksum against expected value
    static bool ValidateChecksum(const TArray<uint8> &Data, const FString &ExpectedChecksum);

    void FindSaveGames(TArray<FString> &OutSaveFiles) const;
};

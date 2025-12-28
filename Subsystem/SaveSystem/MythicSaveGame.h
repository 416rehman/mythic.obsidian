#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Character/CharacterData.h"
#include "World/WorldData.h"
#include "MythicSaveGame.generated.h"

/**
 * UMythicSaveGame
 * 
 * The root object serialized to disk.
 * Can hold either a Character Save (Client) or World Save (Host).
 */
UCLASS()
class MYTHIC_API UMythicSaveGame : public USaveGame {
    GENERATED_BODY()

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Save Data")
    FString SaveSlotName;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Save Data")
    FDateTime CreationTime;

    // SHA256 checksum of the serialized data for integrity validation
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save Data")
    FString DataChecksum;

    // --- Data Sections ---

    // If this is a Character Save, this will be populated.
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Save Data")
    FSerializedCharacterData CharacterData;

    // If this is a World Save (Host), this will be populated.
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Save Data")
    FSerializedWorldData WorldData;

    // --- Helper Methods ---

    // Checks definitions to ensure data is valid for the current game version
    void FixupData();
};

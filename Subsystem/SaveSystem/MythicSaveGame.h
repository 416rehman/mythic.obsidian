#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Character/CharacterData.h"
#include "World/WorldData.h"
#include "MythicSaveGame.generated.h"

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


    // If this is a Character Save, this will be populated.
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Save Data")
    FSerializedCharacterData CharacterData;

    // If this is a World Save (Host), this will be populated.
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Save Data")
    FSerializedWorldData WorldData;


    void FixupData();
};

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "MythicSaveGameManifest.generated.h"

/**
 * Metadata for a character slot, allowing UI to display info without loading the full save.
 */
USTRUCT(BlueprintType)
struct FMythicCharacterMetadata {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Save System")
    FString CharacterID; // The Slot Name (e.g. GUID)

    UPROPERTY(BlueprintReadOnly, Category = "Save System")
    FString DisplayName; // User-defined name

    UPROPERTY(BlueprintReadOnly, Category = "Save System")
    FString ClassName; // e.g. "Warrior"

    UPROPERTY(BlueprintReadOnly, Category = "Save System")
    int32 Level = 1;

    UPROPERTY(BlueprintReadOnly, Category = "Save System")
    bool bIsHardcore = false;

    UPROPERTY(BlueprintReadOnly, Category = "Save System")
    bool bIsDead = false;

    UPROPERTY(BlueprintReadOnly, Category = "Save System")
    FDateTime LastPlayed;

    FMythicCharacterMetadata()
        : LastPlayed(FDateTime::MinValue()) {}
};

/**
 * Lightweight SaveGame object that only stores the list of characters.
 * Saved to a fixed slot "MythicCharacterManifest".
 */
UCLASS()
class MYTHIC_API UMythicSaveGameManifest : public USaveGame {
    GENERATED_BODY()

public:
    UPROPERTY()
    TMap<FString, FMythicCharacterMetadata> CharacterSlots;
};

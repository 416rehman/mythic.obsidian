#pragma once

#include "CoreMinimal.h"
#include "SavedInventory.h"
#include "SavedProficiency.h"
#include "CharacterData.generated.h"

UENUM(BlueprintType)
enum class EMythicCharacterSaveVersion : uint8 {
    InitialVersion = 0,
    LatestVersion,
    VersionPlusOne
};

constexpr EMythicCharacterSaveVersion CurrentCharacterSaveVersion = EMythicCharacterSaveVersion::LatestVersion;

/**
 * The Universal Character Data Packet.
 * 
 * Design: Derived State Architecture
 * - Attributes are NOT saved directly
 * - Attributes are derived from proficiencies (rewards) + items (stats) on load
 */
USTRUCT(BlueprintType)
struct FSerializedCharacterData {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    int32 DataVersion = static_cast<int32>(EMythicCharacterSaveVersion::InitialVersion);

    UPROPERTY(BlueprintReadWrite)
    FString CharacterID;

    UPROPERTY(BlueprintReadWrite)
    FString CharacterName;

    UPROPERTY(BlueprintReadWrite)
    TArray<FSerializedInventoryData> Inventories;

    UPROPERTY(BlueprintReadWrite)
    TArray<FSerializedProficiencyData> Proficiencies;

    // Note: Attributes are NOT saved - they are derived from proficiencies + items on load

    // Last world transform of the player's pawn, so a reload restores position/rotation instead of respawning at
    // the default PlayerStart. Gated by bHasSavedTransform so saves written before this field existed (which would
    // deserialize to Identity) don't teleport the player to the world origin.
    UPROPERTY(BlueprintReadWrite)
    FTransform SavedTransform = FTransform::Identity;

    UPROPERTY(BlueprintReadWrite)
    bool bHasSavedTransform = false;

    UPROPERTY(BlueprintReadWrite)
    TMap<FString, FString> CustomData;

    /** Serializes an actor into this character data struct */
    static bool Serialize(AActor *SourceActor, FSerializedCharacterData &OutData);

    /** Applies this character data struct to an actor */
    static bool Deserialize(AActor *TargetActor, const FSerializedCharacterData &InData);
};

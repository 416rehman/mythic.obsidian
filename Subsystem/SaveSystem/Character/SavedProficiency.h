#pragma once

#include "CoreMinimal.h"
#include "SavedProficiency.generated.h"

class UProficiencyComponent;

USTRUCT(BlueprintType)
struct FSerializedProficiencyData {
    GENERATED_BODY()

    // Reference to the proficiency definition asset
    UPROPERTY(BlueprintReadWrite)
    FSoftObjectPath ProficiencyAsset;

    // String-based storage for FGameplayAttribute (pointers don't serialize)
    UPROPERTY(BlueprintReadWrite)
    FString ProgressAttributeSetClass;

    UPROPERTY(BlueprintReadWrite)
    FString ProgressAttributeName;

    // Current XP value - ClaimedLevels is derived from this on load
    UPROPERTY(BlueprintReadWrite)
    float CurrentXP = 0.0f;
};

struct FSerializedProficiencyHelper {
    static void Serialize(UProficiencyComponent *Component, TArray<FSerializedProficiencyData> &OutData);
    static void Deserialize(UProficiencyComponent *Component, const TArray<FSerializedProficiencyData> &InData);
};

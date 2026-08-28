#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPath.h"
#include "SavedProficiency.generated.h"

class UProficiencyComponent;
class UProficiencyDefinition;

/** Persistent proficiency row storing a typed definition reference and its canonical cumulative XP. */
USTRUCT(BlueprintType)
struct FSerializedProficiencyData {
    GENERATED_BODY()

    /** Typed proficiency identity; the Progress Stat and its GAS attributes are derived from this definition. */
    UPROPERTY(BlueprintReadWrite)
    TSoftObjectPtr<UProficiencyDefinition> ProficiencyDefinition;

    /** Canonical cumulative XP; level and claimed rewards are deterministically derived on load. */
    UPROPERTY(BlueprintReadWrite)
    float CurrentXP = 0.0f;
};

struct FSerializedProficiencyHelper {
    static bool Serialize(UProficiencyComponent *Component, TArray<FSerializedProficiencyData> &OutData);
    static bool Deserialize(UProficiencyComponent *Component, const TArray<FSerializedProficiencyData> &InData);
};

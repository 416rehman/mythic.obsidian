#pragma once

#include "CoreMinimal.h"
#include "Mythic/World/LivingWorld/LivingWorldTypes.h"
#include "SavedFactionStanding.generated.h"

USTRUCT(BlueprintType)
struct FSerializedFactionStandingData {
    GENERATED_BODY()

    // Index into the faction database. Defaults to the invalid sentinel.
    UPROPERTY(BlueprintReadWrite)
    uint8 FactionIndex = FMythicFactionId::InvalidIndex;

    // This player's standing toward that faction (negative = disliked, positive = liked; 0 = neutral, never stored).
    UPROPERTY(BlueprintReadWrite)
    float Value = 0.0f;
};

struct FSerializedFactionStandingHelper {
    static bool ShouldPersist(uint8 FactionIndex, float Value) {
        return FactionIndex != FMythicFactionId::InvalidIndex && Value != 0.0f;
    }
};

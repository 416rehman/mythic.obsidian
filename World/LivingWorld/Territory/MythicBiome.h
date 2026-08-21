
#pragma once

#include "CoreMinimal.h"
#include "MythicBiome.generated.h"


UENUM(BlueprintType)
enum class EMythicBiome : uint8 {
    Plains = 0 UMETA(DisplayName = "Plains"),
    Forest UMETA(DisplayName = "Forest"),
    Mountain UMETA(DisplayName = "Mountain"),
    Wetland UMETA(DisplayName = "Wetland"),
    Wasteland UMETA(DisplayName = "Wasteland"),
    Desert UMETA(DisplayName = "Desert"),
    COUNT UMETA(Hidden)
};

static constexpr int32 MythicBiomeCount = static_cast<int32>(EMythicBiome::COUNT);


USTRUCT(BlueprintType)
struct MYTHIC_API FMythicBiomeThresholds {
    GENERATED_BODY()

    /** Elevation above which a cell is Mountain. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MountainElevation = 0.72f;

    /** Moisture above which a cell is Wetland. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float WetlandMoisture = 0.70f;

    /** Moisture above which a (non-wetland) cell is Forest. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ForestMoisture = 0.45f;

    /** Moisture below which a cell is arid (Wasteland, or Desert if also elevated). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float WastelandMoisture = 0.18f;

    /** Elevation a dry cell must exceed (while below MountainElevation) to be Desert rather than Wasteland. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float DesertElevation = 0.55f;
};

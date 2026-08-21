
#pragma once

#include "CoreMinimal.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "MythicDanger.generated.h"


UENUM(BlueprintType)
enum class EMythicDangerTier : uint8 {
    Safe = 0 UMETA(DisplayName = "Safe"),
    Low UMETA(DisplayName = "Low"),
    Moderate UMETA(DisplayName = "Moderate"),
    High UMETA(DisplayName = "High"),
    Extreme UMETA(DisplayName = "Extreme"),
    COUNT UMETA(Hidden)
};


USTRUCT(BlueprintType)
struct MYTHIC_API FMythicDangerTierParams {
    GENERATED_BODY()

    /** Distance (cells) from the safe core at which a cell first reaches Low danger. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Danger", meta = (ClampMin = "0"))
    int32 LowTierDistance = 3;

    /** Distance (cells) at which a cell reaches Moderate danger. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Danger", meta = (ClampMin = "0"))
    int32 ModerateTierDistance = 8;

    /** Distance (cells) at which a cell reaches High danger. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Danger", meta = (ClampMin = "0"))
    int32 HighTierDistance = 15;

    /** Distance (cells) at which a cell reaches Extreme danger. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Danger", meta = (ClampMin = "0"))
    int32 ExtremeTierDistance = 25;

    /** Extra tier steps added by a fully-contested cell (MilitaryStrength 1.0). 2.0 => a max-strength cell jumps two
     *  tiers above its distance-derived base (rounded). 0 disables the strength contribution. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Danger", meta = (ClampMin = "0.0"))
    float StrengthTierBoost = 2.0f;
};


struct MYTHIC_API FMythicDanger {
    static int32 ChebyshevDistance(const FMythicCellCoord& A, const FMythicCellCoord& B);

    static EMythicDangerTier ComputeDangerTier(int32 DistanceCells, float MilitaryStrength, bool bHasCapital,
                                               const FMythicDangerTierParams& Params);
};

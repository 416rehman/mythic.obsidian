#pragma once

#include "CoreMinimal.h"
#include "Containers/ArrayView.h"
#include "GameplayTagContainer.h"
#include "MythicHealthBands.generated.h"

/**
 * A named slice of the health bar. While an actor sits inside one, it carries the band's tag, which is what lets a
 * gameplay effect gate a modifier on "the target is nearly dead" without any code knowing what nearly dead means.
 *
 * Bands may overlap: an actor at 10% is inside both a 0..0.5 band and a 0..0.2 one, and carries both tags.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicHealthBand {
    GENERATED_BODY()

    // Tag carried while inside the band. Invalid = the row is inert.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health Bands", meta = (Categories = "GAS.State.Health"))
    FGameplayTag Tag;

    // Health fraction the band covers, both ends inclusive. 0..0.5 is "at or below half"; 0.9..1 is "barely scratched".
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health Bands", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MinFraction = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health Bands", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MaxFraction = 1.0f;
};

USTRUCT(BlueprintType)
struct MYTHIC_API FMythicHealthBandConfig {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health Bands")
    TArray<FMythicHealthBand> Bands;

    bool IsConfigured() const { return Bands.Num() > 0; }
};

struct FMythicHealthBandRules {
    static bool BandContains(const FMythicHealthBand &Band, float Fraction) {
        if (!Band.Tag.IsValid()) {
            return false;
        }
        // An inverted row would otherwise match nothing and read as a dead band rather than an authoring slip.
        const float Low = FMath::Min(Band.MinFraction, Band.MaxFraction);
        const float High = FMath::Max(Band.MinFraction, Band.MaxFraction);
        return Fraction >= Low && Fraction <= High;
    }

    static void ResolveBands(TConstArrayView<FMythicHealthBand> Bands, float Fraction, FGameplayTagContainer &OutActive) {
        OutActive.Reset();
        const float Clamped = FMath::Clamp(Fraction, 0.0f, 1.0f);
        for (const FMythicHealthBand &Band : Bands) {
            if (BandContains(Band, Clamped)) {
                OutActive.AddTag(Band.Tag);
            }
        }
    }

    // Health as a 0..1 fraction. Anything with no maximum reads as unhurt, matching UMythicGA_Triggered::GetHealthFraction.
    static float FractionOf(float Health, float MaxHealth) {
        if (MaxHealth <= 0.0f) {
            return 1.0f;
        }
        return FMath::Clamp(Health / MaxHealth, 0.0f, 1.0f);
    }
};


#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "MythicEconomyPricing.generated.h"

UENUM(BlueprintType)
enum class EMythicEconomyAxis : uint8 {
    None UMETA(DisplayName = "None"),
    Food UMETA(DisplayName = "Food"),
    Materials UMETA(DisplayName = "Materials"),
    Arms UMETA(DisplayName = "Arms"),
    Wealth UMETA(DisplayName = "Wealth")
};

USTRUCT(BlueprintType)
struct FMythicEconomyPricingParams {
    GENERATED_BODY()

    // How strongly economy scarcity moves the price. 0 ⇒ inert (multiplier == 1.0, byte-identical to static pricing).
    // A small positive value (e.g. 0.1) gives gentle, realistic swings; larger values react harder to deficits.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0"))
    float Elasticity = 0.0f;

    // Lower clamp on the multiplier — the deepest discount a surplus can drive (0.75 ⇒ never below 75% of base price).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0"))
    float MinBand = 0.75f;

    // Upper clamp on the multiplier — the highest surcharge a famine can drive (1.5 ⇒ never above 150% of base price).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0"))
    float MaxBand = 1.5f;

    // The "healthy" reserve level this axis is measured against. Reserves below it read as scarcity, above it as surplus;
    // it also normalizes the per-tick demand term. Guarded to >= 1 inside the formula so it never divides by zero.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1.0"))
    float ReserveReference = 100.0f;
};

struct FMythicEconomyPricing {
    static float ComputeScarcityMultiplier(float AxisReserves, float AxisDemand, float AxisPrice, EMythicEconomyAxis Axis,
                                           const FMythicEconomyPricingParams &Params) {
        if (Axis == EMythicEconomyAxis::None || Params.Elasticity <= 0.0f) {
            return 1.0f;
        }
        if (AxisDemand <= 0.0f && AxisPrice <= 0.0f) {
            return 1.0f;
        }
        const float Ref = FMath::Max(Params.ReserveReference, 1.0f);
        const float PriceSignal = AxisPrice - 1.0f;
        const float ReserveSignal = (Ref - AxisReserves) / Ref;
        const float DemandSignal = FMath::Max(AxisDemand, 0.0f) / Ref;
        const float Scarcity = PriceSignal + ReserveSignal + DemandSignal;
        const float Mult = 1.0f + Params.Elasticity * Scarcity;
        return FMath::Clamp(Mult, Params.MinBand, Params.MaxBand);
    }

    static float ApplyLocalSellPressure(float BaseMultiplier, float RecentPlayerSellUnits, float PressurePerUnit, float FloorMult) {
        if (PressurePerUnit <= 0.0f || RecentPlayerSellUnits <= 0.0f || BaseMultiplier <= FloorMult) {
            return BaseMultiplier;
        }
        const float DecayFactor = FMath::Exp(-PressurePerUnit * RecentPlayerSellUnits);
        return FloorMult + (BaseMultiplier - FloorMult) * DecayFactor;
    }

    static EMythicEconomyAxis AxisForItem(const FGameplayTagContainer &ItemTypeTags, const TMap<FGameplayTag, EMythicEconomyAxis> &Map) {
        if (ItemTypeTags.IsEmpty() || Map.Num() == 0) {
            return EMythicEconomyAxis::None;
        }
        for (const FGameplayTag &Tag : ItemTypeTags) {
            if (const EMythicEconomyAxis *Found = Map.Find(Tag)) {
                return *Found;
            }
        }
        for (const TPair<FGameplayTag, EMythicEconomyAxis> &Pair : Map) {
            if (ItemTypeTags.HasTag(Pair.Key)) {
                return Pair.Value;
            }
        }
        return EMythicEconomyAxis::None;
    }
};

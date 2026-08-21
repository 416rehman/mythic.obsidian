
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Containers/ArrayView.h"
#include "MythicWeatherCombatRules.generated.h"

USTRUCT(BlueprintType)
struct FMythicWeatherDamageMod {
    GENERATED_BODY()

    /** Weather this row is live under. The CURRENT weather matches when it equals this tag or is a child of it
     *  (author Environment.Weather.Rain for rain only, or Environment.Weather to hit every weather). Invalid = row inert. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather Combat", meta = (Categories = "Environment.Weather"))
    FGameplayTag WeatherTag;

    /** Damage-type/element tag the HIT must carry (matched against the hit's context tags: aggregated source tags +
     *  status-intent GAS.Debuff.* tags — see file header). EMPTY = matches ANY hit in this weather (a weather-wide mod). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather Combat")
    FGameplayTag DamageTypeTag;

    /** Damage multiplier applied when this row matches (0.75 = -25%, 1.25 = +25%). Matching rows MULTIPLY together.
     *  Clamped non-negative at resolve time. 1.0 = no damage change (a row can exist only for its AddStatusTag). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather Combat", meta = (ClampMin = "0.0"))
    float Multiplier = 1.0f;

    /** OPTIONAL bonus status buildup: when this row matches, the execution feeds one extra buildup increment of this
     *  status (GAS.Debuff.*) into the target — e.g. shock hits in rain add bonus Slowed buildup. Empty = no bonus. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather Combat", meta = (Categories = "GAS.Debuff"))
    FGameplayTag AddStatusTag;
};

USTRUCT(BlueprintType)
struct FMythicWeatherCombatConfig {
    GENERATED_BODY()

    /** The authored weather→combat rows. Empty = inert (weather never touches damage). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather Combat")
    TArray<FMythicWeatherDamageMod> Mods;

    bool IsConfigured() const { return Mods.Num() > 0; }
};

struct FMythicWeatherCombatRules {
    static bool RowMatches(const FMythicWeatherDamageMod &Row, const FGameplayTag &Weather,
                           const FGameplayTagContainer &HitDamageTags) {
        if (!Row.WeatherTag.IsValid() || !Weather.IsValid()) {
            return false;
        }
        if (!Weather.MatchesTag(Row.WeatherTag)) {
            return false;
        }
        if (Row.DamageTypeTag.IsValid() && !HitDamageTags.HasTag(Row.DamageTypeTag)) {
            return false;
        }
        return true;
    }

    static float ResolveWeatherMultiplier(TConstArrayView<FMythicWeatherDamageMod> Mods, const FGameplayTag &Weather,
                                          const FGameplayTagContainer &HitDamageTags) {
        float Result = 1.0f;
        for (const FMythicWeatherDamageMod &Row : Mods) {
            if (RowMatches(Row, Weather, HitDamageTags)) {
                Result *= FMath::Max(0.0f, Row.Multiplier);
            }
        }
        return Result;
    }

    static FGameplayTag ResolveWeatherStatusBonus(TConstArrayView<FMythicWeatherDamageMod> Mods, const FGameplayTag &Weather,
                                                  const FGameplayTagContainer &HitDamageTags) {
        for (const FMythicWeatherDamageMod &Row : Mods) {
            if (Row.AddStatusTag.IsValid() && RowMatches(Row, Weather, HitDamageTags)) {
                return Row.AddStatusTag;
            }
        }
        return FGameplayTag();
    }
};

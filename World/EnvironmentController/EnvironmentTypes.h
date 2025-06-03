#pragma once

#include "GameplayTagContainer.h"
#include "Mythic.h"
#include "Materials/MaterialParameterCollection.h"
#include "EnvironmentTypes.generated.h"

UENUM(BlueprintType)
enum EDayTime {
    // 06:00 - 12:00
    Morning,
    // 12:00 - 17:00
    Afternoon,
    // 17:00 - 20:00
    Evening,
    // 20:00 - 06:00
    Night
};

UENUM(BlueprintType)
enum ESeason {
    // Months: 3, 4, 5
    Spring,
    // Months: 6, 7, 8
    Summer,
    // Months: 9, 10, 11
    Autumn,
    // Months: 12, 1, 2
    Winter,
};

USTRUCT(BlueprintType)
struct FScalarWeatherAttribute {
    GENERATED_BODY()

    // The name of the attribute in the Material Parameter Collection
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName Name;

    // The minimum value of the attribute
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MinValue = 0.0f;

    // The maximum value of the attribute
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaxValue = 1.0f;

    // Return a random value between MinValue and MaxValue
    FCollectionScalarParameter ComputeScalarParameter() const {
        auto Value = FCollectionScalarParameter();
        Value.DefaultValue = FMath::Lerp(MinValue, MaxValue, FMath::FRand());
        Value.ParameterName = Name;

        return Value;
    }
};

USTRUCT(BlueprintType)
struct FComputedVectorParam {
    GENERATED_BODY()

    // The name of the attribute in the Material Parameter Collection
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName Name;

    // The value of the attribute
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector Value = FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct FVectorWeatherAttribute {
    GENERATED_BODY()

    // The name of the attribute in the Material Parameter Collection
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName Name;

    // The minimum value of the attribute
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLinearColor MinValue = FLinearColor::Black;

    // The maximum value of the attribute
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLinearColor MaxValue = FLinearColor::White;

    // Return a random value between MinValue and MaxValue
    FCollectionVectorParameter ComputeVectorParameter() const {
        auto Value = FCollectionVectorParameter();
        Value.DefaultValue = FLinearColor(FMath::Lerp(MinValue.R, MaxValue.R, FMath::FRand()),
                                          FMath::Lerp(MinValue.G, MaxValue.G, FMath::FRand()),
                                          FMath::Lerp(MinValue.B, MaxValue.B, FMath::FRand()),
                                          FMath::Lerp(MinValue.A, MaxValue.A, FMath::FRand())
            );
        Value.ParameterName = Name;

        return Value;
    }
};

// Struct to define the characteristics of a weather type
UCLASS(BlueprintType, Blueprintable, EditInlineNew)
class MYTHIC_API UWeatherType : public UDataAsset {
    GENERATED_BODY()

public:
    // Tag of this weather type
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta=(Categories="Environment.Weather"))
    FGameplayTag Tag;

    // Optional pre-weather type required before this weather
    // Can only transition to this weather, if the current weather is the pre-weather
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Transition")
    FGameplayTag OnlyAllowTransitionFromWeather;

    // Optional post-weather type required after this weather
    // Can only transition out of this weather to the post-weather
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Transition")
    FGameplayTag OnlyAllowTransitionToWeather;

    // The month range this weather type can occur in
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Time", meta=(UIMin="1", UIMax="12"))
    FInt32Interval MonthRange = FInt32Interval(1, 12);

    // The minimum time this weather type can last
    // No effect if used as a pre-weather, since the duration of this weather type will fit within the transition time to the target weather.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Time")
    float MinDurationInMinutes = 25.0f;

    // The maximum time this weather type can last
    // No effect if used as a pre-weather, since the duration of this weather type will fit within the transition time to the target weather.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Time")
    float MaxDurationInMinutes = 60.0f;

    // The scalar attributes that can be modified by this weather
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    TArray<FScalarWeatherAttribute> ScalarAttributes;

    // The vector attributes that can be modified by this weather
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    TArray<FVectorWeatherAttribute> VectorAttributes;

    // Fog Density - The type is a vec2
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FVector2D FogDensity = FVector2D(0.02, 0.02);

    // Fog Height Falloff - vec2
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FVector2D FogHeightFalloff = FVector2D(0.2, 0.2);

    // Check if this weather type can transition to the target weather
    bool CanTransitionTo(const UWeatherType &TargetWeather) const {
        // If we have a Post-Weather, we can only transition to the target weather if the target weather is the Post-Weather
        if (OnlyAllowTransitionToWeather.IsValid()) {
            return TargetWeather.Tag == OnlyAllowTransitionToWeather;
        }

        // If the target weather has a Pre-Weather, we can only transition to the target weather if the current weather is the Pre-Weather
        if (TargetWeather.OnlyAllowTransitionFromWeather.IsValid()) {
            return Tag == TargetWeather.OnlyAllowTransitionFromWeather;
        }

        // If the target weather has no Pre-Weather, we can transition to the target weather
        return true;
    }
};

// Struct to define the characteristics of a weather state that is calculated by the server and replicated to clients
USTRUCT(BlueprintType)
struct FWeatherCycleInfo {
    GENERATED_BODY()

    // If this is set, a transition is in progress to this weather type
    // When this is set, compute the TransitioningFromWeatherAttributes and TransitioningToWeatherAttributes
    // When the transition is complete, set CurrentWeather to TransitionToWeather, and clear TransitionToWeather
    UPROPERTY(BlueprintReadOnly, Category = "Time of Day Controller | Weather")
    UWeatherType *TransitionToWeather = nullptr;

    // Stores the transition values, this is used by the current transition to interpolate between the current weather and the target weather
    // Calculated by the server, replicated to clients
    UPROPERTY()
    TArray<FCollectionScalarParameter> TransitionToScalarValues = TArray<FCollectionScalarParameter>();

    // Stores the transition values, this is used by the current transition to interpolate between the current weather and the target weather
    // Calculated by the server, replicated to clients
    UPROPERTY()
    TArray<FCollectionVectorParameter> TransitionToVectorValues = TArray<FCollectionVectorParameter>();

    // This is how long the weather will last before another transition is triggered
    UPROPERTY()
    float TransitionLength = 15.0f;

    // The fog density of the current weather
    UPROPERTY()
    float FogDensity = 0.02f;

    // The fog height falloff of the current weather
    UPROPERTY()
    float FogHeightFalloff = 0.2f;

    // Set Instantly
    UPROPERTY()
    bool bSetInstantly = false;

    FWeatherCycleInfo(){};

    // Constructor from a weather type
    FWeatherCycleInfo(UWeatherType *SelectedWeather, bool SetInstantly = false) {
        /// /////////////////////////////////
        /// COMPUTE THE NEW WEATHER CYCLE
        /// /////////////////////////////////
        this->TransitionLength = FMath::RandRange(SelectedWeather->MinDurationInMinutes, SelectedWeather->MaxDurationInMinutes);
        this->TransitionToWeather = SelectedWeather;

        this->TransitionToScalarValues.Empty();
        this->TransitionToVectorValues.Empty();

        // Update the TransitionToScalarValues
        for (const auto &ScalarAttribute : SelectedWeather->ScalarAttributes) {
            this->TransitionToScalarValues.Add(ScalarAttribute.ComputeScalarParameter());
        }

        // Update the TransitionToVectorValues
        for (const auto &VectorAttribute : SelectedWeather->VectorAttributes) {
            this->TransitionToVectorValues.Add(VectorAttribute.ComputeVectorParameter());
        }

        this->FogHeightFalloff = FMath::RandRange(SelectedWeather->FogHeightFalloff.X, SelectedWeather->FogHeightFalloff.Y);
        this->FogDensity = FMath::RandRange(SelectedWeather->FogDensity.X, SelectedWeather->FogDensity.Y);

        // Print the computed weather cycle values
        UE_LOG(Mythic_Environment, Warning, TEXT("Computed Fog Density: %f"), this->FogDensity);
        UE_LOG(Mythic_Environment, Warning, TEXT("Computed Fog Height Falloff: %f"), this->FogHeightFalloff);

        bSetInstantly = SetInstantly;
    }
};

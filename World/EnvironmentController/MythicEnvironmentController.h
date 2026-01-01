// 

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentTypes.h"
#include "Mythic.h"
#include "Engine/DirectionalLight.h"
#include "GameFramework/Actor.h"
#include "Mythic/Subsystem/SaveSystem/World/MythicSaveableActor.h"
#include "MythicEnvironmentController.generated.h"

class UExponentialHeightFogComponent;
class USkyAtmosphereComponent;
class ASkyAtmosphere;
class AExponentialHeightFog;

// --- HELPERS ---------------------------------------------------------------
// Returns the current time of day as a float from 0-24
// 1-30
static int GetDayOfMonth(FTimespan timespan) {
    auto day = timespan.GetDays() % 30;
    return day == 0 ? 30 : day;
}

static int GetYear(FTimespan timespan) {
    return timespan.GetDays() / 360 + 1;
}

// Returns the current month of the year as an int from 1-12
static int GetMonthOfYear(FTimespan timespan) {
    auto month = timespan.GetDays() % 360 / 30;
    return month == 0 ? 12 : month;
}

static FDateTime AsDateTime(FTimespan timespan) {
    // Returns the current date and time
    auto year = GetYear(timespan);
    auto month = GetMonthOfYear(timespan);
    auto day = GetDayOfMonth(timespan);
    auto hour = timespan.GetHours();
    auto minute = timespan.GetMinutes();
    auto second = timespan.GetSeconds();

    return FDateTime(year, month, day, hour, minute, second);
}

static ESeason MonthAsSeason(int32 Month) {
    // Spring: March, April, May
    if (Month == 3 || Month == 4 || Month == 5) {
        return ESeason::Spring;
    }

    // Summer: June, July, August
    if (Month == 6 || Month == 7 || Month == 8) {
        return ESeason::Summer;
    }

    // Autumn: September, October, November
    if (Month == 9 || Month == 10 || Month == 11) {
        return ESeason::Autumn;
    }

    if (Month < 1 || Month > 12) {
        UE_LOG(Myth, Error, TEXT("Invalid month - %d"), Month);
    }

    // Winter: December, January, February
    return ESeason::Winter;
}

// Returns the DayTime based on the current hour
// Morning: 7-11:59
// Afternoon: 12-16:59
// Evening: 17-19:59
// Night: 20-6:59
static EDayTime HourAsDayTime(uint8 CurrentHour) {
    if (CurrentHour >= 7 && CurrentHour < 12) {
        return EDayTime::Morning;
    }

    if (CurrentHour >= 12 && CurrentHour < 17) {
        return EDayTime::Afternoon;
    }

    if (CurrentHour >= 17 && CurrentHour < 20) {
        return EDayTime::Evening;
    }

    return EDayTime::Night;
}

// Delegate signature
// DayTime change - Morning, Afternoon, Evening, Night
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDayTimeChangedSignature, EDayTime, PrevDayTime, EDayTime, NewDayTime);

// Hour change - 0-23
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHourChangedSignature, int32, PrevHour, int32, NewHour);

// Day change - 1-30
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDayChangedSignature, int32, PrevDay, int32, NewDay);

// Month change - 1-12 with season
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnMonthChangedSignature, int32, PrevMonth, int32, NewMonth, ESeason, PrevSeason, ESeason, NewSeason);

// Year change - 1 - Infinite
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnYearChangedSignature, int32, PrevYear, int32, NewYear);

// WeatherChangeStart - FromWeather, ToWeather, TransitionDuration
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnWeatherChangeStartSignature, FGameplayTag, FromWeather, FGameplayTag, ToWeather, float, TransitionLength);

// WeatherChange - PreviousWeather, NewWeather
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeatherChangeSignature, FGameplayTag, PreviousWeather, FGameplayTag, NewWeather);

// Target weather reached
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTargetWeatherReachedSignature, FGameplayTag, TargetWeather);


UCLASS(Blueprintable, BlueprintType)
class MYTHIC_API AMythicEnvironmentController : public AActor, public IMythicSaveableActor {
    GENERATED_BODY()

private:
    FTimerHandle TimeOfDayTimerHandle;
    FTimerHandle WeatherTimerHandle;

    // The actual component of the height fog actor in the scene
    UPROPERTY()
    UExponentialHeightFogComponent *FogComponent = nullptr;

    // The actual component of the sky atmosphere actor in the scene
    UPROPERTY()
    USkyAtmosphereComponent *SkyAtmosphereComponent = nullptr;

    // Inlined so we skip the function call overhead
    inline void TimeTick();
    inline void WeatherTick();
    inline void HandleWeatherTransition();
    inline void CycleWeather();
    void SetWeatherTransition(const FWeatherCycleInfo &NewWeatherCycle);

    float NightLightIntensity;

public:
    // Sets default values for this actor's properties
    AMythicEnvironmentController();

    virtual void OnConstruction(const FTransform &Transform) override;

    // IMythicSaveableActor Interface
    virtual void SerializeCustomData(TArray<uint8> &OutCustomData) override;
    virtual void DeserializeCustomData(const TArray<uint8> &InCustomData) override;

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

    // How long should the day be, or how long it takes in real time seconds from 07:00 to 20:00 in game time
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time of Day Controller", meta = (ClampMin = "2", Units = "Seconds"))
    float DayLength = 720.0f;

    // How long should the night be, or how long it takes in real time seconds from 20:00 to 07:00 in game time
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time of Day Controller", meta = (ClampMin = "2", Units = "Seconds"))
    float NightLength = 240.0f;

    // Bool to update the directional lights
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time of Day Controller")
    bool bUpdateDirectionalLights = true;

    // Directional light that will act as the sun
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time of Day Controller", meta = (EditCondition = "bUpdateDirectionalLights"))
    ADirectionalLight *DaytimeDirectionalLight;

    // Night time directional light that will act as the moon
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time of Day Controller", meta = (EditCondition = "bUpdateDirectionalLights"))
    ADirectionalLight *NighttimeDirectionalLight;

    // Timespan of the current time of day - The actual
    UPROPERTY(BlueprintReadOnly, EditAnywhere, ReplicatedUsing=OnRep_Time, SaveGame, Category = "Time of Day Controller")
    FTimespan Time;

    // Update frequency of the time. In real time seconds, how often the time will be updated
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Time of Day Controller", meta = (ClampMin = "0.1"))
    float TimeUpdateFrequency = 0.1f;

    // Update frequency of the weather, this is how often the weather will be updated during a transition. In real time seconds
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Time of Day Controller", meta = (ClampMin = "0.1"))
    float WeatherUpdateFrequency = 1.0f;

    // Weather types
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Time of Day Controller | Weather")
    TArray<TObjectPtr<UWeatherType>> WeatherTypes;

    // The Material Parameter Collection used by the weather system
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Time of Day Controller | Weather")
    UMaterialParameterCollection *WeatherMPC;

    // The in-game time it takes for a weather transition to complete
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Time of Day Controller | Weather", meta = (ClampMin = "10"))
    float TransitionDurationInMins = 30.0f;

    // The exponential height fog actor
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Time of Day Controller | Weather")
    AExponentialHeightFog *ExponentialHeightFog;

    // The sky atmosphere actor in the scene
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Time of Day Controller | Weather")
    ASkyAtmosphere *SkyAtmosphere;

private:
    // If this is set, all upcoming transitions will make sure to eventually transition to this weather type
    // I.e If this is set to Rain, which has cloudy weather as a prerequisite, and current weather is clear, the next transition will be to cloudy, then rain.
    // NOTE: If by the time the transition is ready to occur, and the month at that time is not within the month range of the target weather, the transition will NOT happen.
    // Cleared after this weather type is reached
    // Can be set via the function: SetTargetWeather
    // After this goal is reached, the event OnTargetWeatherReached is called
    UPROPERTY(Replicated, SaveGame)
    UWeatherType *GuaranteedTargetWeather;

    UPROPERTY(ReplicatedUsing=OnRep_WeatherTransition, SaveGame)
    FWeatherCycleInfo WeatherTransition;

    ///
    ///~ Current Weather
    // Tag of the current weather type - Set after the transition is complete
    // Flow: CurrentWeather -> Transition -> TargetWeather
    UPROPERTY(ReplicatedUsing=OnRep_CurrentWeather, SaveGame)
    UWeatherType *CurrentWeather;

    // Time the weather changed to the current weather type
    UPROPERTY(SaveGame)
    FTimespan WeatherChangedAt;

    // Transition from these values - Cached values of the MPC when the transition started for interpolation
    // Not replicated, the clients will receive OnRep_CurrentWeatherState and cache the values
    UPROPERTY()
    TArray<FCollectionScalarParameter> TransitionFromScalarValues;

    // Transition from these values - Cached values of the MPC when the transition started for interpolation
    // Not replicated, the clients will receive OnRep_CurrentWeatherState and cache the values
    UPROPERTY()
    TArray<FCollectionVectorParameter> TransitionFromVectorValues;

    // The Fog height falloff at the start of the transition
    float CachedFogHeightFalloff;

    // The Fog density at the start of the transition
    float CachedFogDensity;

    // Time the transition started, used to track the progress of the transition
    // Set in the OnRep_CurrentWeatherState function
    FTimespan TransitionStartedAt;

    bool isCurrentWeatherExpired() const;

    // The parameter name for the Wind Direction in the WeatherMPC
    FName WindTargetParameterName = "WindDirection";

    // Helper to force apply weather visuals to MPC
    void ApplyWeatherVisuals(const UWeatherType *Weather);

public:
    void UpdateLighting() const;

    // OnRep Functions
    UFUNCTION()
    void OnRep_Time();

    UFUNCTION()
    void OnRep_CurrentWeather(UWeatherType *PreviousWeather);

    UFUNCTION()
    void OnRep_WeatherTransition();

    ///~ RPCs ---------------------------------------------------------------
    // multicast to all clients to sync the time of day
    UFUNCTION(NetMulticast, Unreliable)
    void MulticastSyncGameWorldTimer(const FTimespan &NewTimespan, const FTimespan &OldTimespan);

    // Multicast to all clients to sync wind direction
    UFUNCTION(NetMulticast, Reliable)
    void MulticastSyncWindTarget(const FLinearColor &WindDirection);

    // Multicast to all clients to change sky atmosphere absorption for dynamic sunrise/sunset colors
    UFUNCTION(NetMulticast, Reliable)
    void MulticastSyncSkyAtmosphereAbsorption(FLinearColor Absorption, float AbsorptionScalar);


    ///~ Getters ---------------------------------------------------------------

    FTimespan GetTimespan() const { return this->Time; }
    FDateTime GetDateTime() const { return AsDateTime(this->Time); }
    bool IsWeatherPaused() const { return this->WeatherTimerHandle.IsValid(); }

    UFUNCTION(BlueprintCallable, Category = "Time of Day Controller")
    UWeatherType *GetCurrentWeather() const { return this->CurrentWeather; }

    const TArray<TObjectPtr<UWeatherType>> &GetWeatherTypes() const { return WeatherTypes; }

    UWeatherType *GetWeatherTypeByTag(FGameplayTag Tag) const;

    // Maps the current time of day to sun's yaw rotation (0 = 18:00, 90 = 00:00, 180 = 06:00, 270 = 12:00). Multiply by -1 to get moon's yaw rotation
    float GetSunPositionForCurrentTime() const;

    // Processes the pre and post weather types to determine the next weather type to transition to, to eventually reach the target weather
    UWeatherType *GetNextWeatherTypeToReachTargetWeather(UWeatherType *FromWeather, UWeatherType *TargetWeather);

    ///~ Setters - SERVER ONLY ---------------------------------------------------------------
    void SetTime(const FDateTime &DateTime);
    void AddTime(const FTimespan &ByTime);
    void SetTimeUpdateFrequency(float Frequency);
    void PauseTime() const;
    void ResumeTime() const;

    void SetGuaranteedTargetWeather(FGameplayTag TargetWeather);
    void PauseWeather() const;
    void ResumeWeather() const;

    void SetWindTargetPosition(const FLinearColor &Position);

    /// DELEGATES ---------------------------------------------------------------
    // Called when the time of day changes - Morning, Afternoon, Evening, Night
    UPROPERTY(BlueprintAssignable, Category = "Time of Day Controller")
    FOnDayTimeChangedSignature DayTimeChangeDelegate;

    // Called when the hour changes - 0-23
    UPROPERTY(BlueprintAssignable, Category = "Time of Day Controller")
    FOnHourChangedSignature HourChangeDelegate;

    // Called when the day changes - 1-30
    UPROPERTY(BlueprintAssignable, Category = "Time of Day Controller")
    FOnDayChangedSignature DayChangeDelegate;

    // Called when the month changes - 1-12
    UPROPERTY(BlueprintAssignable, Category = "Time of Day Controller")
    FOnMonthChangedSignature MonthChangeDelegate;

    // Called when the year changes - 1 - Infinite
    UPROPERTY(BlueprintAssignable, Category = "Time of Day Controller")
    FOnYearChangedSignature YearChangeDelegate;

    // Called when the weather transition begins - FromWeather, ToWeather, TransitionDuration
    UPROPERTY(BlueprintAssignable, Category = "Time of Day Controller")
    FOnWeatherChangeStartSignature WeatherTransitionDelegate;

    // Called when the weather has fully transitioned - PreviousWeather, NewWeather
    UPROPERTY(BlueprintAssignable, Category = "Time of Day Controller")
    FOnWeatherChangeSignature WeatherChangeDelegate;

    // Called when the target weather has been reached. Can be set via SetTargetWeather.
    UPROPERTY(BlueprintAssignable, Category = "Time of Day Controller")
    FOnTargetWeatherReachedSignature TargetWeatherReachedDelegate;
    /// DELEGATES END
    ///
    ///

private:
    // Friend is MythicEnvironmentSubsystem
    friend class UMythicEnvironmentSubsystem;
};

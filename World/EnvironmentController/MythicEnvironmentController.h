
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

static int GetDayOfMonth(FTimespan timespan) {
    auto day = timespan.GetDays() % 30;
    return day == 0 ? 30 : day;
}

static int GetYear(FTimespan timespan) {
    return timespan.GetDays() / 360 + 1;
}

static int GetMonthOfYear(FTimespan timespan) {
    auto month = timespan.GetDays() % 360 / 30;
    return month == 0 ? 12 : month;
}

static FDateTime AsDateTime(FTimespan timespan) {
    auto year = GetYear(timespan);
    auto month = GetMonthOfYear(timespan);
    auto day = GetDayOfMonth(timespan);
    auto hour = timespan.GetHours();
    auto minute = timespan.GetMinutes();
    auto second = timespan.GetSeconds();

    day = FMath::Min(day, FDateTime::DaysInMonth(year, month));

    return FDateTime(year, month, day, hour, minute, second);
}

static ESeason MonthAsSeason(int32 Month) {
    if (Month == 3 || Month == 4 || Month == 5) {
        return Spring;
    }

    if (Month == 6 || Month == 7 || Month == 8) {
        return Summer;
    }

    if (Month == 9 || Month == 10 || Month == 11) {
        return Autumn;
    }

    if (Month < 1 || Month > 12) {
        UE_LOG(Myth, Error, TEXT("Invalid month - %d"), Month);
    }

    return Winter;
}

static EDayTime HourAsDayTime(uint8 CurrentHour) {
    if (CurrentHour >= 7 && CurrentHour < 12) {
        return Morning;
    }

    if (CurrentHour >= 12 && CurrentHour < 17) {
        return Afternoon;
    }

    if (CurrentHour >= 17 && CurrentHour < 20) {
        return Evening;
    }

    return Night;
}

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDayTimeChangedSignature, EDayTime, PrevDayTime, EDayTime, NewDayTime);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHourChangedSignature, int32, PrevHour, int32, NewHour);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDayChangedSignature, int32, PrevDay, int32, NewDay);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnMonthChangedSignature, int32, PrevMonth, int32, NewMonth, ESeason, PrevSeason, ESeason, NewSeason);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnYearChangedSignature, int32, PrevYear, int32, NewYear);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnWeatherChangeStartSignature, FGameplayTag, FromWeather, FGameplayTag, ToWeather, float, TransitionLength);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeatherChangeSignature, FGameplayTag, PreviousWeather, FGameplayTag, NewWeather);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTargetWeatherReachedSignature, FGameplayTag, TargetWeather);


UCLASS(Blueprintable, BlueprintType)
class MYTHIC_API AMythicEnvironmentController : public AActor, public IMythicSaveableActor {
    GENERATED_BODY()

    FTimerHandle TimeOfDayTimerHandle;
    FTimerHandle WeatherTimerHandle;

    UPROPERTY()
    UExponentialHeightFogComponent *FogComponent = nullptr;

    UPROPERTY()
    USkyAtmosphereComponent *SkyAtmosphereComponent = nullptr;

    inline void TimeTick();
    inline void WeatherTick();
    inline void HandleWeatherTransition();
    inline void CycleWeather();
    void SetWeatherTransition(const FWeatherCycleInfo &NewWeatherCycle);

    float NightLightIntensity;

public:
    AMythicEnvironmentController();

    virtual void OnConstruction(const FTransform &Transform) override;

    virtual void SerializeCustomData(TArray<uint8> &OutCustomData) override;
    virtual void DeserializeCustomData(const TArray<uint8> &InCustomData) override;

protected:
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

    // Significance of the World Chronicle beat posted when the calendar SEASON turns ("Winter descends"). Must be >= the
    // chronicle's MinSignificance (default 0.5) to surface; defaults slightly above a routine weather beat since a season
    // change is a bigger world event. 0 disables the beat. (Season changes always fire; only the news beat is gated here.)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Time of Day Controller | Weather", meta = (UIMin = "0.0"))
    float SeasonNewsSignificance = 0.6f;

    // The exponential height fog actor
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Time of Day Controller | Weather")
    AExponentialHeightFog *ExponentialHeightFog;

    // The sky atmosphere actor in the scene
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Time of Day Controller | Weather")
    ASkyAtmosphere *SkyAtmosphere;

private:
    UPROPERTY(Replicated, SaveGame)
    UWeatherType *GuaranteedTargetWeather;

    UPROPERTY(ReplicatedUsing=OnRep_WeatherTransition, SaveGame)
    FWeatherCycleInfo WeatherTransition;

    UPROPERTY(ReplicatedUsing=OnRep_CurrentWeather, SaveGame)
    UWeatherType *CurrentWeather;

    UPROPERTY(SaveGame)
    FTimespan WeatherChangedAt;

    UPROPERTY()
    TArray<FCollectionScalarParameter> TransitionFromScalarValues;

    UPROPERTY()
    TArray<FCollectionVectorParameter> TransitionFromVectorValues;

    float CachedFogHeightFalloff;

    float CachedFogDensity;

    FTimespan TransitionStartedAt;

    bool isCurrentWeatherExpired() const;

    FName WindTargetParameterName = "WindDirection";

    void ApplyWeatherVisuals(const UWeatherType *Weather);

public:
    void UpdateLighting() const;

    UFUNCTION()
    void OnRep_Time();

    UFUNCTION()
    void OnRep_CurrentWeather(UWeatherType *PreviousWeather);

    UFUNCTION()
    void OnRep_WeatherTransition();

    UFUNCTION(NetMulticast, Reliable)
    void MulticastSyncGameWorldTimer(const FTimespan &NewTimespan, const FTimespan &OldTimespan);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastSyncWindTarget(const FLinearColor &WindDirection);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastSyncSkyAtmosphereAbsorption(FLinearColor Absorption, float AbsorptionScalar);


    FTimespan GetTimespan() const { return this->Time; }
    FDateTime GetDateTime() const { return AsDateTime(this->Time); }
    bool IsWeatherPaused() const;

    UFUNCTION(BlueprintCallable, Category = "Time of Day Controller")
    UWeatherType *GetCurrentWeather() const { return this->CurrentWeather; }

    UWeatherType *GetGuaranteedTargetWeather() const { return this->GuaranteedTargetWeather; }

    void GetWeatherTransitionInfo(bool &bActive, FGameplayTag &TargetTag, float &Progress, double &ElapsedMinutes, float &LengthMinutes) const {
        bActive = !this->WeatherTransition.TransitionToWeather.IsNull();
        const UWeatherType *Target = this->WeatherTransition.TransitionToWeather.Get();
        TargetTag = (Target && Target->Tag.IsValid()) ? Target->Tag : FGameplayTag();
        ElapsedMinutes = (this->Time - this->TransitionStartedAt).GetTotalMinutes();
        LengthMinutes = this->TransitionDurationInMins;
        Progress = ComputeWeatherTransitionProgress(this->WeatherTransition.bSetInstantly, ElapsedMinutes, LengthMinutes);
    }

    const TArray<TObjectPtr<UWeatherType>> &GetWeatherTypes() const { return WeatherTypes; }

    UWeatherType *GetWeatherTypeByTag(FGameplayTag Tag) const;

    float GetSunPositionForCurrentTime() const;

    UWeatherType *GetNextWeatherTypeToReachTargetWeather(UWeatherType *FromWeather, UWeatherType *TargetWeather);

    static float ComputeWeatherTransitionProgress(bool bSetInstantly, double ElapsedMinutes, float DurationMinutes) {
        if (bSetInstantly || DurationMinutes <= 0.0f) {
            return 1.0f;
        }
        return FMath::Clamp(static_cast<float>(ElapsedMinutes / DurationMinutes), 0.0f, 1.0f);
    }

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

private:
    friend class UMythicEnvironmentSubsystem;
};

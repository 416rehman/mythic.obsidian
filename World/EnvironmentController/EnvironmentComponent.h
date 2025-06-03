// 

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentTypes.h"
#include "MythicEnvironmentController.h"
#include "Components/ActorComponent.h"
#include "EnvironmentComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MYTHIC_API UEnvironmentComponent : public UActorComponent {
    GENERATED_BODY()
    // Constructor
    UEnvironmentComponent();

protected:
    UFUNCTION()
    virtual void BeginPlay() override;

    UFUNCTION()
    void OnEnvironmentControllerRegistered(AMythicEnvironmentController *EnvironmentController);

    // Function to call when the hour changes
    UFUNCTION()
    void CallOnHourChanged(int32 OldHour, int32 NewHour) {
        this->OnHourChanged.Broadcast(OldHour, NewHour);
    }

    UFUNCTION()
    void CallOnDaytimeChanged(EDayTime OldDayTime, EDayTime NewDayTime) { this->OnDaytimeChanged.Broadcast(OldDayTime, NewDayTime); }

    UFUNCTION()
    void CallOnDayChanged(int32 OldDay, int32 NewDay) { this->OnDayChanged.Broadcast(OldDay, NewDay); }

    UFUNCTION()
    void CallOnMonthChanged(int32 OldMonth, int32 NewMonth, ESeason OldSeason, ESeason NewSeason) {
        this->OnMonthChanged.Broadcast(OldMonth, NewMonth, OldSeason, NewSeason);
    }

    UFUNCTION()
    void CallOnYearChanged(int32 OldYear, int32 NewYear) { this->OnYearChanged.Broadcast(OldYear, NewYear); }

    UFUNCTION()
    void CallOnWeatherChangeStarted(FGameplayTag From, FGameplayTag To, float Length) { this->OnWeatherChangeStarted.Broadcast(From, To, Length); }

    UFUNCTION()
    void CallOnWeatherChanged(FGameplayTag OldWeather, FGameplayTag NewWeather) { this->OnWeatherChanged.Broadcast(OldWeather, NewWeather); }

    UFUNCTION()
    void CallOnTargetWeatherReached(FGameplayTag TargetWeather) { this->OnTargetWeatherReached.Broadcast(TargetWeather); }

public:
    // When the hour changes. 0-23
    UPROPERTY(BlueprintAssignable, Category = "Environment")
    FOnHourChangedSignature OnHourChanged;

    // When the daytime changes. Morning, afternoon, evening, night
    UPROPERTY(BlueprintAssignable, Category = "Environment")
    FOnDayTimeChangedSignature OnDaytimeChanged;

    // When the day changes. 1-31
    UPROPERTY(BlueprintAssignable, Category = "Environment")
    FOnDayChangedSignature OnDayChanged;

    // When the month changes. 1-12
    UPROPERTY(BlueprintAssignable, Category = "Environment")
    FOnMonthChangedSignature OnMonthChanged;

    // When the year changes.
    UPROPERTY(BlueprintAssignable, Category = "Environment")
    FOnYearChangedSignature OnYearChanged;

    // When the weather change starts
    UPROPERTY(BlueprintAssignable, Category = "Environment")
    FOnWeatherChangeStartSignature OnWeatherChangeStarted;

    // When the weather changes
    UPROPERTY(BlueprintAssignable, Category = "Environment")
    FOnWeatherChangeSignature OnWeatherChanged;

    // When the target weather is reached
    UPROPERTY(BlueprintAssignable, Category = "Environment")
    FOnTargetWeatherReachedSignature OnTargetWeatherReached;
};

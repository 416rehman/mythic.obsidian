// 
#include "EnvironmentComponent.h"

#include "Mythic.h"
#include "MythicEnvironmentSubsystem.h"

UEnvironmentComponent::UEnvironmentComponent() {}

void UEnvironmentComponent::BeginPlay() {
    Super::BeginPlay();
    // Get the game instance
    UGameInstance *GameInstance = GetWorld()->GetGameInstance();
    if (!GameInstance) {
        UE_LOG(Myth, Error, TEXT("UEnvironmentComponent::BeginPlay: GameInstance is null"));
    }
    // Get the Environment subsystem
    UMythicEnvironmentSubsystem *EnvironmentSubsystem = GameInstance->GetSubsystem<UMythicEnvironmentSubsystem>();
    if (!EnvironmentSubsystem) {
        UE_LOG(Myth, Error, TEXT("UEnvironmentComponent::BeginPlay: EnvironmentSubsystem is null"));
        return;
    }
    // Listen for the environment controller to be set in the environment subsystem then do the following
    if (auto Controller = EnvironmentSubsystem->GetEnvironmentController()) {
        OnEnvironmentControllerRegistered(Controller);
    }
    else {
        EnvironmentSubsystem->OnEnvironmentControllerRegisterDelegate.AddUniqueDynamic(this, &UEnvironmentComponent::OnEnvironmentControllerRegistered);
    }
}

void UEnvironmentComponent::OnEnvironmentControllerRegistered(AMythicEnvironmentController *EnvironmentController) {
    if (!EnvironmentController) {
        UE_LOG(Myth, Error, TEXT("UEnvironmentComponent::OnEnvironmentControllerRegistered: EnvironmentController is null"));
        return;
    }

    // When the environment controller broadcasts its events, broadcast our own events
    EnvironmentController->HourChangeDelegate.AddUniqueDynamic(this, &UEnvironmentComponent::CallOnHourChanged);
    EnvironmentController->DayTimeChangeDelegate.AddUniqueDynamic(this, &UEnvironmentComponent::CallOnDaytimeChanged);
    EnvironmentController->DayChangeDelegate.AddUniqueDynamic(this, &UEnvironmentComponent::CallOnDayChanged);
    EnvironmentController->MonthChangeDelegate.AddUniqueDynamic(this, &UEnvironmentComponent::CallOnMonthChanged);
    EnvironmentController->YearChangeDelegate.AddUniqueDynamic(this, &UEnvironmentComponent::CallOnYearChanged);
    EnvironmentController->WeatherTransitionDelegate.AddUniqueDynamic(this, &UEnvironmentComponent::CallOnWeatherChangeStarted);
    EnvironmentController->WeatherChangeDelegate.AddUniqueDynamic(this, &UEnvironmentComponent::CallOnWeatherChanged);
    EnvironmentController->TargetWeatherReachedDelegate.AddUniqueDynamic(this, &UEnvironmentComponent::CallOnTargetWeatherReached);

    // CATCH-UP LOGIC: Broadcast current state immediately so actors initialize correctly
    // 1. DayTime
    auto CurrentHour = EnvironmentController->GetTimespan().GetHours();
    auto CurrentDayTime = HourAsDayTime(CurrentHour);
    // Broadcast with same Old/New to indicate immediate state
    this->CallOnDaytimeChanged(CurrentDayTime, CurrentDayTime);

    // 2. Weather
    if (auto CurrentWeather = EnvironmentController->GetCurrentWeather()) {
        this->CallOnWeatherChanged(CurrentWeather->Tag, CurrentWeather->Tag);
    }
}

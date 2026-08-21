#include "EnvironmentComponent.h"

#include "Mythic.h"
#include "MythicEnvironmentSubsystem.h"

UEnvironmentComponent::UEnvironmentComponent() {}

void UEnvironmentComponent::BeginPlay() {
    Super::BeginPlay();
    UGameInstance *GameInstance = GetWorld()->GetGameInstance();
    if (!GameInstance) {
        UE_LOG(Myth, Error, TEXT("UEnvironmentComponent::BeginPlay: GameInstance is null"));
    }
    UMythicEnvironmentSubsystem *EnvironmentSubsystem = GameInstance->GetSubsystem<UMythicEnvironmentSubsystem>();
    if (!EnvironmentSubsystem) {
        UE_LOG(Myth, Error, TEXT("UEnvironmentComponent::BeginPlay: EnvironmentSubsystem is null"));
        return;
    }
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

    EnvironmentController->HourChangeDelegate.AddUniqueDynamic(this, &UEnvironmentComponent::CallOnHourChanged);
    EnvironmentController->DayTimeChangeDelegate.AddUniqueDynamic(this, &UEnvironmentComponent::CallOnDaytimeChanged);
    EnvironmentController->DayChangeDelegate.AddUniqueDynamic(this, &UEnvironmentComponent::CallOnDayChanged);
    EnvironmentController->MonthChangeDelegate.AddUniqueDynamic(this, &UEnvironmentComponent::CallOnMonthChanged);
    EnvironmentController->YearChangeDelegate.AddUniqueDynamic(this, &UEnvironmentComponent::CallOnYearChanged);
    EnvironmentController->WeatherTransitionDelegate.AddUniqueDynamic(this, &UEnvironmentComponent::CallOnWeatherChangeStarted);
    EnvironmentController->WeatherChangeDelegate.AddUniqueDynamic(this, &UEnvironmentComponent::CallOnWeatherChanged);
    EnvironmentController->TargetWeatherReachedDelegate.AddUniqueDynamic(this, &UEnvironmentComponent::CallOnTargetWeatherReached);

    auto CurrentHour = EnvironmentController->GetTimespan().GetHours();
    auto CurrentDayTime = HourAsDayTime(CurrentHour);
    this->CallOnDaytimeChanged(CurrentDayTime, CurrentDayTime);

    if (auto CurrentWeather = EnvironmentController->GetCurrentWeather()) {
        this->CallOnWeatherChanged(CurrentWeather->Tag, CurrentWeather->Tag);
    }
}

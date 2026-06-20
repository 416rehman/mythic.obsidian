// 


#include "MythicEnvironmentSubsystem.h"

#include "Mythic.h"
#include "EnvironmentTags.h" // Environment.Time.* / Environment.Season.* world-state tags

void UMythicEnvironmentSubsystem::SetEnvironmentController(AMythicEnvironmentController *Controller) {
    this->EnvironmentController = Controller;
    if (this->EnvironmentController) {
        UE_LOG(Myth_Environment, Log, TEXT("EnvironmentController set to %s"), *Controller->GetName());
        this->OnEnvironmentControllerRegisterDelegate.Broadcast(Controller);

        // After the broadcast, clear the delegate
        this->OnEnvironmentControllerRegisterDelegate.Clear();
    }
    else {
        UE_LOG(Myth_Environment, Log, TEXT("EnvironmentController set to nullptr"));
    }
}

AMythicEnvironmentController *UMythicEnvironmentSubsystem::GetEnvironmentController() const {
    return this->EnvironmentController;
}

FDateTime UMythicEnvironmentSubsystem::GetCurrentTime() const {
    if (!this->EnvironmentController) {
        UE_LOG(Myth_Environment, Error, TEXT("EnvironmentController is not set!"));
        return FDateTime::Now();
    }

    return this->EnvironmentController->GetDateTime();
}

FGameplayTag UMythicEnvironmentSubsystem::GetWeather() const {
    if (this->EnvironmentController) {
        if (auto currentWeather = this->EnvironmentController->GetCurrentWeather()) {
            return currentWeather->Tag;
        }
    }

    UE_LOG(Myth_Environment, Error, TEXT("EnvironmentController is not set!"));
    return FGameplayTag::EmptyTag;
}

ESeason UMythicEnvironmentSubsystem::GetSeason() const {
    if (this->EnvironmentController) {
        auto Month = GetMonthOfYear(this->EnvironmentController->GetTimespan());
        return MonthAsSeason(Month);
    }
    UE_LOG(Myth_Environment, Error, TEXT("EnvironmentController is not set!"));
    return ESeason::Winter;
}

EDayTime UMythicEnvironmentSubsystem::GetDayTime() const {
    if (!this->EnvironmentController) {
        UE_LOG(Myth_Environment, Error, TEXT("EnvironmentController is not set!"));
        return EDayTime::Night;
    }

    return HourAsDayTime(this->EnvironmentController->GetTimespan().GetHours());
}

FGameplayTag UMythicEnvironmentSubsystem::GetDayTimeTag() const {
    // Controller-gated (unlike GetDayTime, NO fail-unsafe Night default): an absent clock yields an empty tag, so a
    // world-state consumer reads the time as "unknown" rather than wrongly "night".
    if (!this->EnvironmentController) {
        return FGameplayTag::EmptyTag;
    }
    switch (GetDayTime()) {
    case EDayTime::Morning:
        return Environment_Time_Morning;
    case EDayTime::Afternoon:
        return Environment_Time_Afternoon;
    case EDayTime::Evening:
        return Environment_Time_Evening;
    case EDayTime::Night:
        return Environment_Time_Night;
    default:
        return FGameplayTag::EmptyTag;
    }
}

FGameplayTag UMythicEnvironmentSubsystem::GetSeasonTag() const {
    if (!this->EnvironmentController) {
        return FGameplayTag::EmptyTag;
    }
    switch (GetSeason()) {
    case ESeason::Spring:
        return Environment_Season_Spring;
    case ESeason::Summer:
        return Environment_Season_Summer;
    case ESeason::Autumn:
        return Environment_Season_Autumn;
    case ESeason::Winter:
        return Environment_Season_Winter;
    default:
        return FGameplayTag::EmptyTag;
    }
}

UWeatherType *UMythicEnvironmentSubsystem::GetWeatherTypeByTag(FGameplayTag Tag) {
    if (!this->EnvironmentController) {
        UE_LOG(Myth_Environment, Error, TEXT("EnvironmentController is not set!"));
        return nullptr;
    }

    return this->EnvironmentController->GetWeatherTypeByTag(Tag);
}

bool UMythicEnvironmentSubsystem::IsWeatherPaused() {
    if (!this->EnvironmentController) {
        UE_LOG(Myth_Environment, Error, TEXT("EnvironmentController is not set!"));
        return false;
    }

    return this->EnvironmentController->IsWeatherPaused();
}

void UMythicEnvironmentSubsystem::SetTargetWeather(FGameplayTag TargetWeather) {
    if (!this->EnvironmentController) {
        UE_LOG(Myth_Environment, Error, TEXT("EnvironmentController is not set!"));
        return;
    }

    this->EnvironmentController->SetGuaranteedTargetWeather(TargetWeather);
}

void UMythicEnvironmentSubsystem::SetWeatherInstantly(FGameplayTag Weather) {
    if (!this->EnvironmentController) {
        UE_LOG(Myth_Environment, Error, TEXT("EnvironmentController is not set!"));
        return;
    }

    // Find the weather type by tag
    auto TargetWeather = this->EnvironmentController->WeatherTypes.FindByPredicate([&Weather](UWeatherType *WeatherType) {
        return WeatherType->Tag.MatchesTag(Weather);
    });

    if (!TargetWeather) {
        UE_LOG(Myth_Environment, Error, TEXT("The EnvironmentController does not have a weather type with the tag %s"), *Weather.ToString());
        return;
    }

    bool bSetInstantly = true;
    auto WeatherTransition = FWeatherCycleInfo(*TargetWeather, this->EnvironmentController->GetTimespan(), bSetInstantly);

    this->EnvironmentController->SetWeatherTransition(WeatherTransition);
}

void UMythicEnvironmentSubsystem::PauseWeather() {
    if (!this->EnvironmentController) {
        UE_LOG(Myth_Environment, Error, TEXT("EnvironmentController is not set!"));
        return;
    }

    return this->EnvironmentController->PauseWeather();
}

void UMythicEnvironmentSubsystem::ResumeWeather() {
    if (!this->EnvironmentController) {
        UE_LOG(Myth_Environment, Error, TEXT("EnvironmentController is not set!"));
        return;
    }

    return this->EnvironmentController->ResumeWeather();
}

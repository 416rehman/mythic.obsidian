// 

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentTypes.h"
#include "GameplayTagContainer.h"
#include "MythicEnvironmentController.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MythicEnvironmentSubsystem.generated.h"

// Delegate that is called when the Environment controller is registered
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnvironmentControllerRegistered, AMythicEnvironmentController *, EnvironmentController);

/**
 * The environment subsystem provides an easy interface for managing the environment of the game world.
 * This includes weather, time of day, and other environmental factors.
 */
UCLASS()
class MYTHIC_API UMythicEnvironmentSubsystem : public UGameInstanceSubsystem {
    GENERATED_BODY()

private:
    // This is the environment controller that will be used to manage the environment
    // Can be registered via the SetEnvironmentController method
    UPROPERTY()
    AMythicEnvironmentController *EnvironmentController;

public:
    /// Getters ---------------------------------------------------------------

    void SetEnvironmentController(AMythicEnvironmentController *Controller);

    UFUNCTION(BlueprintCallable, Category="Environment")
    AMythicEnvironmentController *GetEnvironmentController() const;

    // Returns the current time of the game world
    UFUNCTION(BlueprintCallable, Category="Environment")
    FDateTime GetCurrentTime() const;

    // Returns the current weather of the game world
    UFUNCTION(BlueprintCallable, Category="Environment")
    FGameplayTag GetWeather() const;

    // Returns the current season of the game world. Spring, Summer, Autumn, Winter
    UFUNCTION(BlueprintCallable, Category="Environment")
    ESeason GetSeason() const;

    // Returns the current DayTime of the game world. Morning, Afternoon, Evening, Night
    UFUNCTION(BlueprintCallable, Category="Environment")
    EDayTime GetDayTime() const;

    // Get Weather Type by Tag
    UFUNCTION(BlueprintCallable, Category="Environment")
    UWeatherType *GetWeatherTypeByTag(FGameplayTag Tag);

    // Is the weather paused
    UFUNCTION(BlueprintCallable, Category="Environment")
    bool IsWeatherPaused();


    /// Setters ---------------------------------------------------------------

    // Set the target weather type to transition to. Will influence the weather processing to eventually transition to the target weather type
    // The OnTargetWeatherReached event is called when the target weather is reached
    UFUNCTION(BlueprintCallable, Category="Environment")
    void SetTargetWeather(FGameplayTag TargetWeather);

    // Sets weather instantly
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Environment")
    void SetWeatherInstantly(FGameplayTag Weather);

    // Pause weather so it will stay at the current weather type
    // SERVER ONLY
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Environment")
    void PauseWeather();

    // Resume weather so it will continue transitioning
    // SERVER ONLY
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Environment")
    void ResumeWeather();

    // Delegate that is called when the Environment controller is registered
    UPROPERTY(BlueprintAssignable, Category="Environment")
    FOnEnvironmentControllerRegistered OnEnvironmentControllerRegisterDelegate;
};

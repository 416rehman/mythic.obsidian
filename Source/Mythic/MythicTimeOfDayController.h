// 

#pragma once

#include "CoreMinimal.h"
#include "Engine/DirectionalLight.h"
#include "GameFramework/Actor.h"
#include "MythicTimeOfDayController.generated.h"

UENUM(BlueprintType)
enum EDayTime
{
	Morning UMETA(DisplayName = "Morning"),
	Afternoon UMETA(DisplayName = "Afternoon"),
	Evening UMETA(DisplayName = "Evening"),
	Night UMETA(DisplayName = "Night")
};

// Delegate signature
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTimeOfDayChangedSignature, EDayTime, PrevDayTime, EDayTime, NewDayTime);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHourChangedSignature, int32, PrevHour, int32, NewHour);
// replicated
UCLASS()
class MYTHIC_API AMythicTimeOfDayController : public AActor
{
	GENERATED_BODY()

private:
	FTimerHandle TimeOfDayTimerHandle;

	// multicast to all clients to sync the time of day
	UFUNCTION(NetMulticast, Reliable)
	void MulticastSyncTimeOfDay(const FTimespan& Timespan);
	void MulticastSyncTimeOfDay_Implementation(const FTimespan& Timespan);
	
	void UpdateTimeOfDay();

	float NightLightIntensity;

public:
	// Sets default values for this actor's properties
	AMythicTimeOfDayController();

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// How long should the morning be, or how long it takes in real time seconds from 06:00 to 12:00 in game time
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time of Day Controller", meta = (ClampMin = "2"))
	float MorningLength = 60.0f;

	// How long should the afternoon be, or how long it takes in real time seconds from 12:00 to 18:00 in game time
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time of Day Controller", meta = (ClampMin = "2"))
	float AfternoonLength = 60.0f;

	// How long should the evening be, or how long it takes in real time seconds from 18:00 to 24:00 in game time
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time of Day Controller", meta = (ClampMin = "2"))
	float EveningLength = 60.0f;

	// How long should the night be, or how long it takes in real time seconds from 00:00 to 06:00 in game time
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time of Day Controller", meta = (ClampMin = "2"))
	float NightLength = 60.0f;

	// Bool to update the directional lights
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time of Day Controller")
	bool bUpdateDirectionalLights = true;

	// If true, night light will always be on. (Daylight will always be off during night)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time of Day Controller")
	bool KeepNightLightDuringDay = false;
	
	// Directional light that will act as the sun
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time of Day Controller", meta = (EditCondition = "bUpdateDirectionalLights"))
	ADirectionalLight* DaytimeDirectionalLight;

	// Night time directional light that will act as the moon
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time of Day Controller", meta = (EditCondition = "bUpdateDirectionalLights"))
	ADirectionalLight* NighttimeDirectionalLight;
	
	// Timespan of the current time of day
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Time of Day Controller")
	FTimespan CurrentTimeOfDay;

	// Update frequency, update DEFAULTS only, read only in blueprint
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Time of Day Controller", meta = (ClampMin = "0.01"))
	float UpdateFrequency = 0.1f;

public:

	UPROPERTY(BlueprintAssignable, Category = "Time of Day Controller")
	FOnTimeOfDayChangedSignature OnTimeOfDayChanged;

	UPROPERTY(BlueprintAssignable, Category = "Time of Day Controller")
	FOnHourChangedSignature OnHourChanged;

	// Get the current day of month (1-30)
	UFUNCTION(BlueprintCallable, Category = "Time of Day Controller")
	int GetDayOfMonth();

	// Get the current month (1-12)
	UFUNCTION(BlueprintCallable, Category = "Time of Day Controller")
	int GetMonth();

	// Get the current year (1 - Infinite)
	UFUNCTION(BlueprintCallable, Category = "Time of Day Controller")
	int GetYear();

	void UpdateSunAndMoon();

	UFUNCTION(BlueprintCallable, Category = "Time of Day Controller")
	EDayTime GetDayTime(int32 current_hour);
	
	// Maps the current time of day to sun's yaw rotation (0 = 18:00, 90 = 00:00, 180 = 06:00, 270 = 12:00). Multiply by -1 to get moon's yaw rotation
	UFUNCTION(BlueprintCallable, Category = "Time of Day Controller", meta=(ReturnDisplayName = "Yaw"))
	float GetSunPositionForCurrentTime();

	// change time function
	UFUNCTION(BlueprintCallable, Category = "Time of Day Controller")
	void SetTime(int32 hour, int32 minute, int32 second);
};

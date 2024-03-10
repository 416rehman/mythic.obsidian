// 


#include "TimeOfDayController.h"

#include "Mythic.h"
#include "Components/LightComponent.h"

// Sets default values
ATimeOfDayController::ATimeOfDayController() {
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	this->UpdateFrequency = 0.01f;

	// replicate to all clients
	bReplicates = true;
}

void ATimeOfDayController::OnConstruction(const FTransform& Transform) {
	Super::OnConstruction(Transform);

	// Update the sun and moon
	UpdateSunAndMoon();

	// Update the time of day
	UpdateTimeOfDay();
}

// Maps the current time of day to sun's yaw rotation (0 = 18:00, 90 = 00:00, 180 = 06:00, 270 = 12:00)
void ATimeOfDayController::BeginPlay() {
	Super::BeginPlay();

	// Start a timer to update the time of day based on the morning, afternoon, evening, and night lengths
	GetWorldTimerManager().SetTimer(this->TimeOfDayTimerHandle, this, &ATimeOfDayController::UpdateTimeOfDay,
	                                this->UpdateFrequency, true);

	if (NighttimeDirectionalLight) {
		NightLightIntensity = NighttimeDirectionalLight->GetLightComponent()->Intensity;
	}
	UpdateSunAndMoon();

	//Initial Broadcast
	const auto newDayTime = this->GetDayTime(this->CurrentTimeOfDay.GetHours());
	// delay the broadcast to allow the game to start
	FTimerHandle handle;
	GetWorldTimerManager().SetTimer(handle, [this, newDayTime]() {
		this->OnTimeOfDayChanged.Broadcast(Night, newDayTime);
	}, 1.0f, false);
}

float ATimeOfDayController::GetSunPositionForCurrentTime() {
	auto todaysSeconds = static_cast<int>(this->CurrentTimeOfDay.GetTotalMilliseconds()) % 86400000;

	UE::Math::TVector2<float> timeRange(0, 86400000);
	UE::Math::TVector2<float> sunRange(0 + 90, 359.9 + 90);
	float sunPos = FMath::GetMappedRangeValueClamped(timeRange, sunRange, todaysSeconds);

	return sunPos;
}

void ATimeOfDayController::SetTime(int32 hour, int32 minute, int32 second) {
	auto currentHour = this->CurrentTimeOfDay.GetHours();
	this->CurrentTimeOfDay = FTimespan(hour, minute, second);
	this->OnHourChanged.Broadcast(currentHour, this->CurrentTimeOfDay.GetHours());
	auto currentTimeOfDay = this->GetDayTime(currentHour);
	auto newTimeOfDay = this->GetDayTime(hour);
	if (currentTimeOfDay != newTimeOfDay) {
		this->OnTimeOfDayChanged.Broadcast(currentTimeOfDay, newTimeOfDay);
	}
}

// Get day as the day of the month (1-30)

int ATimeOfDayController::GetDayOfMonth() {
	auto day = this->CurrentTimeOfDay.GetDays() % 30;
	if (day == 0) {
		day = 30;
	}
	return day;
}

// Get month as the month of the year (1-12)

int ATimeOfDayController::GetMonth() {
	const auto dayOfMonth = this->GetDayOfMonth(); // Get day of the month (1-30)
	const auto monthNum = dayOfMonth == 30
		                      ? this->CurrentTimeOfDay.GetDays() / 30 - 1
		                      : this->CurrentTimeOfDay.GetDays() / 30;
	// Get month number (0-11)
	const auto monthOfYear = monthNum % 12; // Get month of the year (0-11)

	return monthOfYear + 1;
}

// Get year as the year of the game (1-Infinity)

int ATimeOfDayController::GetYear() {
	return this->CurrentTimeOfDay.GetDays() / 360 + 1;
}

void ATimeOfDayController::UpdateSunAndMoon() {
	if (bUpdateDirectionalLights) {
		auto yaw = this->GetSunPositionForCurrentTime();
		if (DaytimeDirectionalLight) {
			DaytimeDirectionalLight->SetActorRotation(FRotator(yaw, 0, 0));
		}

		if (NighttimeDirectionalLight) {
			NighttimeDirectionalLight->SetActorRotation(FRotator(-30, yaw, 0));

			// Dim the moonlight if its between 6:00 and 18:00
			if (!KeepNightLightDuringDay) {
				auto Nightlight = NighttimeDirectionalLight->GetLightComponent();
				if (CurrentTimeOfDay.GetHours() >= 7 && CurrentTimeOfDay.GetHours() <= 16 && Nightlight->Intensity >
					0.001) {
					Nightlight->SetIntensity(Nightlight->Intensity - (NightLightIntensity * 0.01f));
					}
				// Otherwise brighten the moonlight to its original intensity
				else if (Nightlight->Intensity < NightLightIntensity) {
					Nightlight->SetIntensity(Nightlight->Intensity + (NightLightIntensity * 0.01f));
				}
			}
		}
	}
}

// Called when the game starts or when spawned

void ATimeOfDayController::MulticastSyncTimeOfDay_Implementation(const FTimespan& Timespan) {
	this->CurrentTimeOfDay = Timespan;
	UE_LOG(Mythic, Warning, TEXT("Time of day synced to %s"), *this->CurrentTimeOfDay.ToString());
}

void ATimeOfDayController::UpdateTimeOfDay() {
	const auto hour = this->CurrentTimeOfDay.GetHours();

	switch (this->GetDayTime(hour)) {
	case EDayTime::Morning:
		this->CurrentTimeOfDay += FTimespan::FromSeconds(21600 / (this->MorningLength / this->UpdateFrequency));
		break;
	case EDayTime::Afternoon:
		this->CurrentTimeOfDay += FTimespan::FromSeconds(21600 / (this->AfternoonLength / this->UpdateFrequency));
		break;
	case EDayTime::Evening:
		this->CurrentTimeOfDay += FTimespan::FromSeconds(21600 / (this->EveningLength / this->UpdateFrequency));
		break;
	default:
		this->CurrentTimeOfDay += FTimespan::FromSeconds(21600 / (this->NightLength / this->UpdateFrequency));
		break;
	}

	UpdateSunAndMoon();

	// if the hour has changed, broadcast the new hour
	const auto newHour = CurrentTimeOfDay.GetHours();
	if (hour != newHour) {
		// sync the time of day for all clients
		if (GetNetMode() < ENetMode::NM_Client) {
			this->MulticastSyncTimeOfDay(CurrentTimeOfDay);
		}
		
		// if this is the server, multicast the new hour to all clients
		if (GetNetMode() < ENetMode::NM_Client) {
			this->MulticastSyncTimeOfDay(CurrentTimeOfDay);
		}

		
		this->OnHourChanged.Broadcast(hour, newHour);

		// if the daytime has changed, broadcast the new daytime
		const auto previousDayTime = this->GetDayTime(hour);
		const auto newDayTime = this->GetDayTime(newHour);
		if (previousDayTime != newDayTime) {
			this->OnTimeOfDayChanged.Broadcast(previousDayTime, newDayTime);
		}
	}
}

EDayTime ATimeOfDayController::GetDayTime(int32 current_hour) {
	if (current_hour >= 0 && current_hour < 6) {
		return EDayTime::Night;
	}

	if (current_hour >= 6 && current_hour < 12) {
		return EDayTime::Morning;
	}

	if (current_hour >= 12 && current_hour < 18) {
		return EDayTime::Afternoon;
	}

	return EDayTime::Evening;
}

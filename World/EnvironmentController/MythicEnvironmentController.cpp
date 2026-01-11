// 


#include "MythicEnvironmentController.h"

#include "Mythic.h"
#include "MythicEnvironmentSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/LightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Engine/ExponentialHeightFog.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Materials/MaterialParameterCollection.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "System/MythicAssetManager.h"

// Sets default values
AMythicEnvironmentController::AMythicEnvironmentController() : FogComponent(nullptr), SkyAtmosphereComponent(nullptr),
                                                               NightLightIntensity(0),
                                                               DaytimeDirectionalLight(nullptr),
                                                               NighttimeDirectionalLight(nullptr), WeatherMPC(nullptr),
                                                               ExponentialHeightFog(nullptr),
                                                               SkyAtmosphere(nullptr),
                                                               GuaranteedTargetWeather(nullptr), WeatherTransition(),
                                                               CurrentWeather(),
                                                               CachedFogHeightFalloff(0), CachedFogDensity(0) {
    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;
    this->TimeUpdateFrequency = 0.01f;
    this->WeatherUpdateFrequency = 1.0f;

    // replicate to all clients
    bReplicates = true;
    bAlwaysRelevant = true;
}

void AMythicEnvironmentController::OnConstruction(const FTransform &Transform) {
    Super::OnConstruction(Transform);

    // Update the sun and moon
    UpdateLighting();

    // Update the time of day
    TimeTick();

    // Update the weather
    if (this->WeatherTypes.Num() > 0 && WeatherMPC) {
        WeatherTick();
    }
}

void AMythicEnvironmentController::SerializeCustomData(TArray<uint8> &OutCustomData) {

    FMemoryWriter MemWriter(OutCustomData);
    FObjectAndNameAsStringProxyArchive Ar(MemWriter, true);
    // Use standard serialization to bypass SaveGame filtering on nested Engine structs
    FWeatherCycleInfo::StaticStruct()->SerializeItem(Ar, &WeatherTransition, nullptr);
}

void AMythicEnvironmentController::DeserializeCustomData(const TArray<uint8> &InCustomData) {

    // Load the custom data buffer
    if (InCustomData.Num() > 0) {
        FMemoryReader MemReader(InCustomData);
        FObjectAndNameAsStringProxyArchive Ar(MemReader, true);

        // This overwrites the WeatherTransition that was loaded (imperfectly) by the standard serializer
        FWeatherCycleInfo::StaticStruct()->SerializeItem(Ar, &WeatherTransition, nullptr);
    }

    // Restore visuals post-load
    if (CurrentWeather) {
        ApplyWeatherVisuals(CurrentWeather);
    }
}

// Maps the current time of day to sun's yaw rotation (0 = 18:00, 90 = 00:00, 180 = 06:00, 270 = 12:00)
void AMythicEnvironmentController::BeginPlay() {
    Super::BeginPlay();

    if (ExponentialHeightFog) {
        FogComponent = Cast<UExponentialHeightFogComponent>(ExponentialHeightFog->GetComponent());
    }
    else {
        UE_LOG(Myth_Environment, Error, TEXT("ExponentialHeightFog is not set"));
    }

    if (SkyAtmosphere) {
        SkyAtmosphereComponent = Cast<USkyAtmosphereComponent>(SkyAtmosphere->GetComponent());
    }
    else {
        UE_LOG(Myth_Environment, Error, TEXT("SkyAtmosphere is not set"));
    }

    // Start a timer to update the time of day based on the morning, afternoon, evening, and night lengths
    GetWorldTimerManager().SetTimer(this->TimeOfDayTimerHandle, this, &AMythicEnvironmentController::TimeTick,
                                    this->TimeUpdateFrequency, true);
    // Start a timer to update the weather cycle
    if (this->WeatherTypes.Num() > 0 && WeatherMPC) {
        GetWorldTimerManager().SetTimer(this->WeatherTimerHandle, this, &AMythicEnvironmentController::WeatherTick,
                                        this->WeatherUpdateFrequency, true);
    }

    if (NighttimeDirectionalLight) {
        NightLightIntensity = NighttimeDirectionalLight->GetLightComponent()->Intensity;
    }

    UpdateLighting();

    // Apply the current weather visuals immediately (Handles Save/Load case where data exists but MPC is reset)
    if (CurrentWeather) {
        ApplyWeatherVisuals(CurrentWeather);
    }

    // if server, initialize the weather cycle. CycleWeather will sync the weather cycle to all clients
    if (GetLocalRole() == ROLE_Authority) {
        if (this->WeatherTypes.Num() > 0 && WeatherMPC && !CurrentWeather) {
            // Only cycle if we don't already have weather (e.g. New Game)
            CycleWeather();
        }
    }

    // Register self with the EnvironmentSubsystem
    if (auto EnvironmentSubsystem = GetGameInstance()->GetSubsystem<UMythicEnvironmentSubsystem>()) {
        EnvironmentSubsystem->SetEnvironmentController(this);
    }
    else {
        UE_LOG(Myth_Environment, Error, TEXT("EnvironmentSubsystem is not set"));
    }
}

float AMythicEnvironmentController::GetSunPositionForCurrentTime() const {
    auto todaysSeconds = static_cast<int>(this->Time.GetTotalMilliseconds()) % 86400000;

    UE::Math::TVector2<float> timeRange(0, 86400000);
    UE::Math::TVector2<float> sunRange(0 + 90, 359.9 + 90);
    float sunPos = FMath::GetMappedRangeValueClamped(timeRange, sunRange, todaysSeconds);

    return sunPos;
}


bool AMythicEnvironmentController::isCurrentWeatherExpired() const {
    // if current weather exists, and it has exceeded its lifetime, transition to the next weather.
    if (!this->CurrentWeather) {
        UE_LOG(Myth_Environment, Warning, TEXT("No current weather"));
        return true;
    }

    // Check if the current weather has exceeded its lifetime
    const auto CurrentWeatherLifetime = this->WeatherTransition.TransitionLength;
    const auto TimeSinceWeatherChange = this->Time - this->WeatherChangedAt;

    return TimeSinceWeatherChange.GetTotalMinutes() >= CurrentWeatherLifetime;
}

UWeatherType *AMythicEnvironmentController::GetWeatherTypeByTag(FGameplayTag Tag) const {
    for (auto WeatherType : WeatherTypes) {
        if (WeatherType->Tag == Tag) {
            return WeatherType;
        }
    }

    return nullptr;
}

// Get next weather type to reach the target weather.
// Processes the pre and post weather types to determine the next weather type to transition to, to eventually reach the target weather
UWeatherType *AMythicEnvironmentController::GetNextWeatherTypeToReachTargetWeather(UWeatherType *FromWeather, UWeatherType *TargetWeather) {
    if (!FromWeather || !TargetWeather) {
        return nullptr;
    }

    // Check if the target weather is already reachable from the current weather
    if (FromWeather->CanTransitionTo(*TargetWeather)) {
        return TargetWeather;
    }

    // Create a set to store visited weather types to prevent cycles
    TSet<FGameplayTag> Visited;
    Visited.Add(FromWeather->Tag);

    // Create a queue for BFS (Breadth-First Search) to find the shortest path to the target weather
    TQueue<UWeatherType *> Queue;
    Queue.Enqueue(FromWeather);

    // Store the path to reach each weather type
    TMap<UWeatherType *, UWeatherType *> PredecessorMap;

    while (!Queue.IsEmpty()) {
        UWeatherType *current_weather;
        Queue.Dequeue(current_weather);

        // Check all possible transitions from the current weather
        for (auto Weather : WeatherTypes) {
            // If we've already visited this weather type, skip it
            if (Visited.Contains(Weather->Tag)) {
                continue;
            }

            // Check if the current weather can transition to this weather type
            if (current_weather->CanTransitionTo(*Weather)) {
                // If this weather type is the target, we found the path
                if (Weather->Tag == TargetWeather->Tag) {
                    PredecessorMap.Add(Weather, current_weather);
                    UWeatherType *NextWeather = Weather;

                    // Traverse back to find the first step towards the target weather
                    while (PredecessorMap.Contains(NextWeather) && PredecessorMap[NextWeather] != FromWeather) {
                        NextWeather = PredecessorMap[NextWeather];
                    }

                    return NextWeather;
                }

                // Otherwise, enqueue the weather type and mark it as visited
                Queue.Enqueue(Weather);
                Visited.Add(Weather->Tag);
                PredecessorMap.Add(Weather, current_weather);
            }
        }
    }

    // If we exit the loop without finding a path, return nullptr
    return nullptr;
}

// Receives a DateTime and overrides the current time of day
void AMythicEnvironmentController::SetTime(const FDateTime &DateTime) {
    checkf(GetLocalRole() == ROLE_Authority, TEXT("Only the server can set the time"));

    // Set the new hour, minute, second, and total days
    auto NewHour = DateTime.GetHour();
    auto NewMinute = DateTime.GetMinute();
    auto NewSeconds = DateTime.GetSecond();
    auto NewDays = DateTime.GetDayOfYear() * DateTime.GetYear();

    auto NewTime = FTimespan(NewDays, NewHour, NewMinute, NewSeconds);

    // Multicast the new time to all clients
    this->MulticastSyncGameWorldTimer(NewTime, this->Time);
    this->Time = NewTime;
}

// AddTime -> SetTime -> MulticastSyncGameWorldTimer
void AMythicEnvironmentController::AddTime(const FTimespan &ByTime) {
    // Server only
    checkf(GetLocalRole() == ROLE_Authority, TEXT("Only the server can add time"));

    // Add the time to the current time of day
    auto currentDateTime = this->GetDateTime();
    auto newDateTime = currentDateTime + ByTime;

    // Set the new time
    this->SetTime(newDateTime);
}

void AMythicEnvironmentController::SetTimeUpdateFrequency(float Frequency) {
    // Server only
    checkf(GetLocalRole() == ROLE_Authority, TEXT("Only the server can set the time update frequency"));

    // Reset the timer with the new frequency
    this->TimeUpdateFrequency = Frequency;
    GetWorldTimerManager().SetTimer(this->TimeOfDayTimerHandle, this, &AMythicEnvironmentController::TimeTick,
                                    this->TimeUpdateFrequency, true);
}

void AMythicEnvironmentController::PauseTime() const {
    // Server only
    checkf(GetLocalRole() == ROLE_Authority, TEXT("Only the server can pause time"));

    // Pause the time of day timer
    GetWorldTimerManager().PauseTimer(this->TimeOfDayTimerHandle);
}

void AMythicEnvironmentController::ResumeTime() const {
    // Server only
    checkf(GetLocalRole() == ROLE_Authority, TEXT("Only the server can resume time"));

    // Resume the time of day timer
    GetWorldTimerManager().UnPauseTimer(this->TimeOfDayTimerHandle);
}

void AMythicEnvironmentController::UpdateLighting() const {
    if (bUpdateDirectionalLights) {
        auto yaw = this->GetSunPositionForCurrentTime();
        if (DaytimeDirectionalLight) {
            DaytimeDirectionalLight->SetActorRotation(FRotator(yaw, 0, 0));
        }

        if (NighttimeDirectionalLight) {
            NighttimeDirectionalLight->SetActorRotation(FRotator(-30, yaw, 0));

            // Dim the moonlight if its between 6:00 and 18:00
            auto Nightlight = NighttimeDirectionalLight->GetLightComponent();
            if (Time.GetHours() >= 7 && Time.GetHours() <= 16 && Nightlight->Intensity >
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

void AMythicEnvironmentController::MulticastSyncWindTarget_Implementation(const FLinearColor &WindTargetPosition) {
    // Set the wind target position
    if (WeatherMPC) {
        UKismetMaterialLibrary::SetVectorParameterValue(this, WeatherMPC, this->WindTargetParameterName, WindTargetPosition);
    }
}

// Called when the game starts or when spawned
void AMythicEnvironmentController::MulticastSyncGameWorldTimer_Implementation(const FTimespan &NewTimespan, const FTimespan &OldTimespan) {
    this->Time = NewTimespan;
    UE_LOG(Myth_Environment, Warning, TEXT("Time of day synced to %s"), *this->Time.ToString());

    /// Broadcast Time Events ---------------------------------------------------------------
    // Broadcast Hour and DayTime Change
    auto NewHour = this->Time.GetHours();
    auto PrevHour = OldTimespan.GetHours();
    if (PrevHour != NewHour) {
        this->HourChangeDelegate.Broadcast(PrevHour, NewHour);
        this->DayTimeChangeDelegate.Broadcast(HourAsDayTime(PrevHour), HourAsDayTime(NewHour));
    }

    // Broadcast Day Change
    auto NewDay = GetDayOfMonth(this->Time);
    auto PrevDay = GetDayOfMonth(OldTimespan);
    if (PrevDay != NewDay) {
        this->DayChangeDelegate.Broadcast(PrevDay, NewDay);
    }

    // Broadcast Month Change along with the Season
    auto NewMonth = GetMonthOfYear(this->Time);
    auto PrevMonth = GetMonthOfYear(OldTimespan);
    if (PrevMonth != NewMonth) {
        auto oldSeason = MonthAsSeason(PrevMonth);
        auto newSeason = MonthAsSeason(NewMonth);
        this->MonthChangeDelegate.Broadcast(PrevMonth, NewMonth, oldSeason, newSeason);
    }

    // Broadcast Year Change
    auto NewYear = GetYear(this->Time);
    auto PrevYear = GetYear(OldTimespan);
    if (PrevYear != NewYear) {
        this->YearChangeDelegate.Broadcast(PrevYear, NewYear);
    }
}

// The brain of the time system
void AMythicEnvironmentController::TimeTick() {
    const auto PreviousTime = this->Time;
    const auto PreviousHour = PreviousTime.GetHours();

    switch (HourAsDayTime(PreviousHour)) {
    case EDayTime::Morning:
    case EDayTime::Afternoon:
    case EDayTime::Evening:
        this->Time += FTimespan::FromSeconds(21600 / (this->DayLength / this->TimeUpdateFrequency));
        break;
    default:
        this->Time += FTimespan::FromSeconds(21600 / (this->NightLength / this->TimeUpdateFrequency));
        break;
    }

    UpdateLighting();

    // SERVER-ONLY: Every hour, sync the time of day for all clients
    const auto NewHour = Time.GetHours();
    if (PreviousHour != NewHour && GetLocalRole() == ROLE_Authority) {
        // Server -> Multicast -> Broadcast events on clients and server
        this->MulticastSyncGameWorldTimer(Time, PreviousTime);

        // Change the sky atmosphere absorption at midnight
        if (NewHour == 0) {
            if (SkyAtmosphereComponent) {
                FLinearColor Absorption = FLinearColor(FMath::RandRange(0.0f, 1.0f), FMath::RandRange(0.0f, 1.0f),
                                                       FMath::RandRange(0.0f, 1.0f));
                float AbsorptionScalar = FMath::RandRange(0.001f, 0.005f);
                this->MulticastSyncSkyAtmosphereAbsorption(Absorption, AbsorptionScalar);
            }
        }
    }
}

void AMythicEnvironmentController::MulticastSyncSkyAtmosphereAbsorption_Implementation(FLinearColor Absorption, float AbsorptionScalar) {
    if (SkyAtmosphereComponent) {
        UE_LOG(Myth_Environment, Warning, TEXT("SkyAtmosphere Absorption synced to %s"), *Absorption.ToString());
        SkyAtmosphereComponent->SetOtherAbsorption(Absorption);
        SkyAtmosphereComponent->SetOtherAbsorptionScale(AbsorptionScalar);
    }
}

// The brain of the weather system
void AMythicEnvironmentController::WeatherTick() {
    // If there is a weather transition in progress, update the weather transition
    // Check if the soft pointer is valid or points to something
    if (!this->WeatherTransition.TransitionToWeather.IsNull()) {
        UWeatherType *TargetWeather = this->WeatherTransition.TransitionToWeather.Get();
        if (TargetWeather) {
            UE_LOG(Myth_Environment, Warning, TEXT("Weather transition in progress to %s"), *TargetWeather->GetName());
            HandleWeatherTransition();
        }
    }
    // SERVER-ONLY: Weather is cycled only through the server - which then modifies the weather cycle and syncs it to all clients
    else if (GetLocalRole() == ROLE_Authority && isCurrentWeatherExpired()) {
        UE_LOG(Myth_Environment, Warning, TEXT("No weather transition in progress"));
        CycleWeather();
    }
}

void AMythicEnvironmentController::HandleWeatherTransition() {
    // Transition start time should be in the past

    // Lerp the weather attributes based on the TransitionStartedAt timespan and the TransitionDurationInMins
    auto TransitionProgress = 1.0f;

    if (this->WeatherTransition.bSetInstantly) {
        // Instant
    }
    else {
        const auto TransitionTime = this->Time - this->TransitionStartedAt;
        TransitionProgress = TransitionTime.GetTotalMinutes() / TransitionDurationInMins;
    }

    // Lerp the scalar attributes
    for (int i = 0; i < TransitionFromScalarValues.Num(); i++) {
        auto MPC_Value = TransitionFromScalarValues[i];
        auto TargetValue = this->WeatherTransition.TransitionToScalarValues[i];
        auto LerpValue = FMath::Lerp(MPC_Value.DefaultValue, TargetValue.DefaultValue, TransitionProgress);

        UKismetMaterialLibrary::SetScalarParameterValue(this, WeatherMPC, TransitionFromScalarValues[i].ParameterName, LerpValue);
    }

    // Lerp the vector attributes
    for (int i = 0; i < TransitionFromVectorValues.Num(); i++) {
        auto MPC_Value = TransitionFromVectorValues[i];
        auto TargetValue = this->WeatherTransition.TransitionToVectorValues[i];
        auto LerpValue = FMath::Lerp(MPC_Value.DefaultValue, TargetValue.DefaultValue, TransitionProgress);

        UKismetMaterialLibrary::SetVectorParameterValue(this, WeatherMPC, TransitionFromVectorValues[i].ParameterName, LerpValue);
    }

    // Lerp the fog density
    if (FogComponent) {
        auto LerpFogDensity = FMath::Lerp(this->CachedFogDensity, this->WeatherTransition.FogDensity, TransitionProgress);
        FogComponent->SetFogDensity(LerpFogDensity);

        // Lerp the fog height falloff
        auto LerpFogHeightFalloff = FMath::Lerp(this->CachedFogHeightFalloff, this->WeatherTransition.FogHeightFalloff, TransitionProgress);
        FogComponent->SetFogHeightFalloff(LerpFogHeightFalloff);
    }

    // If the transition is complete, set the current weather to the target weather
    UWeatherType *TargetWeather = this->WeatherTransition.TransitionToWeather.Get();
    if (TransitionProgress >= 1.0f || TargetWeather == this->CurrentWeather || Time < TransitionStartedAt) {
        auto NewWeatherTag = TargetWeather ? TargetWeather->Tag : FGameplayTag();
        auto OldWeatherTag = this->CurrentWeather ? this->CurrentWeather->Tag : FGameplayTag();

        this->CurrentWeather = TargetWeather;
        this->WeatherTransition.TransitionToWeather = nullptr;
        this->WeatherChangedAt = Time;
        if (this->CurrentWeather == this->GuaranteedTargetWeather) {
            // We have reached our target weather type. Emit the event
            this->GuaranteedTargetWeather = nullptr;
            this->TargetWeatherReachedDelegate.Broadcast(this->CurrentWeather->Tag);
        }

        // Broadcast the weather change event
        this->WeatherChangeDelegate.Broadcast(NewWeatherTag, OldWeatherTag);
        UE_LOG(Myth_Environment, Warning, TEXT("EnvironmentController: Transition complete. New weather: %s"), *this->CurrentWeather->GetName());
    }
}

void AMythicEnvironmentController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION(AMythicEnvironmentController, Time, COND_InitialOnly);
    DOREPLIFETIME(AMythicEnvironmentController, CurrentWeather);
    DOREPLIFETIME(AMythicEnvironmentController, WeatherTransition);
    DOREPLIFETIME(AMythicEnvironmentController, GuaranteedTargetWeather);
}

void AMythicEnvironmentController::OnRep_Time() {
    // Initial sync of time for client
    // We can also trigger an immediate lighting update
    UpdateLighting();
    UE_LOG(Myth_Environment, Log, TEXT("OnRep_Time: Time synced to %s"), *Time.ToString());
}

void AMythicEnvironmentController::OnRep_CurrentWeather(UWeatherType *PreviousWeather) {
    UE_LOG(Myth_Environment, Log, TEXT("OnRep_CurrentWeather: %s -> %s"),
           PreviousWeather ? *PreviousWeather->GetName() : TEXT("None"),
           CurrentWeather ? *CurrentWeather->GetName() : TEXT("None"));

    // Ensure visuals are up to date
    if (CurrentWeather) {
        ApplyWeatherVisuals(CurrentWeather);
    }

    // Broadcast change
    FGameplayTag OldTag = PreviousWeather ? PreviousWeather->Tag : FGameplayTag::EmptyTag;
    FGameplayTag NewTag = CurrentWeather ? CurrentWeather->Tag : FGameplayTag::EmptyTag;
    this->WeatherChangeDelegate.Broadcast(NewTag, OldTag);
}

// Apply authoritative visuals from the WeatherTransition struct (which holds server-synced instance values)
void AMythicEnvironmentController::ApplyWeatherVisuals(const UWeatherType *Weather) {
    if (!Weather || !WeatherMPC) {
        return;
    }

    // Instead of re-rolling random values from the Weather Asset (which causes desync),
    // CHECK FOR DATA VALIDITY
    // If we have authoritative data in the transition struct, use it.
    // This persist even after the transition is finished (as we only clear the pointer, not the arrays).
    const bool bHasAuthoritativeData = WeatherTransition.TransitionToScalarValues.Num() > 0;

    // Restore local helper state from valid transition data
    if (WeatherTransition.StartTime.GetTicks() > 0) {
        this->TransitionStartedAt = WeatherTransition.StartTime;
    }

    // we apply the specific values stored in WeatherTransition.
    // These values are calculated by the Server, Replicated, and Saved.

    // Apply Scalar Values
    if (bHasAuthoritativeData) {
        for (const auto &Param : WeatherTransition.TransitionToScalarValues) {
            // Only apply if this parameter belongs to the current weather type context usually,
            // but applying all cached values is safer to ensure state matches server.
            UKismetMaterialLibrary::SetScalarParameterValue(this, WeatherMPC, Param.ParameterName, Param.DefaultValue);
        }

        // Apply Vector Values
        for (const auto &Param : WeatherTransition.TransitionToVectorValues) {
            UKismetMaterialLibrary::SetVectorParameterValue(this, WeatherMPC, Param.ParameterName, Param.DefaultValue);
        }

        // Apply Fog
        if (FogComponent) {
            FogComponent->SetFogDensity(WeatherTransition.FogDensity);
            FogComponent->SetFogHeightFalloff(WeatherTransition.FogHeightFalloff);
        }
    }
}

void AMythicEnvironmentController::OnRep_WeatherTransition() {
    // Async load the weather type if not already loaded
    UMythicAssetManager::LoadAsync(this, WeatherTransition.TransitionToWeather,
                                   [this](UWeatherType *TargetWeather) {
                                       UE_LOG(Myth_Environment, Log, TEXT("OnRep_WeatherTransition: Transition to %s"),
                                              TargetWeather ? *TargetWeather->GetName() : TEXT("None"));

                                       // Cache "From" values to interpolate from current visual state
                                       TransitionFromScalarValues.Empty();
                                       TransitionFromVectorValues.Empty();

                                       // Iterate over TargetWeather attributes to cache starting values from MPC
                                       if (TargetWeather) {
                                           for (const auto &ScalarAttr : TargetWeather->ScalarAttributes) {
                                               FCollectionScalarParameter Val;
                                               Val.ParameterName = ScalarAttr.Name;
                                               Val.DefaultValue = UKismetMaterialLibrary::GetScalarParameterValue(this, WeatherMPC, ScalarAttr.Name);
                                               TransitionFromScalarValues.Add(Val);
                                           }

                                           for (const auto &VectorAttr : TargetWeather->VectorAttributes) {
                                               FCollectionVectorParameter Val;
                                               Val.ParameterName = VectorAttr.Name;
                                               Val.DefaultValue = UKismetMaterialLibrary::GetVectorParameterValue(this, WeatherMPC, VectorAttr.Name);
                                               TransitionFromVectorValues.Add(Val);
                                           }
                                       }

                                       if (FogComponent) {
                                           this->CachedFogHeightFalloff = FogComponent->FogHeightFalloff;
                                           this->CachedFogDensity = FogComponent->FogDensity;
                                       }

                                       // Set the transition locally
                                       this->TransitionStartedAt = WeatherTransition.StartTime;

                                       // Trigger delegate
                                       FGameplayTag FromTag = CurrentWeather ? CurrentWeather->Tag : FGameplayTag::EmptyTag;
                                       FGameplayTag ToTag = TargetWeather ? TargetWeather->Tag : FGameplayTag::EmptyTag;
                                       this->WeatherTransitionDelegate.Broadcast(ToTag, FromTag, WeatherTransition.TransitionLength);
                                   });
}

void AMythicEnvironmentController::CycleWeather() {
    checkf(GetLocalRole() == ROLE_Authority, TEXT("Only the server can cycle weather"));

    // If GuaranteedTargetWeather is set, get the next weather type to reach the target weather
    UWeatherType *SelectedWeather;
    if (this->GuaranteedTargetWeather) {
        if (auto NextWeather = this->GetNextWeatherTypeToReachTargetWeather(this->CurrentWeather, this->GuaranteedTargetWeather)) {
            SelectedWeather = NextWeather;
        }
        else {
            // No gradual transition possible, set the target weather as the next weather
            SelectedWeather = this->GuaranteedTargetWeather;
            this->GuaranteedTargetWeather = nullptr;
        }
    }
    else {
        // Otherwise get a random weather type
        SelectedWeather = WeatherTypes[FMath::RandRange(0, WeatherTypes.Num() - 1)];
    }

    if (!SelectedWeather) {
        UE_LOG(Myth_Environment, Warning, TEXT("No weather type selected"));
        return;
    }

    // if we can't transition to the selected weather type
    if (this->CurrentWeather && !this->CurrentWeather->CanTransitionTo(*SelectedWeather)) {
        // If the current month is not in the season of the selected weather, try to find another weather type before giving up
        if (!SelectedWeather->MonthRange.Contains(GetMonthOfYear(this->Time))) {
            this->CycleWeather();
            return;
        }

        // Extend the current weather lifetime
        return;
    }

    // Create the new weather cycle with the StartTime set to current Time
    this->WeatherTransition = FWeatherCycleInfo(SelectedWeather, this->Time);

    // On Server, manually call OnRep to trigger local events/caching
    OnRep_WeatherTransition();
}

void AMythicEnvironmentController::SetWeatherTransition(const FWeatherCycleInfo &NewWeatherCycle) {
    // This function was previously used by Multicast, now mainly internal or for instant sets
    // We can deprecate it or redirect.
    // For now, let's just ensure it updates the struct
    this->WeatherTransition = NewWeatherCycle;
    OnRep_WeatherTransition();
}

// REMOVED MulticastSyncWeather_Implementation as it is replaced by OnRep_WeatherTransition
// Keeping empty shell if header still declares it, or removing if I removed from header. 
// I commented out in header, so I should remove here.

// ...

// WEATHER
void AMythicEnvironmentController::SetGuaranteedTargetWeather(FGameplayTag TargetWeather) {
    checkf(GetLocalRole() == ROLE_Authority, TEXT("Only the server can set the target weather"));

    this->GuaranteedTargetWeather = GetWeatherTypeByTag(TargetWeather);
}

void AMythicEnvironmentController::PauseWeather() const {
    checkf(GetLocalRole() == ROLE_Authority, TEXT("Only the server can pause the weather"));

    GetWorld()->GetTimerManager().PauseTimer(this->WeatherTimerHandle);
}

void AMythicEnvironmentController::ResumeWeather() const {
    checkf(GetLocalRole() == ROLE_Authority, TEXT("Only the server can resume the weather"));

    GetWorld()->GetTimerManager().UnPauseTimer(this->WeatherTimerHandle);
}

void AMythicEnvironmentController::SetWindTargetPosition(const FLinearColor &TargetPosition) {
    checkf(GetLocalRole() == ROLE_Authority, TEXT("Only the server can set the wind target position"));

    if (!WeatherMPC) {
        UE_LOG(Myth_Environment, Error, TEXT("Cannot set wind TargetPosition without a WeatherMPC"));
        return;
    }

    this->MulticastSyncWindTarget(TargetPosition);
}

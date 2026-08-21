

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
#include "Engine/GameInstance.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/CausalFabric/CausalFabric.h"
#include "World/MythicTags_World.h"

namespace {
    FGameplayTag SeasonToEventTag(ESeason Season) {
        switch (Season) {
        case Spring:
            return WORLD_EVENT_SEASON_SPRING;
        case Summer:
            return WORLD_EVENT_SEASON_SUMMER;
        case Autumn:
            return WORLD_EVENT_SEASON_AUTUMN;
        case Winter:
            return WORLD_EVENT_SEASON_WINTER;
        default:
            return FGameplayTag();
        }
    }
}

AMythicEnvironmentController::AMythicEnvironmentController() : FogComponent(nullptr), SkyAtmosphereComponent(nullptr),
                                                               NightLightIntensity(0),
                                                               DaytimeDirectionalLight(nullptr),
                                                               NighttimeDirectionalLight(nullptr), WeatherMPC(nullptr),
                                                               ExponentialHeightFog(nullptr),
                                                               SkyAtmosphere(nullptr),
                                                               GuaranteedTargetWeather(nullptr), WeatherTransition(),
                                                               CurrentWeather(),
                                                               CachedFogHeightFalloff(0), CachedFogDensity(0) {
    PrimaryActorTick.bCanEverTick = false;
    this->TimeUpdateFrequency = 0.05f;
    this->WeatherUpdateFrequency = 1.0f;

    bReplicates = true;
    bAlwaysRelevant = true;
}

void AMythicEnvironmentController::OnConstruction(const FTransform &Transform) {
    Super::OnConstruction(Transform);

    UpdateLighting();

    TimeTick();

    if (this->WeatherTypes.Num() > 0 && WeatherMPC) {
        WeatherTick();
    }
}

void AMythicEnvironmentController::SerializeCustomData(TArray<uint8> &OutCustomData) {
    FMemoryWriter MemWriter(OutCustomData);
    FObjectAndNameAsStringProxyArchive Ar(MemWriter, true);
    FWeatherCycleInfo::StaticStruct()->SerializeItem(Ar, &WeatherTransition, nullptr);
}

void AMythicEnvironmentController::DeserializeCustomData(const TArray<uint8> &InCustomData) {
    if (InCustomData.Num() > 0) {
        FMemoryReader MemReader(InCustomData);
        FObjectAndNameAsStringProxyArchive Ar(MemReader, true);

        FWeatherCycleInfo::StaticStruct()->SerializeItem(Ar, &WeatherTransition, nullptr);
    }

    if (CurrentWeather) {
        ApplyWeatherVisuals(CurrentWeather);
    }
}

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

    GetWorldTimerManager().SetTimer(this->TimeOfDayTimerHandle, this, &AMythicEnvironmentController::TimeTick,
                                    this->TimeUpdateFrequency, true);
    if (this->WeatherTypes.Num() > 0 && WeatherMPC) {
        GetWorldTimerManager().SetTimer(this->WeatherTimerHandle, this, &AMythicEnvironmentController::WeatherTick,
                                        this->WeatherUpdateFrequency, true);
    }

    if (NighttimeDirectionalLight) {
        NightLightIntensity = NighttimeDirectionalLight->GetLightComponent()->Intensity;
    }

    UpdateLighting();

    if (CurrentWeather) {
        ApplyWeatherVisuals(CurrentWeather);
    }

    if (GetLocalRole() == ROLE_Authority) {
        if (this->WeatherTypes.Num() > 0 && WeatherMPC && !CurrentWeather) {
            CycleWeather();
        }
    }

    if (auto EnvironmentSubsystem = GetGameInstance()->GetSubsystem<UMythicEnvironmentSubsystem>()) {
        EnvironmentSubsystem->SetEnvironmentController(this);
    }
    else {
        UE_LOG(Myth_Environment, Error, TEXT("EnvironmentSubsystem is not set"));
    }
}

float AMythicEnvironmentController::GetSunPositionForCurrentTime() const {
    const float todaysSeconds = static_cast<float>(FMath::Fmod(this->Time.GetTotalMilliseconds(), 86400000.0));

    UE::Math::TVector2<float> timeRange(0, 86400000);
    UE::Math::TVector2<float> sunRange(90.0f, 449.9f);
    float sunPos = FMath::GetMappedRangeValueClamped(timeRange, sunRange, todaysSeconds);

    return sunPos;
}


bool AMythicEnvironmentController::isCurrentWeatherExpired() const {
    if (!this->CurrentWeather) {
        UE_LOG(Myth_Environment, Warning, TEXT("No current weather"));
        return true;
    }

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

UWeatherType *AMythicEnvironmentController::GetNextWeatherTypeToReachTargetWeather(UWeatherType *FromWeather, UWeatherType *TargetWeather) {
    if (!FromWeather || !TargetWeather) {
        return nullptr;
    }

    if (FromWeather->CanTransitionTo(*TargetWeather)) {
        return TargetWeather;
    }

    TSet<FGameplayTag> Visited;
    Visited.Add(FromWeather->Tag);

    TQueue<UWeatherType *> Queue;
    Queue.Enqueue(FromWeather);

    TMap<UWeatherType *, UWeatherType *> PredecessorMap;

    while (!Queue.IsEmpty()) {
        UWeatherType *current_weather;
        Queue.Dequeue(current_weather);

        for (auto Weather : WeatherTypes) {
            if (Visited.Contains(Weather->Tag)) {
                continue;
            }

            if (current_weather->CanTransitionTo(*Weather)) {
                if (Weather->Tag == TargetWeather->Tag) {
                    PredecessorMap.Add(Weather, current_weather);
                    UWeatherType *NextWeather = Weather;

                    while (PredecessorMap.Contains(NextWeather) && PredecessorMap[NextWeather] != FromWeather) {
                        NextWeather = PredecessorMap[NextWeather];
                    }

                    return NextWeather;
                }

                Queue.Enqueue(Weather);
                Visited.Add(Weather->Tag);
                PredecessorMap.Add(Weather, current_weather);
            }
        }
    }

    return nullptr;
}

void AMythicEnvironmentController::SetTime(const FDateTime &DateTime) {
    checkf(GetLocalRole() == ROLE_Authority, TEXT("Only the server can set the time"));

    auto NewHour = DateTime.GetHour();
    auto NewMinute = DateTime.GetMinute();
    auto NewSeconds = DateTime.GetSecond();
    const int32 TargetYear = DateTime.GetYear();
    const int32 ShownMonth = DateTime.GetMonth();
    const int32 ShownDay = DateTime.GetDay();
    const int32 RawMonth = (ShownMonth == 12) ? 0 : ShownMonth;
    const int32 RawDay = (ShownDay == 30) ? 0 : ShownDay;
    const int32 NewDays = (TargetYear - 1) * 360 + RawMonth * 30 + RawDay;

    auto NewTime = FTimespan(NewDays, NewHour, NewMinute, NewSeconds);

    this->MulticastSyncGameWorldTimer(NewTime, this->Time);
    this->Time = NewTime;
}

void AMythicEnvironmentController::AddTime(const FTimespan &ByTime) {
    checkf(GetLocalRole() == ROLE_Authority, TEXT("Only the server can add time"));

    auto currentDateTime = this->GetDateTime();
    auto newDateTime = currentDateTime + ByTime;

    this->SetTime(newDateTime);
}

void AMythicEnvironmentController::SetTimeUpdateFrequency(float Frequency) {
    checkf(GetLocalRole() == ROLE_Authority, TEXT("Only the server can set the time update frequency"));

    this->TimeUpdateFrequency = Frequency;
    GetWorldTimerManager().SetTimer(this->TimeOfDayTimerHandle, this, &AMythicEnvironmentController::TimeTick,
                                    this->TimeUpdateFrequency, true);
}

void AMythicEnvironmentController::PauseTime() const {
    checkf(GetLocalRole() == ROLE_Authority, TEXT("Only the server can pause time"));

    GetWorldTimerManager().PauseTimer(this->TimeOfDayTimerHandle);
}

void AMythicEnvironmentController::ResumeTime() const {
    checkf(GetLocalRole() == ROLE_Authority, TEXT("Only the server can resume time"));

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

            auto Nightlight = NighttimeDirectionalLight->GetLightComponent();
            const float Step = NightLightIntensity * 0.01f;
            const bool bDaytime = Time.GetHours() >= 7 && Time.GetHours() <= 16;
            const float Target = bDaytime ? 0.0f : NightLightIntensity;
            if (!FMath::IsNearlyEqual(Nightlight->Intensity, Target, KINDA_SMALL_NUMBER)) {
                const float Next = bDaytime ? Nightlight->Intensity - Step : Nightlight->Intensity + Step;
                Nightlight->SetIntensity(FMath::Clamp(Next, 0.0f, NightLightIntensity));
            }
        }
    }
}

void AMythicEnvironmentController::MulticastSyncWindTarget_Implementation(const FLinearColor &WindTargetPosition) {
    if (WeatherMPC) {
        UKismetMaterialLibrary::SetVectorParameterValue(this, WeatherMPC, this->WindTargetParameterName, WindTargetPosition);
    }
}

void AMythicEnvironmentController::MulticastSyncGameWorldTimer_Implementation(const FTimespan &NewTimespan, const FTimespan &OldTimespan) {
    this->Time = NewTimespan;
    UE_LOG(Myth_Environment, Log, TEXT("Time of day synced to %s"), *this->Time.ToString());

    auto NewHour = this->Time.GetHours();
    auto PrevHour = OldTimespan.GetHours();
    if (PrevHour != NewHour) {
        this->HourChangeDelegate.Broadcast(PrevHour, NewHour);
        this->DayTimeChangeDelegate.Broadcast(HourAsDayTime(PrevHour), HourAsDayTime(NewHour));
    }

    auto NewDay = GetDayOfMonth(this->Time);
    auto PrevDay = GetDayOfMonth(OldTimespan);
    if (PrevDay != NewDay) {
        this->DayChangeDelegate.Broadcast(PrevDay, NewDay);
    }

    auto NewMonth = GetMonthOfYear(this->Time);
    auto PrevMonth = GetMonthOfYear(OldTimespan);
    if (PrevMonth != NewMonth) {
        auto oldSeason = MonthAsSeason(PrevMonth);
        auto newSeason = MonthAsSeason(NewMonth);
        this->MonthChangeDelegate.Broadcast(PrevMonth, NewMonth, oldSeason, newSeason);

        if (oldSeason != newSeason && HasAuthority() && SeasonNewsSignificance > 0.0f) {
            if (const UGameInstance *GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr) {
                if (UMythicLivingWorldSubsystem *LWS = GI->GetSubsystem<UMythicLivingWorldSubsystem>()) {
                    const FGameplayTag SeasonTag = SeasonToEventTag(newSeason);
                    if (SeasonTag.IsValid()) {
                        FMythicWorldEvent SeasonEvent;
                        SeasonEvent.EventTag = SeasonTag;
                        SeasonEvent.CategoryFlags = EMythicEventCategory::Environment;
                        SeasonEvent.Significance = SeasonNewsSignificance;
                        LWS->SubmitWorldEvent(SeasonEvent);
                    }
                }
            }
        }
    }

    auto NewYear = GetYear(this->Time);
    auto PrevYear = GetYear(OldTimespan);
    if (PrevYear != NewYear) {
        this->YearChangeDelegate.Broadcast(PrevYear, NewYear);
    }
}

void AMythicEnvironmentController::TimeTick() {
    const auto PreviousTime = this->Time;
    const auto PreviousHour = PreviousTime.GetHours();

    switch (HourAsDayTime(PreviousHour)) {
    case Morning:
    case Afternoon:
    case Evening:
        this->Time += FTimespan::FromSeconds(21600 / (this->DayLength / this->TimeUpdateFrequency));
        break;
    default:
        this->Time += FTimespan::FromSeconds(21600 / (this->NightLength / this->TimeUpdateFrequency));
        break;
    }

    UpdateLighting();

    const auto NewHour = Time.GetHours();
    if (PreviousHour != NewHour && GetLocalRole() == ROLE_Authority) {
        this->MulticastSyncGameWorldTimer(Time, PreviousTime);

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

void AMythicEnvironmentController::WeatherTick() {
    if (!this->WeatherTransition.TransitionToWeather.IsNull()) {
        UWeatherType *TargetWeather = this->WeatherTransition.TransitionToWeather.Get();
        if (TargetWeather) {
            UE_LOG(Myth_Environment, Verbose, TEXT("Weather transition in progress to %s"), *TargetWeather->GetName());
            HandleWeatherTransition();
        }
    }
    else if (GetLocalRole() == ROLE_Authority && isCurrentWeatherExpired()) {
        UE_LOG(Myth_Environment, Log, TEXT("Weather expired; cycling to the next one"));
        CycleWeather();
    }
}

void AMythicEnvironmentController::HandleWeatherTransition() {
    const float TransitionProgress = ComputeWeatherTransitionProgress(
        this->WeatherTransition.bSetInstantly,
        (this->Time - this->TransitionStartedAt).GetTotalMinutes(),
        TransitionDurationInMins);

    for (int i = 0; i < TransitionFromScalarValues.Num(); i++) {
        auto MPC_Value = TransitionFromScalarValues[i];
        auto TargetValue = this->WeatherTransition.TransitionToScalarValues[i];
        auto LerpValue = FMath::Lerp(MPC_Value.DefaultValue, TargetValue.DefaultValue, TransitionProgress);

        UKismetMaterialLibrary::SetScalarParameterValue(this, WeatherMPC, TransitionFromScalarValues[i].ParameterName, LerpValue);
    }

    for (int i = 0; i < TransitionFromVectorValues.Num(); i++) {
        auto MPC_Value = TransitionFromVectorValues[i];
        auto TargetValue = this->WeatherTransition.TransitionToVectorValues[i];
        auto LerpValue = FMath::Lerp(MPC_Value.DefaultValue, TargetValue.DefaultValue, TransitionProgress);

        UKismetMaterialLibrary::SetVectorParameterValue(this, WeatherMPC, TransitionFromVectorValues[i].ParameterName, LerpValue);
    }

    if (FogComponent) {
        auto LerpFogDensity = FMath::Lerp(this->CachedFogDensity, this->WeatherTransition.FogDensity, TransitionProgress);
        FogComponent->SetFogDensity(LerpFogDensity);

        auto LerpFogHeightFalloff = FMath::Lerp(this->CachedFogHeightFalloff, this->WeatherTransition.FogHeightFalloff, TransitionProgress);
        FogComponent->SetFogHeightFalloff(LerpFogHeightFalloff);
    }

    UWeatherType *TargetWeather = this->WeatherTransition.TransitionToWeather.Get();
    if (GetLocalRole() == ROLE_Authority && (TransitionProgress >= 1.0f || TargetWeather == this->CurrentWeather || Time < TransitionStartedAt)) {
        auto NewWeatherTag = TargetWeather ? TargetWeather->Tag : FGameplayTag();
        auto OldWeatherTag = this->CurrentWeather ? this->CurrentWeather->Tag : FGameplayTag();

        this->CurrentWeather = TargetWeather;
        this->WeatherTransition.TransitionToWeather = nullptr;
        this->WeatherChangedAt = Time;
        if (this->CurrentWeather == this->GuaranteedTargetWeather) {
            this->GuaranteedTargetWeather = nullptr;
            this->TargetWeatherReachedDelegate.Broadcast(this->CurrentWeather->Tag);
        }

        this->WeatherChangeDelegate.Broadcast(OldWeatherTag, NewWeatherTag);
        UE_LOG(Myth_Environment, Warning, TEXT("EnvironmentController: Transition complete. New weather: %s"), *this->CurrentWeather->GetName());

        if (this->CurrentWeather && this->CurrentWeather->bGeneratesWorldNews && NewWeatherTag.IsValid()) {
            if (const UGameInstance *GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr) {
                if (UMythicLivingWorldSubsystem *LWS = GI->GetSubsystem<UMythicLivingWorldSubsystem>()) {
                    FMythicWorldEvent WeatherEvent;
                    WeatherEvent.EventTag = NewWeatherTag;
                    WeatherEvent.CategoryFlags = EMythicEventCategory::Environment;
                    WeatherEvent.Significance = this->CurrentWeather->WorldNewsSignificance;
                    LWS->SubmitWorldEvent(WeatherEvent);
                }
            }
        }
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
    UpdateLighting();
    UE_LOG(Myth_Environment, Log, TEXT("OnRep_Time: Time synced to %s"), *Time.ToString());
}

void AMythicEnvironmentController::OnRep_CurrentWeather(UWeatherType *PreviousWeather) {
    UE_LOG(Myth_Environment, Log, TEXT("OnRep_CurrentWeather: %s -> %s"),
           PreviousWeather ? *PreviousWeather->GetName() : TEXT("None"),
           CurrentWeather ? *CurrentWeather->GetName() : TEXT("None"));

    if (CurrentWeather) {
        ApplyWeatherVisuals(CurrentWeather);
    }

    FGameplayTag OldTag = PreviousWeather ? PreviousWeather->Tag : FGameplayTag::EmptyTag;
    FGameplayTag NewTag = CurrentWeather ? CurrentWeather->Tag : FGameplayTag::EmptyTag;
    this->WeatherChangeDelegate.Broadcast(OldTag, NewTag);
}

void AMythicEnvironmentController::ApplyWeatherVisuals(const UWeatherType *Weather) {
    if (!Weather || !WeatherMPC) {
        return;
    }

    const bool bHasAuthoritativeData = WeatherTransition.TransitionToScalarValues.Num() > 0;

    if (WeatherTransition.StartTime.GetTicks() > 0) {
        this->TransitionStartedAt = WeatherTransition.StartTime;
    }


    if (bHasAuthoritativeData) {
        for (const auto &Param : WeatherTransition.TransitionToScalarValues) {
            UKismetMaterialLibrary::SetScalarParameterValue(this, WeatherMPC, Param.ParameterName, Param.DefaultValue);
        }

        for (const auto &Param : WeatherTransition.TransitionToVectorValues) {
            UKismetMaterialLibrary::SetVectorParameterValue(this, WeatherMPC, Param.ParameterName, Param.DefaultValue);
        }

        if (FogComponent) {
            FogComponent->SetFogDensity(WeatherTransition.FogDensity);
            FogComponent->SetFogHeightFalloff(WeatherTransition.FogHeightFalloff);
        }
    }
}

void AMythicEnvironmentController::OnRep_WeatherTransition() {
    UMythicAssetManager::LoadAsync(this, WeatherTransition.TransitionToWeather,
                                   [this](UWeatherType *TargetWeather) {
                                       UE_LOG(Myth_Environment, Log, TEXT("OnRep_WeatherTransition: Transition to %s"),
                                              TargetWeather ? *TargetWeather->GetName() : TEXT("None"));

                                       if (!TargetWeather) {
                                           return;
                                       }

                                       TransitionFromScalarValues.Empty();
                                       TransitionFromVectorValues.Empty();

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

                                       this->TransitionStartedAt = WeatherTransition.StartTime;

                                       FGameplayTag FromTag = CurrentWeather ? CurrentWeather->Tag : FGameplayTag::EmptyTag;
                                       FGameplayTag ToTag = TargetWeather ? TargetWeather->Tag : FGameplayTag::EmptyTag;
                                       this->WeatherTransitionDelegate.Broadcast(FromTag, ToTag, WeatherTransition.TransitionLength);
                                   });
}

void AMythicEnvironmentController::CycleWeather() {
    checkf(GetLocalRole() == ROLE_Authority, TEXT("Only the server can cycle weather"));

    UWeatherType *SelectedWeather;
    if (this->GuaranteedTargetWeather) {
        if (auto NextWeather = this->GetNextWeatherTypeToReachTargetWeather(this->CurrentWeather, this->GuaranteedTargetWeather)) {
            SelectedWeather = NextWeather;
        }
        else {
            SelectedWeather = this->GuaranteedTargetWeather;
            this->GuaranteedTargetWeather = nullptr;
        }
    }
    else {
        SelectedWeather = WeatherTypes.Num() > 0 ? WeatherTypes[FMath::RandRange(0, WeatherTypes.Num() - 1)] : nullptr;
    }

    if (!SelectedWeather) {
        UE_LOG(Myth_Environment, Warning, TEXT("No weather type selected"));
        return;
    }

    if (this->CurrentWeather && !this->CurrentWeather->CanTransitionTo(*SelectedWeather)) {
        if (!SelectedWeather->MonthRange.Contains(GetMonthOfYear(this->Time))) {
            this->CycleWeather();
            return;
        }

        return;
    }

    this->WeatherTransition = FWeatherCycleInfo(SelectedWeather, this->Time);

    OnRep_WeatherTransition();
}

void AMythicEnvironmentController::SetWeatherTransition(const FWeatherCycleInfo &NewWeatherCycle) {
    this->WeatherTransition = NewWeatherCycle;
    OnRep_WeatherTransition();
}


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

bool AMythicEnvironmentController::IsWeatherPaused() const {
    return GetWorld()->GetTimerManager().IsTimerPaused(this->WeatherTimerHandle);
}

void AMythicEnvironmentController::SetWindTargetPosition(const FLinearColor &TargetPosition) {
    checkf(GetLocalRole() == ROLE_Authority, TEXT("Only the server can set the wind target position"));

    if (!WeatherMPC) {
        UE_LOG(Myth_Environment, Error, TEXT("Cannot set wind TargetPosition without a WeatherMPC"));
        return;
    }

    this->MulticastSyncWindTarget(TargetPosition);
}

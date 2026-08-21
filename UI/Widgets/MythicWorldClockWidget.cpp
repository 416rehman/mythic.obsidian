// Copyright Stellar Games. All Rights Reserved.

#include "MythicWorldClockWidget.h"

#include "Animation/WidgetAnimation.h"
#include "CommonTextBlock.h"
#include "Components/Image.h"
#include "Engine/GameInstance.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"
#include "UI/MythicHUDLayout.h"
#include "UI/MythicUIStyle.h"
#include "World/EnvironmentController/EnvironmentTags.h"
#include "World/EnvironmentController/MythicEnvironmentController.h"
#include "World/EnvironmentController/MythicEnvironmentSubsystem.h"

#define LOCTEXT_NAMESPACE "Mythic"

namespace {
const TCHAR *Clock_FacePath = TEXT("/Game/Mythic/UI/Globals/materials/kit/MI_UI_SkyFace_HUD.MI_UI_SkyFace_HUD");

const FName Clock_HourFrac(TEXT("HourFrac"));
const FName Clock_Weather(TEXT("Weather"));
const FName Clock_PrevWeather(TEXT("PrevWeather"));
const FName Clock_Transition(TEXT("Transition"));
const FName Clock_MoonPhase(TEXT("MoonPhase"));
const FName Clock_Season(TEXT("Season"));
const FName Clock_Sunrise(TEXT("Sunrise"));
const FName Clock_Sunset(TEXT("Sunset"));
const FName Clock_FacePx(TEXT("FacePx"));
const FName Clock_Wind(TEXT("Wind"));

constexpr float Clock_SunriseHour = 7.0f;
constexpr float Clock_SunsetHour = 20.0f;

constexpr float Clock_FaceSizePx = 64.0f;
constexpr float Clock_QuadPx = 80.0f;

constexpr float Clock_EaseInterval = 1.0f / 30.0f;

constexpr int32 Clock_SpringStartMonth = 3;
constexpr int32 Clock_DaysPerMonth = 30;

constexpr float Clock_DimTint = 0.60f;

FText DayTimeText(EDayTime DayTime) {
    switch (DayTime) {
        case EDayTime::Morning:
            return LOCTEXT("DayMorning", "Morning");
        case EDayTime::Afternoon:
            return LOCTEXT("DayAfternoon", "Afternoon");
        case EDayTime::Evening:
            return LOCTEXT("DayEvening", "Evening");
        case EDayTime::Night:
            return LOCTEXT("DayNight", "Night");
        default:
            return FText::GetEmpty();
    }
}

FText SeasonText(ESeason Season) {
    switch (Season) {
        case ESeason::Spring:
            return LOCTEXT("SeasonSpring", "Spring");
        case ESeason::Summer:
            return LOCTEXT("SeasonSummer", "Summer");
        case ESeason::Autumn:
            return LOCTEXT("SeasonAutumn", "Autumn");
        case ESeason::Winter:
            return LOCTEXT("SeasonWinter", "Winter");
        default:
            return FText::GetEmpty();
    }
}

float SeasonFraction(ESeason Season, int32 Month, int32 Day) {
    const int32 MonthInSeason = ((Month - Clock_SpringStartMonth + 12) % 12) % 3;
    const float Through = static_cast<float>(MonthInSeason * Clock_DaysPerMonth + FMath::Max(Day - 1, 0)) /
                          static_cast<float>(3 * Clock_DaysPerMonth);
    return static_cast<float>(Season) + FMath::Clamp(Through, 0.0f, 0.999f);
}
}


FText UMythicWorldClockWidget::FormatClockTime(int32 Hour, int32 Minute) {
    return FText::FromString(FString::Printf(TEXT("%02d:%02d"), FMath::Clamp(Hour, 0, 23), FMath::Clamp(Minute, 0, 59)));
}

FText UMythicWorldClockWidget::FormatDayLine(uint8 DayTime, uint8 Season) {
    return FormatPhaseLine(DayTime, Season, -1);
}

FText UMythicWorldClockWidget::FormatPhaseLine(uint8 DayTime, uint8 Season, int32 DayOfMonth) {
    const FText Phase = DayTimeText(static_cast<EDayTime>(DayTime));
    const FText Seas = SeasonText(static_cast<ESeason>(Season));

    FText Line;
    if (Phase.IsEmpty()) {
        Line = Seas;
    }
    else if (Seas.IsEmpty()) {
        Line = Phase;
    }
    else {
        Line = FText::Format(LOCTEXT("ClockPhaseLine", "{0} · {1}"), Phase, Seas);
    }
    if (DayOfMonth >= 0) {
        Line = FText::Format(LOCTEXT("ClockPhaseLineDay", "Day {0} · {1}"), FText::AsNumber(DayOfMonth), Line);
    }
    return Line;
}

uint8 UMythicWorldClockWidget::WeatherKind(FGameplayTag Tag) {
    if (Tag.MatchesTagExact(Environment_Weather_Overcast)) {
        return 2;
    }
    if (Tag.MatchesTagExact(Environment_Weather_Rain)) {
        return 3;
    }
    if (Tag.MatchesTagExact(Environment_Weather_Snow)) {
        return 4;
    }
    return 0;
}


void UMythicWorldClockWidget::NativeConstruct() {
    Super::NativeConstruct();

    if (Img_Face) {
        if (UMaterialInterface *Face = LoadObject<UMaterialInterface>(nullptr, Clock_FacePath)) {
            FSlateBrush Brush;
            Brush.SetResourceObject(Face);
            Brush.DrawAs = ESlateBrushDrawType::Image;
            Brush.ImageSize = FVector2D(Clock_QuadPx, Clock_QuadPx);
            Brush.TintColor = FSlateColor(FLinearColor::White);
            Img_Face->SetBrush(Brush);
            FaceMID = Img_Face->GetDynamicMaterial();
        }
        Img_Face->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
    if (FaceMID) {
        FaceMID->SetScalarParameterValue(Clock_Sunrise, Clock_SunriseHour);
        FaceMID->SetScalarParameterValue(Clock_Sunset, Clock_SunsetHour);
        FaceMID->SetScalarParameterValue(Clock_FacePx, Clock_FaceSizePx);
        FaceMID->SetScalarParameterValue(Clock_Transition, 1.0f);
        FaceMID->SetScalarParameterValue(Clock_Weather, 0.0f);
        FaceMID->SetScalarParameterValue(Clock_PrevWeather, 0.0f);
        FaceMID->SetScalarParameterValue(Clock_MoonPhase, 0.5f);
        FaceMID->SetScalarParameterValue(Clock_Wind, 0.0f);
    }

    const auto Resize = [](UCommonTextBlock *Text, int32 Size) {
        if (!Text) {
            return;
        }
        FSlateFontInfo Font = Text->GetFont();
        Font.Size = Size;
        Font.LetterSpacing = 0;
        Font.bForceMonospaced = false;
        Text->SetFont(Font);
        Text->SetJustification(ETextJustify::Center);
        Text->SetShadowOffset(FVector2D(1.0f, 1.0f));
        Text->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.55f));
    };
    Resize(Txt_Time, TimeFontSize);
    Resize(Txt_Phase, PhaseFontSize);
    if (Txt_Time) {
        Txt_Time->SetColorAndOpacity(FSlateColor(FMythicUIStyle::Get().Ink));
    }
    if (Txt_Phase) {
        Txt_Phase->SetColorAndOpacity(FSlateColor(FMythicUIStyle::Get().InkSubtle * 1.15f));
        Txt_Phase->SetRenderOpacity(0.0f);
    }

    const auto Collapse = [](UWidget *W) {
        if (W) {
            W->SetVisibility(ESlateVisibility::Collapsed);
        }
    };
    Collapse(Img_Face);
    Collapse(Txt_Time);
    Collapse(Txt_Phase);
    bChildrenShown = false;

    if (UWorld *World = GetWorld()) {
        TWeakObjectPtr<UMythicWorldClockWidget> WeakThis(this);
        World->GetTimerManager().SetTimerForNextTick([WeakThis]() {
            UMythicWorldClockWidget *Self = WeakThis.Get();
            UMythicHUDLayout *Layout = Self ? Self->FindLayout() : nullptr;
            if (!Layout) {
                return;
            }
            Layout->SetElementDimTint(Self, Clock_DimTint);
            Layout->OnHUDRevealChanged.AddUObject(Self, &UMythicWorldClockWidget::HandleRevealChanged);
        });
    }

    Poll();

    if (UWorld *World = GetWorld()) {
        World->GetTimerManager().SetTimer(PollTimer, FTimerDelegate::CreateWeakLambda(this, [this]() { Poll(); }),
                                          PollInterval, true);
    }
}

void UMythicWorldClockWidget::NativeDestruct() {
    if (const UWorld *World = GetWorld()) {
        FTimerManager &Timers = World->GetTimerManager();
        Timers.ClearTimer(PollTimer);
        Timers.ClearTimer(PhaseWordTimer);
        Timers.ClearTimer(TransitionTimer);
    }
    UnbindController();
    if (UMythicHUDLayout *Layout = FindLayout()) {
        Layout->OnHUDRevealChanged.RemoveAll(this);
    }
    Super::NativeDestruct();
}

UMythicHUDLayout *UMythicWorldClockWidget::FindLayout() const {
    return GetTypedOuter<UMythicHUDLayout>();
}

void UMythicWorldClockWidget::BindController(AMythicEnvironmentController *Controller) {
    if (!Controller || BoundController.Get() == Controller) {
        return;
    }
    UnbindController();
    BoundController = Controller;
    Controller->WeatherTransitionDelegate.AddUniqueDynamic(this, &UMythicWorldClockWidget::HandleWeatherTransitionStart);
    Controller->WeatherChangeDelegate.AddUniqueDynamic(this, &UMythicWorldClockWidget::HandleWeatherChanged);
    Controller->DayTimeChangeDelegate.AddUniqueDynamic(this, &UMythicWorldClockWidget::HandleDayTimeChanged);
    Controller->DayChangeDelegate.AddUniqueDynamic(this, &UMythicWorldClockWidget::HandleDayChanged);
    Controller->MonthChangeDelegate.AddUniqueDynamic(this, &UMythicWorldClockWidget::HandleMonthChanged);
}

void UMythicWorldClockWidget::UnbindController() {
    AMythicEnvironmentController *Controller = BoundController.Get();
    BoundController = nullptr;
    if (!Controller) {
        return;
    }
    Controller->WeatherTransitionDelegate.RemoveDynamic(this, &UMythicWorldClockWidget::HandleWeatherTransitionStart);
    Controller->WeatherChangeDelegate.RemoveDynamic(this, &UMythicWorldClockWidget::HandleWeatherChanged);
    Controller->DayTimeChangeDelegate.RemoveDynamic(this, &UMythicWorldClockWidget::HandleDayTimeChanged);
    Controller->DayChangeDelegate.RemoveDynamic(this, &UMythicWorldClockWidget::HandleDayChanged);
    Controller->MonthChangeDelegate.RemoveDynamic(this, &UMythicWorldClockWidget::HandleMonthChanged);
}


void UMythicWorldClockWidget::Poll() {
    const UGameInstance *GI = GetGameInstance();
    UMythicEnvironmentSubsystem *Env = GI ? GI->GetSubsystem<UMythicEnvironmentSubsystem>() : nullptr;
    AMythicEnvironmentController *Controller = Env ? Env->GetEnvironmentController() : nullptr;
    if (!Controller) {
        UnbindController();
        return;
    }

    const bool bFirstLook = BoundController.Get() != Controller;
    BindController(Controller);

    if (!bChildrenShown) {
        bChildrenShown = true;
        const auto Show = [](UWidget *W) {
            if (W) {
                W->SetVisibility(ESlateVisibility::HitTestInvisible);
            }
        };
        Show(Img_Face);
        Show(Txt_Time);
        Show(Txt_Phase);
    }

    const FDateTime Now = Env->GetCurrentTime();

    if (bFirstLook && FaceMID) {
        CurrentKind = WeatherKind(Env->GetWeather());
        TransitionT = 1.0f;
        FaceMID->SetScalarParameterValue(Clock_Weather, static_cast<float>(CurrentKind));
        FaceMID->SetScalarParameterValue(Clock_PrevWeather, static_cast<float>(CurrentKind));
        FaceMID->SetScalarParameterValue(Clock_Transition, 1.0f);
        LastDayKey = -1;
    }

    const int32 MinuteKey = Now.GetHour() * 60 + Now.GetMinute();
    if (MinuteKey != LastMinuteKey) {
        LastMinuteKey = MinuteKey;
        if (FaceMID) {
            const float HourFrac = (static_cast<float>(Now.GetHour()) + Now.GetMinute() / 60.0f +
                                    Now.GetSecond() / 3600.0f) / 24.0f;
            FaceMID->SetScalarParameterValue(Clock_HourFrac, HourFrac);
        }
        const FString Time = FormatClockTime(Now.GetHour(), Now.GetMinute()).ToString();
        if (Txt_Time && Time != LastTime) {
            LastTime = Time;
            Txt_Time->SetText(FText::FromString(Time));
        }
    }

    const int32 DayKey = Now.GetMonth() * 100 + Now.GetDay();
    if (DayKey != LastDayKey) {
        LastDayKey = DayKey;
        if (FaceMID) {
            FaceMID->SetScalarParameterValue(Clock_MoonPhase,
                                             static_cast<float>(FMath::Max(Now.GetDay() - 1, 0)) / 30.0f);
            FaceMID->SetScalarParameterValue(Clock_Season,
                                             SeasonFraction(Env->GetSeason(), Now.GetMonth(), Now.GetDay()));
        }
        RefreshPhaseLine();
    }

    if (bFirstLook) {
        RefreshPhaseLine();
    }

    if (const UMythicHUDLayout *Layout = FindLayout()) {
        const bool bRevealed = Layout->IsHUDRevealed();
        if (bRevealed && !bPhaseWordShown) {
            ShowPhaseWord(0.0f);
        }
        else if (!bRevealed && bPhaseWordShown && !GetWorld()->GetTimerManager().IsTimerActive(PhaseWordTimer)) {
            HidePhaseWord();
        }
    }
}


void UMythicWorldClockWidget::BeginWeatherEase(uint8 FromKind, uint8 ToKind) {
    CurrentKind = ToKind;
    TransitionT = 0.0f;
    if (FaceMID) {
        FaceMID->SetScalarParameterValue(Clock_PrevWeather, static_cast<float>(FromKind));
        FaceMID->SetScalarParameterValue(Clock_Weather, static_cast<float>(ToKind));
        FaceMID->SetScalarParameterValue(Clock_Transition, 0.0f);
    }
    if (UWorld *World = GetWorld()) {
        if (!World->GetTimerManager().IsTimerActive(TransitionTimer)) {
            World->GetTimerManager().SetTimer(TransitionTimer,
                                              FTimerDelegate::CreateWeakLambda(this, [this]() { TickWeatherEase(); }),
                                              Clock_EaseInterval, true);
        }
    }
}

void UMythicWorldClockWidget::TickWeatherEase() {
    TransitionT += Clock_EaseInterval / FMath::Max(WeatherEaseSeconds, 0.01f);
    const float K = FMath::Clamp(TransitionT, 0.0f, 1.0f);
    if (FaceMID) {
        FaceMID->SetScalarParameterValue(Clock_Transition, K * K * (3.0f - 2.0f * K));
    }
    if (TransitionT >= 1.0f) {
        TransitionT = 1.0f;
        if (FaceMID) {
            FaceMID->SetScalarParameterValue(Clock_Transition, 1.0f);
        }
        if (const UWorld *World = GetWorld()) {
            World->GetTimerManager().ClearTimer(TransitionTimer);
        }
    }
}

void UMythicWorldClockWidget::HandleWeatherTransitionStart(FGameplayTag From, FGameplayTag To, float Length) {
    const uint8 ToKind = WeatherKind(To);
    const uint8 FromKind = (TransitionT < 0.5f) ? WeatherKind(From) : CurrentKind;
    if (ToKind == CurrentKind && TransitionT >= 1.0f) {
        return;
    }
    BeginWeatherEase(FromKind, ToKind);
    if (UMythicHUDLayout *Layout = FindLayout()) {
        Layout->PokeElement(this);
    }
    ShowPhaseWord(PhaseWordHoldSeconds);
}

void UMythicWorldClockWidget::HandleWeatherChanged(FGameplayTag Previous, FGameplayTag New) {
    const uint8 NewKind = WeatherKind(New);
    if (NewKind == CurrentKind) {
        return;
    }
    BeginWeatherEase(CurrentKind, NewKind);
    if (UMythicHUDLayout *Layout = FindLayout()) {
        Layout->PokeElement(this);
    }
    ShowPhaseWord(PhaseWordHoldSeconds);
}


void UMythicWorldClockWidget::HandleDayTimeChanged(EDayTime PrevDayTime, EDayTime NewDayTime) {
    RefreshPhaseLine();
    if (UMythicHUDLayout *Layout = FindLayout()) {
        Layout->PokeElement(this);
    }
    ShowPhaseWord(PhaseWordHoldSeconds);
}

void UMythicWorldClockWidget::HandleDayChanged(int32 PrevDay, int32 NewDay) {
    if (FaceMID) {
        FaceMID->SetScalarParameterValue(Clock_MoonPhase, static_cast<float>(FMath::Max(NewDay - 1, 0)) / 30.0f);
    }
    if (bShowDayCount) {
        RefreshPhaseLine();
    }
}

void UMythicWorldClockWidget::HandleMonthChanged(int32 PrevMonth, int32 NewMonth, ESeason PrevSeason, ESeason NewSeason) {
    if (FaceMID) {
        FaceMID->SetScalarParameterValue(Clock_Season, SeasonFraction(NewSeason, NewMonth, 1));
    }
    if (PrevSeason == NewSeason) {
        return;
    }
    RefreshPhaseLine();
    if (UMythicHUDLayout *Layout = FindLayout()) {
        Layout->PokeElement(this);
    }
    ShowPhaseWord(PhaseWordHoldSeconds);
}

void UMythicWorldClockWidget::RefreshPhaseLine() {
    if (!Txt_Phase) {
        return;
    }
    const UGameInstance *GI = GetGameInstance();
    UMythicEnvironmentSubsystem *Env = GI ? GI->GetSubsystem<UMythicEnvironmentSubsystem>() : nullptr;
    if (!Env || !Env->GetEnvironmentController()) {
        return;
    }
    const int32 DayOfMonth = bShowDayCount ? Env->GetCurrentTime().GetDay() : -1;
    const FString Line = FormatPhaseLine(static_cast<uint8>(Env->GetDayTime()), static_cast<uint8>(Env->GetSeason()),
                                         DayOfMonth).ToString();
    if (Line != LastPhase) {
        LastPhase = Line;
        Txt_Phase->SetText(FText::FromString(Line));
    }
}

void UMythicWorldClockWidget::ShowPhaseWord(float HoldSeconds) {
    RefreshPhaseLine();
    if (!bPhaseWordShown) {
        bPhaseWordShown = true;
        if (Anim_PhaseLine) {
            PlayAnimationForward(Anim_PhaseLine);
        }
        else if (Txt_Phase) {
            Txt_Phase->SetRenderOpacity(1.0f);
        }
    }
    if (HoldSeconds > 0.0f) {
        if (UWorld *World = GetWorld()) {
            World->GetTimerManager().SetTimer(PhaseWordTimer, FTimerDelegate::CreateWeakLambda(this, [this]() {
                                                  const UMythicHUDLayout *Layout = FindLayout();
                                                  if (!Layout || !Layout->IsHUDRevealed()) {
                                                      HidePhaseWord();
                                                  }
                                              }),
                                              HoldSeconds, false);
        }
    }
}

void UMythicWorldClockWidget::HidePhaseWord() {
    if (!bPhaseWordShown) {
        return;
    }
    bPhaseWordShown = false;
    if (Anim_PhaseLine) {
        PlayAnimationReverse(Anim_PhaseLine);
    }
    else if (Txt_Phase) {
        Txt_Phase->SetRenderOpacity(0.0f);
    }
}

void UMythicWorldClockWidget::HandleRevealChanged(bool bRevealed) {
    if (bRevealed) {
        ShowPhaseWord(0.0f);
    }
    else if (const UWorld *World = GetWorld()) {
        if (!World->GetTimerManager().IsTimerActive(PhaseWordTimer)) {
            HidePhaseWord();
        }
    }
}

#undef LOCTEXT_NAMESPACE

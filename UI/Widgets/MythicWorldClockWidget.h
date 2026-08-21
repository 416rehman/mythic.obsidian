// Copyright Stellar Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "World/EnvironmentController/EnvironmentTypes.h"
#include "MythicWorldClockWidget.generated.h"

class AMythicEnvironmentController;
class UCommonTextBlock;
class UImage;
class UMaterialInstanceDynamic;
class UMythicHUDLayout;
class UWidgetAnimation;

UCLASS()
class MYTHIC_API UMythicWorldClockWidget : public UUserWidget {
    GENERATED_BODY()

public:
    /** "06:27". Zero-padded, so the readout never changes width and never jitters. */
    UFUNCTION(BlueprintPure, Category = "Mythic|HUD")
    static FText FormatClockTime(int32 Hour, int32 Minute);

    /** "Morning · Spring". Kept for Blueprint callers; the widget itself uses FormatPhaseLine. */
    UFUNCTION(BlueprintPure, Category = "Mythic|HUD")
    static FText FormatDayLine(uint8 DayTime, uint8 Season);

    /** "Afternoon · Autumn", or "Day 12 · Afternoon · Autumn" when DayOfMonth >= 0. */
    UFUNCTION(BlueprintPure, Category = "Mythic|HUD")
    static FText FormatPhaseLine(uint8 DayTime, uint8 Season, int32 DayOfMonth);

    /**
     * The material's weather index. Clear 0, Cloudy 1, Overcast 2, Rain 3, Snow 4, Wind 5.
     *
     * EnvironmentTags.h only defines Clear / Overcast / Rain / Snow today; 1 and 5 are in the material already so
     * that adding the tags later is a one-line change here and nothing in the shader.
     */
    UFUNCTION(BlueprintPure, Category = "Mythic|HUD")
    static uint8 WeatherKind(FGameplayTag Tag);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    /** The sky-glass itself: one quad carrying MI_UI_SkyFace_HUD. Everything you see is in here. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> Img_Face;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> Txt_Time;

    /** "Afternoon · Autumn". Invisible at rest; fades in on a change or while Z is held. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> Txt_Phase;

    UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
    TObjectPtr<UWidgetAnimation> Anim_PhaseLine;

    /** How often to look at the clock. Cheap: a look that finds no change does nothing at all. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|HUD", meta = (ClampMin = "0.1"))
    float PollInterval = 1.0f;

    /**
     * Sizes are set here rather than on the widgets because CommonTextBlock re-applies its style's font on every
     * construct, so a size typed into the designer is thrown away.
     */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|HUD", meta = (ClampMin = "6"))
    int32 TimeFontSize = 16;

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|HUD", meta = (ClampMin = "6"))
    int32 PhaseFontSize = 11;

    /** How long the phase line stays after a change. Keep equal to the HUD rule row's hold, or they disagree. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|HUD", meta = (ClampMin = "0.0"))
    float PhaseWordHoldSeconds = 4.0f;

    /** The weather ease. The controller's own fade takes game-minutes; the face leads it so the change reads. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|HUD", meta = (ClampMin = "0.1"))
    float WeatherEaseSeconds = 2.0f;

    /** Off by default: "Day 12 · Afternoon · Autumn" is 151px and sits 4px from the screen edge at 1920. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|HUD")
    bool bShowDayCount = false;

private:
    void Poll();
    void BindController(AMythicEnvironmentController *Controller);
    void UnbindController();
    UMythicHUDLayout *FindLayout() const;

    void BeginWeatherEase(uint8 FromKind, uint8 ToKind);
    void TickWeatherEase();

    void RefreshPhaseLine();
    void ShowPhaseWord(float HoldSeconds);
    void HidePhaseWord();
    void HandleRevealChanged(bool bRevealed);

    UFUNCTION()
    void HandleWeatherTransitionStart(FGameplayTag From, FGameplayTag To, float Length);

    UFUNCTION()
    void HandleWeatherChanged(FGameplayTag Previous, FGameplayTag New);

    UFUNCTION()
    void HandleDayTimeChanged(EDayTime PrevDayTime, EDayTime NewDayTime);

    UFUNCTION()
    void HandleDayChanged(int32 PrevDay, int32 NewDay);

    UFUNCTION()
    void HandleMonthChanged(int32 PrevMonth, int32 NewMonth, ESeason PrevSeason, ESeason NewSeason);

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> FaceMID;

    UPROPERTY(Transient)
    TWeakObjectPtr<AMythicEnvironmentController> BoundController;

    FTimerHandle PollTimer;
    FTimerHandle PhaseWordTimer;
    FTimerHandle TransitionTimer;

    float TransitionT = 1.0f;

    uint8 CurrentKind = 0;

    FString LastTime;
    FString LastPhase;

    int32 LastMinuteKey = -1;
    int32 LastDayKey = -1;

    bool bChildrenShown = false;
    bool bPhaseWordShown = false;
};

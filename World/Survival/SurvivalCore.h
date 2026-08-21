
#pragma once

#include "CoreMinimal.h"

enum ESurvivalStatusBit : uint8 {
    ESSB_None = 0,
    ESSB_Starving = 1 << 0,
    ESSB_WellFed = 1 << 1,
    ESSB_Dehydrated = 1 << 2,
    ESSB_Cold = 1 << 3,
    ESSB_Overheated = 1 << 4,
};

struct FSurvivalThresholds {
    float StarvingEnter = 0.15f;
    float StarvingExit = 0.20f;
    float WellFedEnter = 0.85f;
    float WellFedExit = 0.80f;
    float DehydratedEnter = 0.15f;
    float DehydratedExit = 0.20f;
    float ColdEnter = 0.20f;
    float ColdExit = 0.25f;
    float OverheatedEnter = 0.90f;
    float OverheatedExit = 0.85f;

    float WetColdAggravation = 0.10f;
};

struct FWarmthWetnessRates {
    float WarmSourceWarmthPerSecond = 8.0f;
    float ColdWeatherWarmthPerSecond = 3.0f;
    float WetChillWarmthPerSecond = 2.0f;
    float PassiveWarmthRegenPerSecond = 2.0f;
    float WettingPerSecond = 6.0f;
    float DryingPerSecond = 4.0f;
    float MaxWarmth = 100.0f;
    float MaxWetness = 100.0f;
    float NeutralWarmth = 50.0f;
};

struct FSurvivalWarmthWetnessResult {
    float NetWarmthDelta = 0.0f;
    float NetWetnessDelta = 0.0f;
};

struct FMythicSurvivalCore {
    static float ComputeDecayStep(float Current, float DecayPerSecond, float DeltaSeconds, float MaxValue) {
        if (DecayPerSecond <= 0.0f || DeltaSeconds <= 0.0f) {
            return FMath::Clamp(Current, 0.0f, FMath::Max(0.0f, MaxValue));
        }
        return FMath::Clamp(Current - (DecayPerSecond * DeltaSeconds), 0.0f, FMath::Max(0.0f, MaxValue));
    }

    static bool ResolveLowBand(bool bWasActive, float Value, float Enter, float Exit) {
        return bWasActive ? (Value < Exit) : (Value < Enter);
    }

    static bool ResolveHighBand(bool bWasActive, float Value, float Enter, float Exit) {
        return bWasActive ? (Value > Exit) : (Value > Enter);
    }

    static uint8 ResolveStatus(float NourishmentFrac, float HydrationFrac, float WarmthFrac, float WetnessFrac,
                               const FSurvivalThresholds &T, uint8 PreviousMask) {
        uint8 Mask = ESSB_None;

        if (ResolveLowBand((PreviousMask & ESSB_Starving) != 0, NourishmentFrac, T.StarvingEnter, T.StarvingExit)) {
            Mask |= ESSB_Starving;
        }
        if (ResolveHighBand((PreviousMask & ESSB_WellFed) != 0, NourishmentFrac, T.WellFedEnter, T.WellFedExit)) {
            Mask |= ESSB_WellFed;
        }
        if (ResolveLowBand((PreviousMask & ESSB_Dehydrated) != 0, HydrationFrac, T.DehydratedEnter, T.DehydratedExit)) {
            Mask |= ESSB_Dehydrated;
        }

        const float WetShift = FMath::Clamp(WetnessFrac, 0.0f, 1.0f) * T.WetColdAggravation;
        if (ResolveLowBand((PreviousMask & ESSB_Cold) != 0, WarmthFrac, T.ColdEnter + WetShift, T.ColdExit + WetShift)) {
            Mask |= ESSB_Cold;
        }
        if (ResolveHighBand((PreviousMask & ESSB_Overheated) != 0, WarmthFrac, T.OverheatedEnter, T.OverheatedExit)) {
            Mask |= ESSB_Overheated;
        }

        return Mask;
    }

    static FSurvivalWarmthWetnessResult ComputeWarmthWetnessNet(bool bHasWarmSource, bool bSheltered, bool bColdWeather,
                                                                bool bWetWeather, float CurrentWarmth, float CurrentWetness,
                                                                const FWarmthWetnessRates &R, float DeltaSeconds) {
        FSurvivalWarmthWetnessResult Out;
        const float Dt = FMath::Max(0.0f, DeltaSeconds);
        const float MaxWarmth = FMath::Max(0.0f, R.MaxWarmth);
        const float MaxWetness = FMath::Max(0.0f, R.MaxWetness);

        float WarmthDelta;
        if (bHasWarmSource) {
            WarmthDelta = R.WarmSourceWarmthPerSecond * Dt;
        }
        else if (bColdWeather && !bSheltered) {
            const float WetFrac = MaxWetness > 0.0f ? FMath::Clamp(CurrentWetness / MaxWetness, 0.0f, 1.0f) : 0.0f;
            WarmthDelta = -(R.ColdWeatherWarmthPerSecond + (R.WetChillWarmthPerSecond * WetFrac)) * Dt;
        }
        else {
            const float Toward = R.NeutralWarmth - CurrentWarmth;
            const float Step = R.PassiveWarmthRegenPerSecond * Dt;
            WarmthDelta = FMath::Clamp(Toward, -Step, Step);
        }
        Out.NetWarmthDelta = FMath::Clamp(CurrentWarmth + WarmthDelta, 0.0f, MaxWarmth) - CurrentWarmth;

        float WetnessDelta;
        if (bWetWeather && !bSheltered && !bHasWarmSource) {
            WetnessDelta = R.WettingPerSecond * Dt;
        }
        else {
            WetnessDelta = -R.DryingPerSecond * Dt;
        }
        Out.NetWetnessDelta = FMath::Clamp(CurrentWetness + WetnessDelta, 0.0f, MaxWetness) - CurrentWetness;

        return Out;
    }

    static bool IsSurvivalActive(bool bMasterEnabled, bool bHasSurvivalSet, bool bIsDead) {
        return bMasterEnabled && bHasSurvivalSet && !bIsDead;
    }
};


#pragma once

#include "CoreMinimal.h"
#include "World/Gathering/MythicYieldQuality.h"

struct FMythicHusbandryRules {
    static EMythicYieldQuality FeedToProduceTier(EMythicYieldQuality FeedTier) {
        const int32 Floor = FMythicYieldQuality::TierIndex(EMythicYieldQuality::Common);
        return FMythicYieldQuality::TierFromIndex(FMath::Max(FMythicYieldQuality::TierIndex(FeedTier), Floor));
    }

    static float FedWindowSeconds(double LastSampleTime, double Now, double FedUntilTime) {
        const double End = FMath::Min(Now, FedUntilTime);
        return static_cast<float>(FMath::Max(0.0, End - LastSampleTime));
    }

    static double ExtendFedUntil(double Now, double CurrentFedUntil, float FeedSecondsPerUnit, float MaxBankSeconds) {
        const double Base = FMath::Max(Now, CurrentFedUntil);
        const double Extended = Base + FMath::Max(0.0f, FeedSecondsPerUnit);
        return FMath::Min(Extended, Now + static_cast<double>(FMath::Max(0.0f, MaxBankSeconds)));
    }
};


#pragma once

#include "CoreMinimal.h"

struct FMythicCorpseHazardConfig {
    float DiseaseStartStageInt = 2.0f;

    float DiseaseRadius = 600.0f;

    float DiseaseSeverityPerStage = 0.5f;

    float SanitationRadius = 2000.0f;

    float SanitationPerFreshCorpse = 1.0f;

    float CarrionAttractPerStage = 1.0f;

    float TickSeconds = 5.0f;
};

struct FMythicCorpseHazardRules {
    static constexpr int32 MaxStageInt = 3;

    static float SanitationPenalty(int32 DecompStageInt, float DistanceToSettlement, const FMythicCorpseHazardConfig &Cfg) {
        if (Cfg.SanitationRadius <= 0.0f) {
            return 0.0f;
        }
        const int32 Stage = FMath::Clamp(DecompStageInt, 0, MaxStageInt);
        const float Dist = FMath::Max(0.0f, DistanceToSettlement);

        const float DistanceFactor = FMath::Clamp(1.0f - (Dist / Cfg.SanitationRadius), 0.0f, 1.0f);

        const float StageFactor = static_cast<float>(MaxStageInt + 1 - Stage) / static_cast<float>(MaxStageInt + 1);

        return FMath::Max(0.0f, Cfg.SanitationPerFreshCorpse) * StageFactor * DistanceFactor;
    }

    static float DiseaseSeverity(int32 DecompStageInt, const FMythicCorpseHazardConfig &Cfg) {
        const int32 Stage = FMath::Clamp(DecompStageInt, 0, MaxStageInt);
        const int32 StartStage = FMath::Clamp(FMath::FloorToInt(Cfg.DiseaseStartStageInt), 0, MaxStageInt);
        if (Stage < StartStage) {
            return 0.0f;
        }
        const int32 StagesInto = (Stage - StartStage) + 1;
        return FMath::Max(0.0f, Cfg.DiseaseSeverityPerStage) * static_cast<float>(StagesInto);
    }

    static bool ShouldEmitDisease(int32 DecompStageInt, const FMythicCorpseHazardConfig &Cfg) {
        return DiseaseSeverity(DecompStageInt, Cfg) > 0.0f;
    }

    static float CarrionAttractiveness(int32 DecompStageInt, const FMythicCorpseHazardConfig &Cfg) {
        const int32 Stage = FMath::Clamp(DecompStageInt, 0, MaxStageInt);
        float Profile;
        switch (Stage) {
        case 0:  Profile = 0.25f; break;
        case 1:  Profile = 1.00f; break;
        case 2:  Profile = 0.75f; break;
        default: Profile = 0.00f; break;
        }
        return FMath::Max(0.0f, Cfg.CarrionAttractPerStage) * Profile;
    }


    static bool ShouldEvictForCap(int32 LiveCount, int32 MaxActive) {
        return MaxActive > 0 && LiveCount > MaxActive;
    }

    static int32 PickEvictIndex(const TArray<float> &AgesSeconds, const TArray<bool> &bLocked) {
        int32 BestIdx = INDEX_NONE;
        float BestAge = -1.0f;
        for (int32 i = 0; i < AgesSeconds.Num(); ++i) {
            if (bLocked.IsValidIndex(i) && bLocked[i]) {
                continue;
            }
            if (AgesSeconds[i] > BestAge) {
                BestAge = AgesSeconds[i];
                BestIdx = i;
            }
        }
        return BestIdx;
    }
};

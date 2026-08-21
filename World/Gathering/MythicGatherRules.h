
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "World/Gathering/MythicHarvestPressureRules.h"

struct FMythicGatherRules {
    static bool CanGather(const FGameplayTagContainer &EquippedToolProbe, const FGameplayTag &RequiredToolTag) {
        if (!RequiredToolTag.IsValid()) {
            return true;
        }
        return EquippedToolProbe.HasTag(RequiredToolTag);
    }

    static float TierFactor(int32 TierIdx) {
        const int32 Tier = FMath::Max(0, TierIdx);
        return 1.0f + 0.5f * static_cast<float>(Tier);
    }

    static float TierYieldMultiplier(int32 TierIdx) {
        return TierFactor(TierIdx);
    }

    static float ScaledRespawnDelay(float BaseDelay, int32 TierIdx) {
        if (BaseDelay <= 0.0f) {
            return BaseDelay;
        }
        return BaseDelay * TierFactor(TierIdx);
    }


    static float DepletedYieldMultiplier(int32 TierIdx, float HarvestPressure, const FMythicHarvestPressureConfig &Cfg) {
        return TierYieldMultiplier(TierIdx) * FMythicHarvestPressureRules::DepletionYieldMultiplier(HarvestPressure, Cfg);
    }

    static float DepletedRespawnDelay(float BaseDelay, int32 TierIdx, float HarvestPressure, const FMythicHarvestPressureConfig &Cfg) {
        return ScaledRespawnDelay(BaseDelay, TierIdx) * FMythicHarvestPressureRules::RespawnDelayMultiplier(HarvestPressure, Cfg);
    }

    static bool IsRespawnGated(float HarvestPressure, const FMythicHarvestPressureConfig &Cfg) {
        return FMythicHarvestPressureRules::IsRespawnGated(HarvestPressure, Cfg.RespawnGateThreshold);
    }
};

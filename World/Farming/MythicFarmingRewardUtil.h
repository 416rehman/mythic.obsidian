
#pragma once

#include "CoreMinimal.h"
#include "Rewards/RewardBase.h"
#include "World/Gathering/MythicYieldQuality.h"

namespace MythicFarmingRewards {
inline bool HasAny(const FRewardsToGive &Rewards) {
    return Rewards.XPReward || Rewards.ItemReward || Rewards.LootReward || Rewards.AbilityReward ||
           Rewards.AttributeReward || Rewards.RenownReward;
}

inline const FRewardsToGive &RouteByTier(EMythicYieldQuality Tier, const FRewardsToGive &Base, const FRewardsToGive &Fine,
                                         const FRewardsToGive &Pristine) {
    if (Tier == EMythicYieldQuality::Pristine && HasAny(Pristine)) {
        return Pristine;
    }
    if (Tier >= EMythicYieldQuality::Fine && HasAny(Fine)) {
        return Fine;
    }
    return Base;
}
}

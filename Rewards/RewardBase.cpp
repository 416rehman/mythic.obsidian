// 

#pragma once

#include "RewardBase.h"
#include "ItemReward.h"
#include "LootReward.h"
#include "XPReward.h"
#include "AbilityReward.h"
#include "AttributeReward.h"

bool FRewardsToGive::Give(APlayerController *PlayerController, bool IsPrivateItem, int32 ItemLevel) const {
    if (!PlayerController) {
        UE_LOG(Mythic, Error, TEXT("FRewardsToGive::Give PlayerController is null"));
        return false;
    }

    auto retval = true;
    
    if (this->XPReward) {
        auto Context = FXPRewardContext(PlayerController);
        retval = this->XPReward->Give(Context) && retval;
        UE_LOG(Mythic, Log, TEXT("FRewardsToGive::Give Gave XP Reward"))
    }

    if (this->ItemReward) {
        auto Context = FItemRewardContext(PlayerController);
        Context.bIsPrivate = IsPrivateItem;
        Context.ItemLevel = ItemLevel;
        retval = this->ItemReward->Give(Context) && retval;
        UE_LOG(Mythic, Log, TEXT("FRewardsToGive::Give Gave Item Reward"))
    }

    if (this->LootReward) {
        auto Context = FLootRewardContext(PlayerController);
        Context.ItemLevel = ItemLevel;
        retval = this->LootReward->Give(Context) && retval;
        UE_LOG(Mythic, Log, TEXT("FRewardsToGive::Give Gave Loot Reward"))
    }

    if (this->AbilityReward) {
        auto Context = FRewardContext(PlayerController);
        retval = this->AbilityReward->Give(Context) && retval;
        UE_LOG(Mythic, Log, TEXT("FRewardsToGive::Give Gave Ability Reward"))
    }

    if (this->AttributeReward) {
        auto Context = FRewardContext(PlayerController);       
        retval = this->AttributeReward->Give(Context) && retval;
        UE_LOG(Mythic, Log, TEXT("FRewardsToGive::Give Gave Attribute Reward"));
    }

    return retval;
}

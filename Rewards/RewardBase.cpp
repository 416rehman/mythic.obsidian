// 

#pragma once

#include "RewardBase.h"
#include "ItemReward.h"
#include "LootReward.h"
#include "XPReward.h"
#include "AbilityReward.h"
#include "AttributeReward.h"
#include "Mythic.h"

bool FRewardsToGive::Give(APlayerController *PlayerController, bool IsPrivateItem, int32 ItemLevel, FVector SpawnLocation) const {
    if (!PlayerController) {
        UE_LOG(Myth, Error, TEXT("FRewardsToGive::Give PlayerController is null"));
        return false;
    }

    auto retval = true;

    if (this->XPReward) {
        auto Context = FXPRewardContext(PlayerController);
        // Forward the level like the Item/Loot contexts below, so the XP reward's overlevel scaling (XPReward.h
        // OverlevelXPBonus) can fire for a LEVELED reward source. Unleveled callers pass ItemLevel=0 → Level=0 →
        // CalculateXP correctly gives flat XP; only the batch path had been silently dropping the level for XP alone.
        Context.Level = ItemLevel;
        retval = this->XPReward->Give(Context) && retval;
        UE_LOG(Myth, Log, TEXT("FRewardsToGive::Give Gave XP Reward"))
    }

    if (this->ItemReward) {
        auto Context = FItemRewardContext(PlayerController);
        Context.bIsPrivate = IsPrivateItem;
        Context.ItemLevel = ItemLevel;
        // Spawn the dropped item at the reward SOURCE (e.g. the destroyed resource node), not the player's feet.
        // ZeroVector means "no source given" → ItemReward/LootReward fall back to the pawn location (backward-safe).
        Context.SpawnLocation = SpawnLocation;
        retval = this->ItemReward->Give(Context) && retval;
        UE_LOG(Myth, Log, TEXT("FRewardsToGive::Give Gave Item Reward"))
    }

    if (this->LootReward) {
        auto Context = FLootRewardContext(PlayerController);
        Context.ItemLevel = ItemLevel;
        Context.SpawnLocation = SpawnLocation; // same source-location forward as the Item context above
        retval = this->LootReward->Give(Context) && retval;
        UE_LOG(Myth, Log, TEXT("FRewardsToGive::Give Gave Loot Reward"))
    }

    if (this->AbilityReward) {
        auto Context = FRewardContext(PlayerController);
        retval = this->AbilityReward->Give(Context) && retval;
        UE_LOG(Myth, Log, TEXT("FRewardsToGive::Give Gave Ability Reward"))
    }

    if (this->AttributeReward) {
        auto Context = FRewardContext(PlayerController);
        retval = this->AttributeReward->Give(Context) && retval;
        UE_LOG(Myth, Log, TEXT("FRewardsToGive::Give Gave Attribute Reward"));
    }

    return retval;
}

FText FRewardsToGive::GetPreviewText() const {
    TArray<FString> Lines;

    if (XPReward) {
        FText Preview = XPReward->GetPreviewText();
        if (!Preview.IsEmpty()) {
            Lines.Add(Preview.ToString());
        }
    }

    if (ItemReward) {
        FText Preview = ItemReward->GetPreviewText();
        if (!Preview.IsEmpty()) {
            Lines.Add(Preview.ToString());
        }
    }

    if (LootReward) {
        FText Preview = LootReward->GetPreviewText();
        if (!Preview.IsEmpty()) {
            Lines.Add(Preview.ToString());
        }
    }

    if (AbilityReward) {
        FText Preview = AbilityReward->GetPreviewText();
        if (!Preview.IsEmpty()) {
            Lines.Add(Preview.ToString());
        }
    }

    if (AttributeReward) {
        FText Preview = AttributeReward->GetPreviewText();
        if (!Preview.IsEmpty()) {
            Lines.Add(Preview.ToString());
        }
    }

    if (Lines.Num() == 0) {
        return FText::GetEmpty();
    }

    return FText::FromString(FString::Join(Lines, TEXT("\n")));
}

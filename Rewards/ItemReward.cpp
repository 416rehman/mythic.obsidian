// 
#include "ItemReward.h"

#include "Mythic.h"
#include "Itemization/Loot/MythicLootManagerSubsystem.h"

bool UItemReward::Give(FRewardContext &Context) const {
    FItemRewardContext *ItemContext = static_cast<FItemRewardContext *>(&Context);
    checkf(ItemContext, TEXT("Invalid ItemRewardContext"));

    auto ItemDef = this->Item;
    checkf(ItemDef, TEXT("ItemDef is null"));
    
    checkf(ItemContext->PlayerController, TEXT("Player is null"));

    // Get the MythicLootManager Subsystem
    auto MythicLootManager = ItemContext->PlayerController->GetGameInstance()->GetSubsystem<UMythicLootManagerSubsystem>();
    checkf(MythicLootManager, TEXT("MythicLootManager not found"));
    
    auto ItemLvl = ItemContext->ItemLevel;

    // If the drop is private, set the target player to the player controller
    APlayerController *TargetPlayer = ItemContext->bIsPrivate ? ItemContext->PlayerController : nullptr;

    ///////////////// GIVE TO INVENTORY /////////////////////
    if (ItemContext->InventoryProvider) {
        // Create the item instance and give it to the inventory
        auto WorldItem = MythicLootManager->CreateAndGive(ItemDef, this->Quantity, ItemContext->InventoryProvider, TargetPlayer, ItemContext->ItemLevel);
        if (WorldItem) {
            UE_LOG(Myth, Error, TEXT("RewardManager::RequestLootFromSource - No room in inventory so spawned the reward as world item instead"));
        }
        return true;
    }

    ///////////////// SPAWN IN WORLD /////////////////////
    // If the spawn location is not set AND the player is not specified, error because we dont know where to spawn the item
    auto SpawnLoc = ItemContext->SpawnLocation;
    if (SpawnLoc.IsZero()) {
        auto Pawn = ItemContext->PlayerController->GetPawn();
        SpawnLoc = Pawn->GetActorLocation();
    }

    // If inventory is not specified, drop the item as a worlditem instead
    auto WorldItem = MythicLootManager->CreateAndSpawn(ItemDef, SpawnLoc, TargetPlayer, ItemLvl, this->Quantity, 100.0f);
    return WorldItem != nullptr;
}

bool UItemReward::GiveItemReward(UItemReward *Reward, FItemRewardContext Context) {
    return Reward->Give(Context);
}

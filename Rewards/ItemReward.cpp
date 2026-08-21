#include "ItemReward.h"

#include "Mythic.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Itemization/Loot/MythicLootManagerSubsystem.h"
#include "Player/MythicPlayerController.h"

bool UItemReward::Give(FRewardContext &Context) const {
    FItemRewardContext *ItemContext = static_cast<FItemRewardContext *>(&Context);
    checkf(ItemContext, TEXT("Invalid ItemRewardContext"));

    auto ItemDef = this->Item;
    checkf(ItemDef, TEXT("ItemDef is null"));

    checkf(ItemContext->PlayerController, TEXT("Player is null"));

    auto MythicLootManager = ItemContext->PlayerController->GetGameInstance()->GetSubsystem<UMythicLootManagerSubsystem>();
    checkf(MythicLootManager, TEXT("MythicLootManager not found"));

    auto ItemLvl = ItemContext->ItemLevel;

    APlayerController *TargetPlayer = ItemContext->bIsPrivate ? ItemContext->PlayerController : nullptr;

    if (ItemContext->InventoryProvider) {
        auto WorldItem = MythicLootManager->CreateAndGive(ItemDef, this->Quantity, ItemContext->InventoryProvider, TargetPlayer, ItemContext->ItemLevel);
        if (WorldItem) {
            UE_LOG(Myth, Error, TEXT("RewardManager::RequestLootFromSource - No room in inventory so spawned the reward as world item instead"));
        }
        NotifyCelebration(ItemContext->PlayerController);
        return true;
    }

    auto SpawnLoc = ItemContext->SpawnLocation;
    if (SpawnLoc.IsZero()) {
        auto Pawn = ItemContext->PlayerController->GetPawn();
        if (!IsValid(Pawn)) {
            UE_LOG(Myth, Warning,
                   TEXT("ItemReward::Give - ZeroVector spawn location and no pawn to resolve a drop spot (recipient is "
                        "pawn-less); skipping the world-drop of item %s."),
                   *GetNameSafe(ItemDef));
            return false;
        }
        SpawnLoc = Pawn->GetActorLocation();
    }

    auto WorldItem = MythicLootManager->CreateAndSpawn(ItemDef, SpawnLoc, TargetPlayer, ItemLvl, this->Quantity, 100.0f);
    const bool bGranted = WorldItem != nullptr;
    if (bGranted) {
        NotifyCelebration(ItemContext->PlayerController);
    }
    return bGranted;
}

void UItemReward::NotifyCelebration(APlayerController *PC) const {
    if (!bCelebrate || !Item) {
        return;
    }
    if (AMythicPlayerController *MythicPC = Cast<AMythicPlayerController>(PC)) {
        MythicPC->ClientNotifyRewardCelebration(Item, this->Quantity);
    }
}

bool UItemReward::GiveItemReward(UItemReward *Reward, FItemRewardContext Context) {
    return Reward->Give(Context);
}

FText UItemReward::GetPreviewText() const {
    if (!Item) {
        return FText::GetEmpty();
    }
    if (Quantity <= 1) {
        return FText::FromString(Item->GetName());
    }
    return FText::FromString(FString::Printf(TEXT("%dx %s"), Quantity, *Item->GetName()));
}

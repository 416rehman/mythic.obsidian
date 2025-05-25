// 


#include "CraftingStation.h"

#include "AbilitySystemComponent.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/Fragments/Passive/CraftableFragment.h"
#include "Itemization/Loot/MythicLootManagerSubsystem.h"


// Sets default values
ACraftingStation::ACraftingStation() {
    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ACraftingStation::BeginPlay() {
    Super::BeginPlay();

}

void ACraftingStation::ServerCraftItem_Implementation(UItemDefinition *Item, AMythicPlayerController *Player, int32 Amount) {
    // Check if the item is valid
    checkf(Item, TEXT("Item is null."));
    checkf(Amount > 0, TEXT("Amount must be greater than 0."));

    // Make sure Item has a CraftableFragment
    auto CraftableFragment = UCraftableFragment::GetCraftableFragmentFromDefinition(Item);
    checkf(CraftableFragment, TEXT("This item cannot be crafted."));

    // Check if the player can craft the item
    checkf(Player, TEXT("Player is null."));
    auto ASC = Player->GetAbilitySystemComponent();
    checkf(ASC, TEXT("Player does not have an ability system component."));

    // Make sure the player has learned the schematic to craft the item
    if (!ASC->HasMatchingGameplayTag(CraftableFragment->RequiredTag)) {
        UE_LOG(Mythic, Warning, TEXT("Player has not learned the schematic to craft this item."));
        return;
    }

    // Get the player's inventory
    auto Backpack = Player->Backpack;
    checkf(Backpack, TEXT("Player does not have a backpack."));

    auto Hotbar = Player->Hotbar;
    checkf(Hotbar, TEXT("Player does not have a hotbar."));

    // Map to store the items that the player has
    TMap<UItemDefinition *, int32> RequiredItemsInBackpack;
    TMap<UItemDefinition *, int32> RequiredItemsInHotbar;

    // Check if the player has the required items
    for (auto Requirement : CraftableFragment->CraftingRequirements) {
        auto RequiredItem = Requirement.RequiredItem.LoadSynchronous();

        // Check the backpack first
        auto AmountInBackpack = Backpack->GetItemCount(RequiredItem);
        if (AmountInBackpack > 0) {
            RequiredItemsInBackpack.Add(RequiredItem, AmountInBackpack);
        }

        if (AmountInBackpack < Requirement.RequiredAmount) {
            // If not enough in the backpack, check the hotbar
            auto AmountInHotbar = Hotbar->GetItemCount(RequiredItem);
            if (AmountInHotbar > 0) {
                RequiredItemsInHotbar.Add(RequiredItem, AmountInHotbar);
            }

            if (AmountInHotbar + AmountInBackpack < Requirement.RequiredAmount) {
                // Player does not have the required item, so return
                return;
            }
        }
    }

    // Remove the required items
    for (auto Requirement : CraftableFragment->CraftingRequirements) {
        auto RequiredItem = Requirement.RequiredItem.LoadSynchronous();

        auto AmountToRemove = Requirement.RequiredAmount;

        // Remove the items from the backpack
        auto AmountInBackpack = RequiredItemsInBackpack[RequiredItem];
        if (AmountInBackpack > 0) {
            auto AmountToRemoveFromBackpack = FMath::Min(AmountToRemove, AmountInBackpack);
            Backpack->ServerRemoveItemByDefinition(RequiredItem, AmountToRemoveFromBackpack);
            AmountToRemove -= AmountToRemoveFromBackpack;
            UE_LOG(Mythic, Log, TEXT("CraftingStation: Removed %d %s from backpack."), AmountToRemove, *RequiredItem->Name.ToString());
        }

        // Remove the items from the hotbar
        if (AmountToRemove > 0) {
            // If not enough in the backpack, check the hotbar
            auto AmountInHotbar = RequiredItemsInHotbar[RequiredItem];
            auto AmountToRemoveFromHotbar = FMath::Min(AmountToRemove, AmountInHotbar);
            Hotbar->ServerRemoveItemByDefinition(RequiredItem, AmountToRemoveFromHotbar);
        }
    }

    // When all the items are removed, use the loot subsystem to give the player the item
    UMythicLootManagerSubsystem *LootSubsystem = Player->GetGameInstance()->GetSubsystem<UMythicLootManagerSubsystem>();
    checkf(LootSubsystem, TEXT("Loot subsystem is null."));

    // Create the item and put it in the backpack. Targetted to player.
    LootSubsystem->CreateAndGive(Item, Amount, Backpack, Player);

    // Notify the player that they crafted the item
    Player->OnCraftItem(Item, Amount);
}

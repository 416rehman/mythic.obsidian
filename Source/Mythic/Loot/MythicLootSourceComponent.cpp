// 


#include "MythicLootSourceComponent.h"

#include "MythicLootManagerSubsystem.h"
#include "MythicLootSource.h"
#include "MythicLootTable.h"
#include "GameFramework/PlayerState.h"
#include "GameModes/MythicGameMode.h"
#include "Mythic/Inventory/ItemDefinition.h"
#include "Mythic/GAS/AttributeSets/MythicAttributeSet_PlayerProgression.h"


// Sets default values for this component's properties
UMythicLootSourceComponent::UMythicLootSourceComponent() {
    this->SetLootSource(this->LootSource);

    SpawnRadius = 100.0f;
}


// Called when the game starts
void UMythicLootSourceComponent::BeginPlay() {
    Super::BeginPlay();

    this->LootManager = GetOwner()->GetGameInstance()->GetSubsystem<UMythicLootManagerSubsystem>();
}

void UMythicLootSourceComponent::SetLootSource(UMythicLootSource *NewLootSource) {
    this->LootSource = NewLootSource;
}

//  TODO
// Request loot from this source, can be called by the server only.
// Also receives a GameplayAbilitySystemComponent to get any player attributes that may affect the loot (like +10 stone from a mining ability, etc)
void UMythicLootSourceComponent::RequestLoot(UAbilitySystemComponent *AbilitySystemComponent, ELootType Type) {
    // Authority check
    if (GetOwnerRole() != ROLE_Authority) {
        UE_LOG(Mythic, Error, TEXT("LootSourceComponent::RequestLoot - Not authority"));
        return;
    }

    // Get MythicGameMode
    AMythicGameMode *GameMode = GetWorld()->GetAuthGameMode<AMythicGameMode>();
    if (GameMode == nullptr) {
        UE_LOG(Mythic, Error, TEXT("LootSourceComponent::RequestLoot - Could not get MythicGameMode"));
        return;
    }
    // Get the CurveFloat row for the rarity from the MythicGameMode.XpLevelRarityCurve
    auto RarityRowMap = GameMode->XPLevelsCurveTable->GetRowMap();
    auto CommonRarityCurve = RarityRowMap.Find("Common");
    auto UncommonRarityCurve = RarityRowMap.Find("Uncommon");
    auto RareRarityCurve = RarityRowMap.Find("Rare");
    auto LegendaryRarityCurve = RarityRowMap.Find("Legendary");
    auto ExoticRarityCurve = RarityRowMap.Find("Exotic");

    // Get Current XP
    bool bFound;
    auto PlayerExp = AbilitySystemComponent->GetGameplayAttributeValue(UMythicAttributeSet_PlayerProgression::GetXPAttribute(), bFound);

    // Check if we have a loot source
    if (LootSource == nullptr) {
        UE_LOG(Mythic, Error, TEXT("LootSourceComponent::RequestLoot - This component does not have a loot source"));
        return;
    }

    // Player from the ability system component
    if (AbilitySystemComponent == nullptr) {
        UE_LOG(Mythic, Error, TEXT("LootSourceComponent::RequestLoot - Provided ASC is null"));
        return;
    }

    // If the AbilitySystemComponent's owner is a PlayerState, set the owner to the player controller
    APlayerController *PlayerController = Cast<APlayerController>(AbilitySystemComponent->GetOwner());
    if (PlayerController == nullptr) {
        // Cast it to player state instead and then get the player controller
        APlayerState *PlayerState = Cast<APlayerState>(AbilitySystemComponent->GetOwner());
        if (PlayerState != nullptr) {
            PlayerController = PlayerState->GetPlayerController();
        }
        else {
            UE_LOG(Mythic, Error,
                   TEXT("LootSourceComponent::RequestLoot - Could not get player controller from ASC owner (should be a player controller or player state)"));
            return;
        }
    }

    // Loop through each source in the loot source
    for (const FLootSource &Source : LootSource->Sources) {
        // If the source type does not match the requested type, continue
        if (Source.Type != Type) {
            UE_LOG(Mythic, Warning, TEXT("LootSourceComponent::RequestLoot - Skipping source %s, type does not match requested type"),
                   *Source.LootTable->GetName());
            continue;
        }

        // Check if the source has a loot table
        if (Source.LootTable == nullptr) {
            UE_LOG(Mythic, Error, TEXT("LootSourceComponent::RequestLoot - Loot source %s has no loot table"), *Source.LootTable->GetName());
            continue;
        }

        // LootTable item count
        int32 NumItems = Source.LootTable->Entries.Num();

        // if no items in the table, continue
        if (NumItems == 0) {
            UE_LOG(Mythic, Error, TEXT("LootSourceComponent::RequestLoot - Loot source has no items in the loot table %s"), *Source.LootTable->GetName());
            continue;
        }

        // List of spawned world items
        TArray<AMythicWorldItem *> SpawnedItems;

        auto GuaranteedItemCount = 0;

        // Check if this table should be rolled by evaluating its drop chance
        if (FMath::FRand() <= Source.DropChance) {

            // Loop through the items in the rolled table
            for (int32 i = 0; (SpawnedItems.Num() - GuaranteedItemCount) < Source.MaxItems && i < NumItems; i++) {
                auto TableEntry = Source.LootTable->Entries[i];

                // Check if the item class is null
                if (TableEntry.ItemClass == nullptr) {
                    UE_LOG(Mythic, Error, TEXT("LootSourceComponent::RequestLoot - Item class is null in loot table entry"));
                    continue;
                }

                // Check if the item is already spawned
                bool AlreadySpawned = SpawnedItems.ContainsByPredicate([&TableEntry](AMythicWorldItem *Item) {
                    for (auto &ItemInstance : Item->ItemInstances) {
                        if (ItemInstance->GetItemDefinitionObject() == TableEntry.ItemClass.GetDefaultObject()) {
                            return true;
                        }
                    }
                    return false;
                });

                // If the item is already spawned, count it and continue
                if (AlreadySpawned) {
                    continue;
                }

                // Check if the item is guaranteed to drop
                auto bShouldDrop = false;
                if (TableEntry.bGuaranteed) {
                    GuaranteedItemCount++;
                    bShouldDrop = true;
                }
                // Otherwise, if the item is not guaranteed, roll for it using its rarity
                else {
                    // If the item has an override drop chance, use it
                    auto ChanceToDrop = TableEntry.OverrideDropChance;
                    if (ChanceToDrop <= 0) { // Else, use the rarity curve to get the chance to drop
                        auto ItemRarity = TableEntry.ItemClass.GetDefaultObject()->Rarity;

                        if (ItemRarity.ToString() == "Itemization.Rarity.Common") {
                            ChanceToDrop = (*CommonRarityCurve)->Eval(PlayerExp);
                        }
                        else if (ItemRarity.ToString() == "Itemization.Rarity.Uncommon") {
                            ChanceToDrop = (*UncommonRarityCurve)->Eval(PlayerExp);
                        }
                        else if (ItemRarity.ToString() == "Itemization.Rarity.Rare") {
                            ChanceToDrop = (*RareRarityCurve)->Eval(PlayerExp);
                        }
                        else if (ItemRarity.ToString() == "Itemization.Rarity.Legendary") {
                            ChanceToDrop = (*LegendaryRarityCurve)->Eval(PlayerExp);
                        }
                        else if (ItemRarity.ToString() == "Itemization.Rarity.Exotic") {
                            ChanceToDrop = (*ExoticRarityCurve)->Eval(PlayerExp);
                        }
                        else {
                            UE_LOG(Mythic, Error, TEXT("LootSourceComponent::RequestLoot - Item %s has no rarity set"), *TableEntry.ItemClass->GetName());
                            continue;
                        }
                    }

                    // Get the chance to drop
                    float RandomValue = FMath::FRand();

                    // To counteract the probability of the item dropping being higher if there are less items in the table we divide the chance by the number of items in the table
                    ChanceToDrop = (ChanceToDrop / NumItems);
                    bShouldDrop = RandomValue <= ChanceToDrop;

                    UE_LOG(Mythic, Log, TEXT("LootSourceComponent::RequestLoot - Item %s has a %f chance to drop, rolled %f"), *TableEntry.ItemClass->GetName(),
                           ChanceToDrop, RandomValue);
                }


                if (bShouldDrop) {
                    UE_LOG(Mythic, Log, TEXT("LootSourceComponent::RequestLoot - Item %s Chosen to drop"), *TableEntry.ItemClass->GetName());
                    auto stackSize = 0;

                    // If the item is stackable, get a random quantity from the range
                    if (TableEntry.ItemClass.GetDefaultObject()->StackSizeMax > 1) {
                        stackSize = FMath::RandRange(TableEntry.StackRange.Min, TableEntry.StackRange.Max);
                    }
                    // Otherwise, if the item is not stackable, set the stack size to 1 and ignore the quantity range
                    else if (TableEntry.StackRange.Min > 1 || TableEntry.StackRange.Max > 1) {
                        UE_LOG(Mythic, Warning,
                               TEXT("LootSourceComponent::RequestLoot - Item %s is not stackable, but has a quantity range set. Ignoring quantity range."),
                               *TableEntry.ItemClass->GetName());
                    }

                    // Create and spawn the item
                    AMythicWorldItem *world_item = this->LootManager->CreateAndSpawn(TableEntry.ItemClass, stackSize, this->GetOwner()->GetActorLocation(),
                                                                               SpawnRadius, PlayerController, Source.IndividualLoot);
                    if (world_item == nullptr) {
                        // TODO - When mail system is implemented, use LootManager.Create to create the item and send it to the player's mailbox
                        UE_LOG(Mythic, Error, TEXT("LootSourceComponent::RequestLoot - Could not spawn item %s"), *TableEntry.ItemClass->GetName());
                        continue;
                    }

                    // Add the item to the list of spawned items
                    SpawnedItems.Add(world_item);
                }
                else {
                    UE_LOG(Mythic, Log, TEXT("LootSourceComponent::RequestLoot - Item %s Not Chosen to drop"), *TableEntry.ItemClass->GetName());
                }
            }

            if (!this->LootSource->bRollAllSources) {
                // If we are not rolling all sources, break after the first source that passes the drop chance
                break;
            }
        }

    }
}

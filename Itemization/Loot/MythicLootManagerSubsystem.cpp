#include "MythicLootManagerSubsystem.h"
#include "../Loot/MythicWorldItem.h"
#include "GameFramework/GameState.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Mythic.h"
#include "Mythic/Itemization/Inventory/MythicInventoryComponent.h"
#include "System/Modding/ModdingSubsystem.h"

UMythicItemInstance *UMythicLootManagerSubsystem::Create(UItemDefinition *item_def, int32 quantity_if_stackable, AController *TargetRecipient, int32 level) {
    // Check if the item definition is valid
    if (!item_def) {
        UE_LOG(Mythic, Warning, TEXT("Item definition is null"));
        return nullptr;
    }

    // Construct a new item instance
    UMythicItemInstance *ItemInstance = NewObject<UMythicItemInstance>();
    if (!ItemInstance) {
        UE_LOG(Mythic, Warning, TEXT("Failed to create item instance"));
        return nullptr;
    }

    if (TargetRecipient) {
        ItemInstance->SetOwner(TargetRecipient);
    }
    else {
        auto GameMode = GetWorld()->GetAuthGameMode();
        if (!GameMode) {
            UE_LOG(Mythic, Warning, TEXT("GameMode is null. GameMode is only available on the server."));
            return nullptr;
        }
        auto GameState = GameMode->GameState;
        ItemInstance->SetOwner(GameState);
    }
    ItemInstance->Initialize(item_def, quantity_if_stackable, level);

    // Call modding hook before dropping
    if (UModdingSubsystem* ModdingSystem = GetGameInstance()->GetSubsystem<UModdingSubsystem>())
    {
        FItemDropHookData HookData = ModdingSystem->CallItemDropHook(item_def, FVector::ZeroVector);
        
        if (HookData.bCancelDrop) {
            UE_LOG(LogTemp, Log, TEXT("Item drop cancelled by mod"));
            return nullptr;
        }
        
        // Use potentially modified location
        UE_LOG(Mythic, Warning, TEXT("Item drop hook cancelled. New location: %s"), *HookData.DropLocation.ToString());
    }
    
    return ItemInstance;
}

AMythicWorldItem *UMythicLootManagerSubsystem::CreateAndSpawn(UItemDefinition *item_def, const FVector &location, AController *TargetRecipient, int32 level = 0,
                                                              int32 quantity_if_stackable = 1, float radius = 100.0f) {
    if (auto item_instance = Create(item_def, quantity_if_stackable, TargetRecipient, level)) {
        return Spawn(item_instance, location, radius, TargetRecipient);
    }

    return nullptr;
}


AMythicWorldItem *UMythicLootManagerSubsystem::CreateAndGive(UItemDefinition *item_def, int32 quantity_if_stackable, UMythicInventoryComponent *inventory,
                                                             AController *TargetRecipient, int32 level) {
    // Make sure the inventory is valid
    if (!inventory) {
        UE_LOG(Mythic, Warning, TEXT("Inventory is null"));
        return nullptr;
    }
    // Make sure the item definition is valid
    if (!item_def) {
        UE_LOG(Mythic, Warning, TEXT("Item definition is null"));
        return nullptr;
    }
    // Make sure the default world item class is set and the item instance is valid
    if (!DefaultWorldItemClass) {
        UE_LOG(Mythic, Warning, TEXT("Default world item class is not set - Use SetDefaultWorldItemClass to set it."));
        return nullptr;
    }

    // Create a new item instance
    UMythicItemInstance *ItemInstance = Create(item_def, quantity_if_stackable, TargetRecipient, level);

    // Give the item to the player
    return inventory->AddItem(ItemInstance, TargetRecipient);
}

AMythicWorldItem *UMythicLootManagerSubsystem::Spawn(UMythicItemInstance *item, const FVector &location, float radius, AController *TargetRecipient) {
    // Make sure the default world item class is set and the item instance is valid
    if (!DefaultWorldItemClass) {
        UE_LOG(Mythic, Warning, TEXT("Default world item class is not set - Use SetDefaultWorldItemClass to set it."));
        return nullptr;
    }
    if (!item) {
        UE_LOG(Mythic, Warning, TEXT("Item instance is null"));
        return nullptr;
    }

    // Warn if location is not set
    if (location == FVector::ZeroVector) {
        UE_LOG(Mythic, Error, TEXT("Location for the drop is not set"));
    }

    // Create and spawn an actor from the default world item class
    // Construct a new world item and set its RelevantPlayerController then spawn it
    FVector start_location = location + FVector(0, 0, 50);
    auto spawn_params = FActorSpawnParameters();
    if (TargetRecipient) {
        spawn_params.Owner = TargetRecipient;
    }
    else {
        auto GameMode = GetWorld()->GetAuthGameMode();
        if (!GameMode) {
            UE_LOG(Mythic, Warning, TEXT("GameMode is null. GameMode is only available on the server."));
            return nullptr;
        }
        auto GameState = GameMode->GameState;
        spawn_params.Owner = GameState;
    }

    AMythicWorldItem *WorldItem = GetWorld()->SpawnActor<AMythicWorldItem>(DefaultWorldItemClass, start_location, FRotator::ZeroRotator, spawn_params);

    // If the world item is null, return null
    if (!WorldItem) {
        UE_LOG(Mythic, Warning, TEXT("SpawnActor failed"));
        return nullptr;
    }

    // if TargetRecipient is set, only the owner will see the item
    WorldItem->bOnlyRelevantToOwner = TargetRecipient != nullptr;
    WorldItem->SetTargetRecipient(TargetRecipient);

    WorldItem->SetItemInstance(item);

    WorldItem->EmulateDropPhysics(start_location, radius);

    // Return the spawned world item
    return WorldItem;
}

void UMythicLootManagerSubsystem::SetDefaultWorldItemClass(const TSubclassOf<AMythicWorldItem> &NewDefaultWorldItemClass) {
    DefaultWorldItemClass = NewDefaultWorldItemClass;
}

void UMythicLootManagerSubsystem::DestroyWorldItem(AMythicWorldItem *WorldItem, AController *Controller) {
    if (!WorldItem) {
        UE_LOG(Mythic, Warning, TEXT("WorldItem is null"));
        return;
    }

    if (!Controller) {
        UE_LOG(Mythic, Warning, TEXT("Controller is null"));
        return;
    }


    // If WorldItem has no target recipient, anyone can destroy it
    if (!WorldItem->GetTargetRecipient() || WorldItem->GetTargetRecipient() == Controller) {
        WorldItem->Destroy();
    }
    else {
        UE_LOG(Mythic, Warning, TEXT("Player %s does not have permission to destroy WorldItem"), *Controller->GetName());
    }
}

bool UMythicLootManagerSubsystem::ShouldCreateSubsystem(UObject *Outer) const {
    // Should only create the subsystem on the server
    UWorld *World = Outer->GetWorld();
    if (World->WorldType != EWorldType::None && World->GetNetMode() < NM_Client) {
        UE_LOG(Mythic, Warning, TEXT("LootManager created on server"));
        return true;
    }

    UE_LOG(Mythic, Warning, TEXT("LootManager will not be created on client"));
    return false;
}

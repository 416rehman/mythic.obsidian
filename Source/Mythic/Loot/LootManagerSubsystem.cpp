#include "LootManagerSubsystem.h"
#include "NavigationSystem.h"
#include "../Inventory/WorldItem.h"
#include "Kismet/GameplayStatics.h"
#include "Mythic/Inventory/InventoryComponent.h"

UItemInstance *ULootManagerSubsystem::Create(TSubclassOf<UItemDefinition> item_def, int32 quantity_if_stackable, AActor *owner) {
    // Construct a new item instance
    UItemInstance *ItemInstance = NewObject<UItemInstance>();
    ItemInstance->SetOwner(owner);
    ItemInstance->Initialize(item_def, quantity_if_stackable);
    return ItemInstance;
}

AWorldItem *ULootManagerSubsystem::CreateAndSpawn(TSubclassOf<UItemDefinition> item_def, int32 quantity_if_stackable, const FVector &location, float radius,
                                         AActor *owner, bool is_private = false) {
    auto item_instance = Create(item_def, quantity_if_stackable, owner);
    return Spawn(item_instance, location, radius, owner, is_private);
}


AWorldItem *ULootManagerSubsystem::CreateAndGive(TSubclassOf<UItemDefinition> item_def, int32 quantity_if_stackable, UInventoryComponent *inventory, AActor *owner,
                                        bool is_private = false) {
    // Make sure the default world item class is set and the item instance is valid
    if (!DefaultWorldItemClass) {
        UE_LOG(Mythic, Warning, TEXT("Default world item class is not set - Use SetDefaultWorldItemClass to set it."));
        return nullptr;
    }

    // Create a new item instance
    UItemInstance *ItemInstance = Create(item_def, quantity_if_stackable, owner);

    // Give the item to the player
    return inventory->AddItem(ItemInstance, is_private);
}

void ULootManagerSubsystem::EmulateDropPhysics(const FVector &location, float radius, AActor *owner, bool is_private, UE::Math::TVector<double> start_location,
                                      AWorldItem *WorldItem) {
    // Use the suggest projectile velocity function to get a suggested velocity to emulate a drop effect
    // Randomize the location using navmesh
    FNavLocation RandomizedLocation;
    UNavigationSystemV1 *NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (NavSys && NavSys->GetRandomPointInNavigableRadius(location, radius, RandomizedLocation, nullptr)) {
        UE_LOG(Mythic, Warning, TEXT("Randomized location via NavSys: %s"), *RandomizedLocation.Location.ToString());
    }
    else {
        // Manually get a random point in the radius
        RandomizedLocation.Location = location + FVector(FMath::RandRange(-radius, radius), FMath::RandRange(-radius, radius), 0);
        UE_LOG(Mythic, Warning, TEXT("Randomized location manually: %s"), *RandomizedLocation.Location.ToString());

        if (!is_private) {
            // if the z is less than the player's z, do a line trace up, and if a ground is found, set the z to that location
            auto player_z = owner->GetActorLocation().Z;
            if (RandomizedLocation.Location.Z < player_z) {
                auto difference = player_z - RandomizedLocation.Location.Z;
                FHitResult HitResult;
                if (GetWorld()->LineTraceSingleByChannel(HitResult, RandomizedLocation.Location,
                                                         RandomizedLocation.Location + FVector(0, 0, difference),
                                                         ECC_Visibility)) {
                    RandomizedLocation.Location.Z = HitResult.Location.Z + 50;
                }
            }
        }
    }

    FVector SuggestedVelocity;
    UGameplayStatics::SuggestProjectileVelocity_CustomArc(
        GetWorld(),
        SuggestedVelocity,
        start_location,
        RandomizedLocation.Location);

    WorldItem->StaticMesh->SetPhysicsLinearVelocity(SuggestedVelocity);
}

AWorldItem *ULootManagerSubsystem::Spawn(UItemInstance *item, const FVector &location, float radius, AActor *owner, bool is_private = false) {
    // Make sure the default world item class is set and the item instance is valid
    if (!DefaultWorldItemClass) {
        UE_LOG(Mythic, Warning, TEXT("Default world item class is not set - Use SetDefaultWorldItemClass to set it."));
        return nullptr;
    }
    if (!item) {
        UE_LOG(Mythic, Warning, TEXT("Item instance is null"));
        return nullptr;
    }

    UE_LOG(Mythic, Warning, TEXT("Creating WorldItem"));
    
    // Do a sphere overlap to find any overlapping world items
    TArray<FHitResult> OverlappingWorldItems;
    GetWorld()->SweepMultiByChannel(OverlappingWorldItems, location, location, FQuat::Identity, ECC_WorldDynamic, FCollisionShape::MakeSphere(radius));

    // If there are any overlapping world items, check to see if they are of the same rarity
    for (auto &Hit : OverlappingWorldItems) {
        AWorldItem *OverlappingWorldItem = Cast<AWorldItem>(Hit.GetActor());
        if (OverlappingWorldItem) {
            // Existing world item should have at least one item instance
            if (OverlappingWorldItem->ItemInstances.Num() <= 0) {
                continue;
            }

            // If the existing world item's rarity is not the same as the new item's rarity, continue
            if (OverlappingWorldItem->ItemInstances[0]->GetItemDefinitionObject()->Rarity != item->GetItemDefinitionObject()->Rarity) {
                continue;
            }

            // The existing world item should have same private status as the new item
            if (OverlappingWorldItem->IsPrivate() != is_private) {
                continue;
            }

            OverlappingWorldItem->AddItemInstance(item);
            return OverlappingWorldItem;
        }
    }

    // Create and spawn an actor from the default world item class
    // Construct a new world item and set its RelevantPlayerController then spawn it
    auto start_location = location + FVector(0, 0, 50);
    AWorldItem *WorldItem = GetWorld()->SpawnActor<AWorldItem>(DefaultWorldItemClass, start_location, FRotator::ZeroRotator);
    // If the world item is null, return null
    if (!WorldItem) {
        UE_LOG(Mythic, Warning, TEXT("SpawnActor failed"));
        return nullptr;
    }

    WorldItem->SetOwner(owner);
    WorldItem->bOnlyRelevantToOwner = is_private;
    WorldItem->SetIsPrivate(is_private);

    // WorldItem->SetIsPrivate(is_private);
    WorldItem->AddItemInstance(item);

    EmulateDropPhysics(location, radius, owner, is_private, start_location, WorldItem);

    // Return the spawned world item
    return WorldItem;
}

void ULootManagerSubsystem::SetDefaultWorldItemClass(TSubclassOf<AWorldItem> NewDefaultWorldItemClass) {
    DefaultWorldItemClass = NewDefaultWorldItemClass;
}

bool ULootManagerSubsystem::ShouldCreateSubsystem(UObject *Outer) const {
    // Should only create the subsystem on the server
    UWorld *World = Outer->GetWorld();
    if (World->WorldType != EWorldType::None) {
        UE_LOG(Mythic, Warning, TEXT("LootManager created on server"));
        return World->GetNetMode() < NM_Client;
    }

    UE_LOG(Mythic, Warning, TEXT("LootManager will not be created on client"));
    return false;
}

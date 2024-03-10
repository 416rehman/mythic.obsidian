#pragma once

#include "Mythic/Inventory/WorldItem.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LootManagerSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class MYTHIC_API ULootManagerSubsystem : public UGameInstanceSubsystem {
    GENERATED_BODY()

protected:
    // Default World Item class to use when creating dropped items
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slots")
    TSubclassOf<AWorldItem> DefaultWorldItemClass;

public:
    // Creates a new item instance
    UFUNCTION(BlueprintCallable)
    UItemInstance *Create(TSubclassOf<UItemDefinition> item_def, int32 quantity_if_stackable, AActor *owner);
    
    // Create a new loot item and spawn it at a given location. If not stackable, only one item will be spawned.
    UFUNCTION(BlueprintCallable)
    AWorldItem *CreateAndSpawn(TSubclassOf<UItemDefinition> item_def, int32 quantity_if_stackable, const FVector &location, float radius, AActor *owner, bool is_private);

    // Create a new loot item and give it to a player. If not stackable, only one item will be given.
    UFUNCTION(BlueprintCallable)
    AWorldItem *CreateAndGive(TSubclassOf<UItemDefinition> item_def, int32 quantity_if_stackable, UInventoryComponent *inventory, AActor *owner, bool is_private);
    void EmulateDropPhysics(const FVector &location, float radius, AActor *owner, bool is_private, UE::Math::TVector<double> start_location,
                           AWorldItem *WorldItem);

    // Spawns a loot item at a given location.
    AWorldItem *Spawn(UItemInstance *item, const FVector &location, float radius, AActor *owner, bool is_private);

    // Randomizes the loot item to be spawned
    void RandomizeLoot();

    // Set the default world item class
    UFUNCTION(BlueprintCallable)
    void SetDefaultWorldItemClass(TSubclassOf<AWorldItem> NewDefaultWorldItemClass);

    bool ShouldCreateSubsystem(UObject *Outer) const override;
};

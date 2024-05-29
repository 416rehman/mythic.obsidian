#pragma once

#include "Mythic/Inventory/MythicWorldItem.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MythicLootManagerSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class MYTHIC_API UMythicLootManagerSubsystem : public UGameInstanceSubsystem {
    GENERATED_BODY()

protected:
    // Default World Item class to use when creating dropped items
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slots")
    TSubclassOf<AMythicWorldItem> DefaultWorldItemClass;

public:
    // Creates a new item instance
    UFUNCTION(BlueprintCallable)
    UMythicItemInstance *Create(TSubclassOf<UItemDefinition> item_def, int32 quantity_if_stackable, AActor *owner);
    
    // Create a new loot item and spawn it at a given location. If not stackable, only one item will be spawned.
    UFUNCTION(BlueprintCallable)
    AMythicWorldItem *CreateAndSpawn(TSubclassOf<UItemDefinition> item_def, int32 quantity_if_stackable, const FVector &location, float radius, AActor *owner, bool is_private);

    // Create a new loot item and give it to a player. If not stackable, only one item will be given.
    UFUNCTION(BlueprintCallable)
    AMythicWorldItem *CreateAndGive(TSubclassOf<UItemDefinition> item_def, int32 quantity_if_stackable, UMythicInventoryComponent *inventory, AActor *owner, bool is_private);
    void EmulateDropPhysics(const FVector &location, float radius, AActor *owner, bool is_private, UE::Math::TVector<double> start_location,
                           AMythicWorldItem *WorldItem);

    // Spawns a loot item at a given location.
    AMythicWorldItem *Spawn(UMythicItemInstance *item, const FVector &location, float radius, AActor *owner, bool is_private);

    // Randomizes the loot item to be spawned
    void RandomizeLoot();

    // Set the default world item class
    UFUNCTION(BlueprintCallable)
    void SetDefaultWorldItemClass(TSubclassOf<AMythicWorldItem> NewDefaultWorldItemClass);

    bool ShouldCreateSubsystem(UObject *Outer) const override;
};

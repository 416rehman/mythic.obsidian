#pragma once

#include "Mythic/Itemization/Loot/MythicWorldItem.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MythicLootManagerSubsystem.generated.h"

class IInventoryProviderInterface;
class UMythicItemInstance;
class UMythicInventoryComponent;
class UItemDefinition;
/**
 * SERVER-ONLY Subsystem that manages the creation and spawning of loot items.
 * Clients can follow this pipeline: PlayerController -> Server RPC -> Subsystem
 */
UCLASS()
class MYTHIC_API UMythicLootManagerSubsystem : public UGameInstanceSubsystem {
    GENERATED_BODY()

protected:
    // SERVER-ONLY: Default World Item class to use when creating dropped items
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slots")
    TSubclassOf<AMythicWorldItem> DefaultWorldItemClass;

public:
    // SERVER-ONLY: Creates a new item instance
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Loot")
    UMythicItemInstance *Create(UItemDefinition *item_def, int32 quantity_if_stackable, AController *TargetRecipient, int32 level);

    // SERVER-ONLY: Create a new loot item and spawn it at a given location. If not stackable, only one item will be spawned.
    // Should be used when spawning items in the world.
    // If the TargetRecipient is set, they will become owner and only that player will see the item. Otherwise, GameState will be the owner and all players will see the item.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Loot")
    AMythicWorldItem *CreateAndSpawn(UItemDefinition *item_def, const FVector &location, AController *TargetRecipient, int32 level,
                                     int32 quantity_if_stackable, float radius);

    // SERVER-ONLY: Create a new loot item and give it to a player. If not stackable, only one item will be given. If the player's inventory is full, a world item will be spawned and returned.
    // Should be used when giving items to players.
    // If the TargetRecipient is set, they will become owner and only that player will see the item. Otherwise, GameState will be the owner and all players will see the item.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Loot")
    AMythicWorldItem *CreateAndGive(UItemDefinition* ItemDef, int32 QtyIfStackable,
                                    TScriptInterface<IInventoryProviderInterface> InventoryProvider, AController* TargetRecipient, int32 Lvl = 0);

    // SERVER-ONLY: Spawns a loot item at a given location.
    // If the TargetRecipient is set, they will become owner and only that player will see the item. Otherwise, GameState will be the owner and all players will see the item.
    AMythicWorldItem *Spawn(UMythicItemInstance *item, const FVector &location, float radius, AController *TargetRecipient);
    void SetDefaultWorldItemClass(const TSubclassOf<AMythicWorldItem> &NewDefaultWorldItemClass);

    // SERVER-ONLY: Used to destroy a WorldItem. Should be called when the item is picked up or destroyed.
    // WorldItem -> Player -> Destroy
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Loot")
    void DestroyWorldItem(AMythicWorldItem *WorldItem, AController *Controller);

    virtual bool ShouldCreateSubsystem(UObject *Outer) const override;
};

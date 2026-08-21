#pragma once

#include "Mythic/Itemization/Loot/MythicWorldItem.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MythicLootManagerSubsystem.generated.h"

class IInventoryProviderInterface;
class UMythicItemInstance;
class UMythicInventoryComponent;
class UItemDefinition;
UCLASS()
class MYTHIC_API UMythicLootManagerSubsystem : public UGameInstanceSubsystem {
    GENERATED_BODY()

protected:
    // SERVER-ONLY: Default World Item class to use when creating dropped items.
    // Seeded in Initialize from UMythicDeveloperSettings::DefaultWorldItemClass so the class is a project setting.
    // Stays BlueprintReadWrite: BP_MythicGameMode assigns this directly, and that assignment is the ONLY reason loot
    // worked before the setting existed. The setting is the floor, not a replacement — a world without that game mode
    // (a test map, a dedicated-server boot) previously got a null class and silently dropped every item.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slots")
    TSubclassOf<AMythicWorldItem> DefaultWorldItemClass;

    // ── Dropped-loot budget ──
    // Every dropped item is a REPLICATED actor. Nothing used to remove them, so in an open world a long session grew
    // the actor count without bound — replication, net relevancy and actor registration all scale with it. This caps
    // the live drops this subsystem has spawned: past the cap the OLDEST surviving drop is destroyed to make room, so
    // the cost of loot is bounded no matter how long the session runs.
    //
    // Only affects loot THIS subsystem spawned. Placed containers, player stalls and anything a designer put in the
    // level are untouched.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Budget", meta = (ClampMin = "0"))
    int32 MaxLiveWorldItems = 400;

    // Seconds a dropped item survives before it despawns on its own. 0 (default) => no timer, the cap alone bounds it.
    // Set this when drops should also rot away in place rather than only when the cap is hit.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Budget", meta = (ClampMin = "0.0"))
    float WorldItemLifetimeSeconds = 0.0f;


    /** Distance (cm) within which a dropped item's glow runs. 0 disables the pass and leaves every glow running. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Budget", meta = (ClampMin = "0.0"))
    float WorldItemFXDistance = 4000.0f;

    /** Seconds between significance passes. Coarse on purpose — a glow arriving a third of a second late is invisible. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Budget", meta = (ClampMin = "0.05"))
    float WorldItemFXInterval = 0.33f;

private:
    UPROPERTY()
    TArray<TWeakObjectPtr<AMythicWorldItem>> LiveWorldItems;

    void EnforceWorldItemBudget();

    FTimerHandle WorldItemFXTimer;

    void RunWorldItemFXPass();

    void UpdateWorldItemFXTimer();

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

    AMythicWorldItem *Spawn(UMythicItemInstance *item, const FVector &location, float radius, AController *TargetRecipient);

    void SetDefaultWorldItemClass(const TSubclassOf<AMythicWorldItem> &NewDefaultWorldItemClass);

    // SERVER-ONLY: Used to destroy a WorldItem. Should be called when the item is picked up or destroyed.
    // WorldItem -> Player -> Destroy
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Loot")
    void DestroyWorldItem(AMythicWorldItem *WorldItem, AController *Controller);

    virtual bool ShouldCreateSubsystem(UObject *Outer) const override;
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
};

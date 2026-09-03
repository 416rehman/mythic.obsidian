#pragma once

#include "Mythic/Itemization/Loot/MythicWorldItem.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MythicLootManagerSubsystem.generated.h"

class APlayerController;
class IInventoryProviderInterface;
class UMythicItemInstance;
class UMythicInventoryComponent;
class UItemDefinition;
struct FLootTierBonus;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnMythicPreLootRoll, APlayerController *, FLootTierBonus &);

UCLASS()
class MYTHIC_API UMythicLootManagerSubsystem : public UGameInstanceSubsystem {
    GENERATED_BODY()

protected:
    /** Server-only resolved cache of the world-item class configured by UMythicDeveloperSettings. */
    UPROPERTY(Transient)
    TSubclassOf<AMythicWorldItem> DefaultWorldItemClass;

    /**
     * Maximum replicated world-item actors owned by this subsystem; the oldest surviving drop is removed at the cap.
     * Placed containers, player stalls, and designer-authored world actors are not part of this budget. Zero disables
     * spawning world drops.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Budget", meta = (ClampMin = "0"))
    int32 MaxLiveWorldItems = 400;

    /** Seconds a dropped item survives before despawning; zero leaves lifetime management to the live-item cap. */
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
    /**
     * Server. Raised once per crediting controller before a slain enemy's tables roll, after the enemy tier and
     * quantity find are in the bonus; a listener edits the bonus in place. Quest, chest and bounty loot never raise it.
     */
    FOnMythicPreLootRoll OnPreLootRoll;

    /** Creates a server-authoritative item instance for the specified recipient and item level. */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Loot")
    UMythicItemInstance *Create(UItemDefinition *item_def, int32 quantity_if_stackable, AController *TargetRecipient, int32 level);

    /**
     * Creates an item and spawns it into the authoritative world. A recipient makes the drop owner-only; a null
     * recipient assigns it to GameState for shared visibility. Non-stackable definitions always spawn one item.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Loot")
    AMythicWorldItem *CreateAndSpawn(UItemDefinition *item_def, const FVector &location, AController *TargetRecipient, int32 level,
                                     int32 quantity_if_stackable, float radius);

    /**
     * Creates an item and gives it to an authoritative inventory, returning a spawned overflow drop when the inventory
     * is full. A recipient makes overflow owner-only; a null recipient assigns it to GameState for shared visibility.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Loot")
    AMythicWorldItem *CreateAndGive(UItemDefinition* ItemDef, int32 QtyIfStackable,
                                    TScriptInterface<IInventoryProviderInterface> InventoryProvider, AController* TargetRecipient, int32 Lvl = 0);

    AMythicWorldItem *Spawn(UMythicItemInstance *item, const FVector &location, float radius, AController *TargetRecipient);

    /** Destroys an authoritative world item after verifying that the requesting controller may interact with it. */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Loot")
    void DestroyWorldItem(AMythicWorldItem *WorldItem, AController *Controller);

    virtual bool ShouldCreateSubsystem(UObject *Outer) const override;
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
};

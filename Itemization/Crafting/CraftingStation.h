// 

#pragma once

#include "CoreMinimal.h"
#include "CraftingComponent.h"
#include "GameFramework/Actor.h"
#include "Interaction/IMythicInteractable.h"
#include "Player/MythicPlayerController.h"
#include "CraftingStation.generated.h"

class UCraftableFragment;
class UCraftingComponent;

UCLASS()
class MYTHIC_API ACraftingStation : public AActor, public IMythicInteractable, public IInventoryProviderInterface {
public:
    virtual TArray<UMythicInventoryComponent *> GetAllInventoryComponents() const override;
    virtual UAbilitySystemComponent * GetSchematicsASC() const override;

private:
    GENERATED_BODY()

public:
    // Sets default values for this actor's properties
    ACraftingStation();

protected:
    // Crafting Component
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crafting")
    UCraftingComponent *CraftingComponent;

    // Inventory component to store crafted items
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
    UMythicInventoryComponent *InventoryComponent;

    UFUNCTION()
    void OnItemCrafted(FCraftingQueueEntry Item);

    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

    /// Server Only - Craft an item. Checks the player's inventory for the required items and removes them if they have them.
    /// @param ItemDefinition The item to craft
    /// @param Quantity The amount of the item to craft
    /// @param RequestingActor The actor requesting the craft, must implement IInventoryProviderInterface
    /// 
    UFUNCTION(Blueprintable, BlueprintCallable, Server, Reliable)
    void ServerCraftItem(UItemDefinition *ItemDefinition, int32 Quantity = 1, const AActor* RequestingActor = nullptr);
};

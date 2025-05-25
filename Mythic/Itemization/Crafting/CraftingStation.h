// 

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/IMythicInteractable.h"
#include "Player/MythicPlayerController.h"
#include "CraftingStation.generated.h"

class UCraftableFragment;

UCLASS()
class MYTHIC_API ACraftingStation : public AActor, public IMythicInteractable {
    GENERATED_BODY()

public:
    // Sets default values for this actor's properties
    ACraftingStation();

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

    /// Server Only - Craft an item. Checks the player's inventory for the required items and removes them if they have them.
    /// @param Item The item to craft
    /// @param Amount The amount of the item to craft
    /// @param Player The player crafting
    /// 
    UFUNCTION(Server, Reliable)
    void ServerCraftItem(UItemDefinition *Item, AMythicPlayerController *Player, int32 Amount = 1);
};

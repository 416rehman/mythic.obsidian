// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Interface.h"
#include "AbilitySystemComponent.h"
#include "Inventory/MythicInventoryComponent.h"
#include "Inventory/MythicItemInstance.h"
#include "InventoryProviderInterface.generated.h"

#define UE_API GAMEPLAYABILITIES_API


/** Interface for actors that expose access to an inventory component */
UINTERFACE(MinimalAPI, Blueprintable)
class UInventoryProviderInterface : public UInterface {
    GENERATED_UINTERFACE_BODY()
};

class IInventoryProviderInterface {
    GENERATED_IINTERFACE_BODY()
    /** Returns ALL the inventory components owned by this actor */
    virtual TArray<UMythicInventoryComponent *> GetAllInventoryComponents() const = 0;

    // Returns the ASC that holds the schematics learned by this actor
    virtual UAbilitySystemComponent *GetSchematicsASC() const = 0;

    /** Returns the first inventory component that will allow the given item type to be placed in it, or nullptr if no such inventory exists */
    virtual UMythicInventoryComponent *GetInventoryForItemType(const FGameplayTag &ItemType) const;

    // Get inventory for item definition
    virtual UMythicInventoryComponent *GetInventoryForItemDefinition(const UItemDefinition *ItemDefinition) const;

    // Get inventory for item instance
    virtual UMythicInventoryComponent *GetInventoryForItemInstance(const UMythicItemInstance *ItemInstance) const;

    // Get inventory for world item
    virtual UMythicInventoryComponent *GetInventoryForWorldItem(const AMythicWorldItem *WorldItem) const;
};

#undef UE_API

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


UINTERFACE(MinimalAPI, Blueprintable)
class UInventoryProviderInterface : public UInterface {
    GENERATED_UINTERFACE_BODY()
};

class IInventoryProviderInterface {
    GENERATED_IINTERFACE_BODY()
    virtual TArray<UMythicInventoryComponent *> GetAllInventoryComponents() const = 0;

    virtual UAbilitySystemComponent *GetSchematicsASC() const = 0;

    virtual UMythicInventoryComponent *GetInventoryForItemType(const FGameplayTag &ItemType) const;

    virtual UMythicInventoryComponent *GetInventoryForItemDefinition(const UItemDefinition *ItemDefinition) const;

    virtual UMythicInventoryComponent *GetInventoryForItemInstance(const UMythicItemInstance *ItemInstance) const;

    virtual UMythicInventoryComponent *GetInventoryForWorldItem(const AMythicWorldItem *WorldItem) const;
};

#undef UE_API

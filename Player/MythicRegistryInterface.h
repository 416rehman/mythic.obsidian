#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MythicRegistryInterface.generated.h"

class UAbilitySystemComponent;
class UMythicInventoryComponent;
class UMythicLifeComponent;

UINTERFACE(MinimalAPI, BlueprintType)
class UMythicRegistryInterface : public UInterface {
    GENERATED_BODY()
};

class MYTHIC_API IMythicRegistryInterface {
    GENERATED_BODY()

public:
    // return the cached ability system component directly
    virtual UAbilitySystemComponent* GetCachedASC() const = 0;

    // return the cached inventory component directly
    virtual UMythicInventoryComponent* GetCachedInventory() const = 0;

    // return the cached life component directly
    virtual UMythicLifeComponent* GetCachedLife() const = 0;
};

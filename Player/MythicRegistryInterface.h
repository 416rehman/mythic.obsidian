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
    virtual UAbilitySystemComponent* GetCachedASC() const = 0;

    virtual UMythicInventoryComponent* GetCachedInventory() const = 0;

    virtual UMythicLifeComponent* GetCachedLife() const = 0;
};

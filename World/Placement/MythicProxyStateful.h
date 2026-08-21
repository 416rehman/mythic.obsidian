#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MythicProxyStateful.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UMythicProxyStateful : public UInterface {
    GENERATED_BODY()
};

class MYTHIC_API IMythicProxyStateful {
    GENERATED_BODY()

public:
    // Called just before the actor is put to sleep. Return everything that must survive.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Proxy")
    int32 GetProxyStateFlags() const;
    virtual int32 GetProxyStateFlags_Implementation() const {
        return 0;
    }

    // Called on a freshly promoted actor, before it is used. Restore what was saved.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Proxy")
    void ApplyProxyStateFlags(int32 StateFlags);
    virtual void ApplyProxyStateFlags_Implementation(int32 StateFlags) {
    }
};

// 

#pragma once

#include "CoreMinimal.h"
#include "LuaState.h"
#include "MythicLuaState.generated.h"

class UModdingSubsystem;
/**
 * 
 */
UCLASS()
class MYTHIC_API UMythicLuaState : public ULuaState {
    GENERATED_BODY()

public:
    UMythicLuaState();

    // Override initialization to setup mod-specific environment
    virtual void ReceiveLuaStateInitialized_Implementation() override;

    //~ API Exposed to Lua

    // Must have UFUNCTION
    UFUNCTION()
    void LogMessage(const FLuaValue &Message);

    //~ End API Exposed to Lua

    // Reference to modding subsystem
    UPROPERTY()
    UModdingSubsystem *ModdingSubsystem;

    // Helper functions
    FLuaValue GetGlobalField(const FString &FieldName);
    bool LuaCallFunction(FLuaValue Function, const TArray<FLuaValue> &Args, FLuaValue &Result);
};

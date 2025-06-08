// ModdingSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "HAL/FileManager.h"
#include "ModdingSubsystem.generated.h"

class UMythicLuaState;
class UItemDefinition;

// Hook event structures
USTRUCT(BlueprintType)
struct FItemDropHookData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    UItemDefinition* Item;
    
    UPROPERTY(BlueprintReadWrite)
    FVector DropLocation;
    
    UPROPERTY(BlueprintReadWrite)
    bool bCancelDrop = false;
};

UCLASS()
class MYTHIC_API UModdingSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    // Main Lua state for all mods
    UPROPERTY(BlueprintReadOnly, Category = "Modding")
    UMythicLuaState* MythicLuaState;

    // Load all mod scripts from the Mods directory
    UFUNCTION(BlueprintCallable, Category = "Modding")
    void LoadModScripts();
    
    //~ Hooks
    UFUNCTION(BlueprintCallable, Category = "Modding")
    FItemDropHookData CallItemDropHook(UItemDefinition* Item, FVector DropLocation);

    // ~ End Hooks

protected:
    // Currently loaded mod files
    UPROPERTY()
    TArray<FString> LoadedModFiles;

    // File watcher handle
    FDelegateHandle FileWatcherHandle;
    
    // Timer for delayed reloading (to avoid multiple rapid reloads)
    FTimerHandle ReloadTimerHandle;
};
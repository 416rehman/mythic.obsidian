// ModdingSubsystem.cpp
#include "ModdingSubsystem.h"

#include "Mythic.h"
#include "MythicLuaState.h"
#include "Engine/World.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Sound/SoundBase.h"

void UModdingSubsystem::Initialize(FSubsystemCollectionBase &Collection) {
    Super::Initialize(Collection);

    // Create our custom Lua state for mods
    MythicLuaState = NewObject<UMythicLuaState>(this);
    MythicLuaState->ModdingSubsystem = this; // Set reference so Lua can call back
    MythicLuaState->GetLuaState(GetWorld());

    // Set up the Lua state
    MythicLuaState->bAddProjectContentDirToPackagePath = true;
    MythicLuaState->AppendProjectContentDirSubDir.Add(TEXT("Mods"));
    UE_LOG(Mythic_Mods, Log, TEXT("Modding Subsystem Initialized"));

    LoadModScripts();
}

void UModdingSubsystem::LoadModScripts() {
    if (!MythicLuaState)
        return;

    FString ModsDirectory = FPaths::ProjectContentDir() + TEXT("Mods/");

    // Clear previously loaded files
    LoadedModFiles.Empty();

    UE_LOG(Mythic_Mods, Log, TEXT("Loading mod scripts from: %s"), *ModsDirectory);

    // Find all .lua files in the mods directory
    TArray<FString> LuaFiles;
    IFileManager::Get().FindFilesRecursive(LuaFiles, *ModsDirectory, TEXT("*.lua"), true, false);

    for (const FString &LuaFile : LuaFiles) {
        UE_LOG(Mythic_Mods, Log, TEXT("Loading mod script: %s"), *LuaFile);
        
        // Run the Lua file
        if (MythicLuaState->RunFile(LuaFile, false, 0, true)) {
            LoadedModFiles.Add(LuaFile);
            UE_LOG(Mythic_Mods, Log, TEXT("Successfully loaded mod: %s"), *LuaFile);
        }
        else {
            UE_LOG(Mythic_Mods, Error, TEXT("Failed to load mod: %s - %s"), *LuaFile, *MythicLuaState->LastError);
        }
    }

    UE_LOG(Mythic_Mods, Log, TEXT("Loaded %d mod scripts"), LoadedModFiles.Num());
}

FItemDropHookData UModdingSubsystem::CallItemDropHook(UItemDefinition *Item, FVector DropLocation) {
    FItemDropHookData HookData;
    HookData.Item = Item;
    HookData.DropLocation = DropLocation;

    if (!MythicLuaState)
        return HookData;

    // Check if the OnItemDropped function exists in Lua
    MythicLuaState->GetGlobal("OnItemDropped");
    if (MythicLuaState->ToLuaValue(1).Type == ELuaValueType::Function) {
        // Prepare arguments
        TArray<FLuaValue> Args;

        // Item name
        FLuaValue ItemName;
        ItemName.Type = ELuaValueType::String;
        ItemName.String = Item ? Item->GetName() : TEXT("Unknown");
        Args.Add(ItemName);

        // Location
        FLuaValue LocationX, LocationY, LocationZ;
        LocationX.Type = ELuaValueType::Number;
        LocationX.Number = DropLocation.X;
        LocationY.Type = ELuaValueType::Number;
        LocationY.Number = DropLocation.Y;
        LocationZ.Type = ELuaValueType::Number;
        LocationZ.Number = DropLocation.Z;
        Args.Add(LocationX);
        Args.Add(LocationY);
        Args.Add(LocationZ);

        // Call the Lua function - it should return a boolean indicating whether to cancel the drop
        FLuaValue ReturnValue;
        if (MythicLuaState->Call(4, ReturnValue, 1)) {
            // Check if the function returned false to cancel the drop
            if (ReturnValue.Type == ELuaValueType::Bool && !ReturnValue.Bool) {
                HookData.bCancelDrop = true;
            }
        }
    }

    MythicLuaState->Pop();
    return HookData;
}
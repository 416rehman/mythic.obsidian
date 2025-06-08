// 


#include "MythicLuaState.h"

#include "ModdingSubsystem.h"
#include "Mythic.h"
#include "World/EnvironmentController/MythicEnvironmentSubsystem.h"


// UMythicLuaState Implementation
UMythicLuaState::UMythicLuaState() {
    ModdingSubsystem = nullptr;
 
    Table.Add("LogMessage", FLuaValue::Function(GET_FUNCTION_NAME_CHECKED(UMythicLuaState, LogMessage)));
   
    // Add some utilities to the VM, but not all of them so modders can't break out of the game
    bLuaOpenLibs = false;
    LuaLibsLoader.bLoadBase = true;
    LuaLibsLoader.bLoadMath = true;
    LuaLibsLoader.bLoadString = true;
    LuaLibsLoader.bLoadTable = true;

    HookInstructionCount = 999999;
    bEnableCountHook = true;
}

void UMythicLuaState::ReceiveLuaStateInitialized_Implementation() {
    Super::ReceiveLuaStateInitialized_Implementation();

    // Get reference to modding subsystem
    if (UGameInstance *GameInstance = GetWorld()->GetGameInstance()) {
        ModdingSubsystem = GameInstance->GetSubsystem<UModdingSubsystem>();
    }
}

void UMythicLuaState::LogMessage(const FLuaValue &Message) {
    if (Message.Type == ELuaValueType::String) {
        UE_LOG(Mythic_Mods, Log, TEXT("Mod: %s"), *Message.String);
    }
}

// Helper function to get global Lua fields
FLuaValue UMythicLuaState::GetGlobalField(const FString& FieldName) {
    FLuaValue Result;
    
    // Get the global table and look for the field
    int32 FieldCount = GetFieldFromTree(FieldName, true);
    if (FieldCount > 0) {
        Result = ToLuaValue(-1);
        Pop(); // Clean up the stack
    }
    
    return Result;
}

// Helper function to call Lua functions
bool UMythicLuaState::LuaCallFunction(FLuaValue Function, const TArray<FLuaValue>& Args, FLuaValue& Result) {
    if (Function.Type != ELuaValueType::Function) {
        return false;
    }

    // Push the function onto the stack
    FromLuaValue(Function);
    
    // Push arguments
    for (const FLuaValue& Arg : Args) {
        FLuaValue ArgCopy = Arg;
        FromLuaValue(ArgCopy);
    }
    
    // Call the function
    return PCall(Args.Num(), Result, 1);
}
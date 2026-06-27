// Copyright Epic Games, Inc. All Rights Reserved.

#include "Mythic.h"
#include "Modules/ModuleManager.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "GameplayDebugger.h"
#include "Debug/GameplayDebuggerCategory_MythicLivingWorld.h"
#endif

/**
 * FMythicGameModule
 */
class FMythicGameModule : public FDefaultGameModuleImpl {
    virtual void StartupModule() override {
#if WITH_GAMEPLAY_DEBUGGER
        // Register the Living World MASS visualizer as a Gameplay Debugger category (apostrophe key to toggle in PIE).
        IGameplayDebugger &GameplayDebugger = IGameplayDebugger::Get();
        GameplayDebugger.RegisterCategory(
            "MythicLivingWorld",
            IGameplayDebugger::FOnGetCategory::CreateStatic(&FGameplayDebuggerCategory_MythicLivingWorld::MakeInstance),
            EGameplayDebuggerCategoryState::EnabledInGameAndSimulate);
        GameplayDebugger.NotifyCategoriesChanged();
#endif
    }

    virtual void ShutdownModule() override {
#if WITH_GAMEPLAY_DEBUGGER
        if (IGameplayDebugger::IsAvailable()) {
            IGameplayDebugger::Get().UnregisterCategory("MythicLivingWorld");
        }
#endif
    }
};

IMPLEMENT_PRIMARY_GAME_MODULE(FMythicGameModule, Mythic, "Mythic");

DEFINE_LOG_CATEGORY(Myth)
DEFINE_LOG_CATEGORY(Myth_Environment)
DEFINE_LOG_CATEGORY(Myth_Mods)
DEFINE_LOG_CATEGORY(MythSaveLoad)

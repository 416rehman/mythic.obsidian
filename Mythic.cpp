// Copyright Epic Games, Inc. All Rights Reserved.

#include "Mythic.h"
#include "Modules/ModuleManager.h"

/**
 * FMythicGameModule
 */
class FMythicGameModule : public FDefaultGameModuleImpl {
    virtual void StartupModule() override {}

    virtual void ShutdownModule() override {}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FMythicGameModule, Mythic, "Mythic");

DEFINE_LOG_CATEGORY(Myth)
DEFINE_LOG_CATEGORY(Myth_Environment)
DEFINE_LOG_CATEGORY(Myth_Mods)
DEFINE_LOG_CATEGORY(MythSaveLoad)

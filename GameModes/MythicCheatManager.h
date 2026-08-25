#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "GameplayTagContainer.h"
#include "MythicCheatManager.generated.h"

class UItemDefinition;

UCLASS()
class MYTHIC_API UMythicCheatManager : public UCheatManager {
    GENERATED_BODY()

public:
    UFUNCTION(Exec)
    void MythOpenMenu(const FString &PageId = TEXT(""));

    UFUNCTION(Exec)
    void MythHelp();

    UFUNCTION(Exec)
    void MythObjective(const FString &Text, int32 Have = 0, int32 Need = 1, int32 bDone = 0, const FString &Quest = TEXT(""));


    /**
     * Open the settings screen directly.
     *
     * Settings lives on the escape menu, two key presses deep, and automated input into a running game is
     * unreliable enough that "did the screen actually draw" kept going unverified. One command reaches it
     * from anywhere, including the death screen - which is the case that moved it off the player menu.
     */
    UFUNCTION(Exec)
    void MythSettings();

    UFUNCTION(Exec)
    void MythSaveCharacter(
        const FString &SlotName = TEXT("DebugCharacter"));

    UFUNCTION(Exec)
    void MythLoadCharacter(
        const FString &SlotName = TEXT("DebugCharacter"));

    UFUNCTION(Exec)
    void MythSaveWorld(
        const FString &SlotName = TEXT("DebugWorld"));

    UFUNCTION(Exec)
    void MythLoadWorld(
        const FString &SlotName = TEXT("DebugWorld"));

    UFUNCTION(Exec)
    void MythListSaves();


    UFUNCTION(Exec)
    void MythListWeather();

    UFUNCTION(Exec)
    void MythSetWeather(const FString &WeatherTag);

    UFUNCTION(Exec)
    void MythSetWeatherInstant(const FString &WeatherTag);


    UFUNCTION(Exec)
    void MythSetTime(float Hour);

    UFUNCTION(Exec)
    void MythAddTime(float Hours);

    UFUNCTION(Exec)
    void MythPauseTime();

    UFUNCTION(Exec)
    void MythResumeTime();

    UFUNCTION(Exec)
    void MythSetTimeSpeed(float NewFrequency);


    UFUNCTION(Exec)
    void MythListItems();

    UFUNCTION(Exec)
    void MythGiveItem(const FString &ItemName, int32 Count = 1, int32 ItemLevel = 1);

    UFUNCTION(Exec)
    void MythListTalents();

    // Grants a talent's ability directly, so its verb can be exercised without farming a Rare-or-better drop
    // that happens to roll it.
    UFUNCTION(Exec)
    void MythGiveTalent(const FString &TalentName);

    UFUNCTION(Exec)
    void MythListSkills();

    // Runs the real levelling route Count times. Server/standalone only - a skill levels on the server or nowhere.
    UFUNCTION(Exec)
    void MythPracticeSkill(const FString &SkillName, int32 Count = 1);

    // Hands over levels without the practice, for reaching a build faster than a test session allows.
    UFUNCTION(Exec)
    void MythLevelSkill(const FString &SkillName, int32 Levels = 1);

    UFUNCTION(Exec)
    void MythClearInventory();

    UFUNCTION(Exec)
    void MythStatus(const FString &StatusName, int32 bOn = 1);

    UFUNCTION(Exec)
    void MythClearStatus();

    UFUNCTION(Exec)
    void MythBuildup(const FString &StatusName, float Amount = 25.0f);

    UFUNCTION(Exec)
    void MythStatusList();

    UFUNCTION(Exec)
    void MythProcChance(const FString &StatusName, float Chance = 1.0f);


    UFUNCTION(Exec)
    void MythListAttributes();

    UFUNCTION(Exec)
    void MythSetAttribute(const FString &AttributeName, float Value);


    UFUNCTION(Exec)
    void MythListProficiencies();

    UFUNCTION(Exec)
    void MythGiveProficiency(const FString &ProficiencyName, float Amount);


    UFUNCTION(Exec)
    void MythLivingWorldStatus();

    UFUNCTION(Exec)
    void MythLivingWorldFactions();

    UFUNCTION(Exec)
    void MythLivingWorldTerritory();

    UFUNCTION(Exec)
    void MythLivingWorldPopulation();

    UFUNCTION(Exec)
    void MythLivingWorldSettlements();

    UFUNCTION(Exec)
    void MythLivingWorldTransferSettlement(int32 SettlementId, int32 FactionIndex);

    UFUNCTION(Exec)
    void MythToggleLivingWorldDebug();


    UFUNCTION(Exec)
    void MythLivingWorldSimulateEvent(const FString &ActionTag, const FString &MoralAxis, float MoralValue);

    UFUNCTION(Exec)
    void MythLivingWorldPressure();

    UFUNCTION(Exec)
    void MythLivingWorldSignificance();

    UFUNCTION(Exec)
    void MythLivingWorldForcePromote();


    UFUNCTION(Exec)
    void MythLivingWorldSocialGraph();

    UFUNCTION(Exec)
    void MythLivingWorldSchemes();

    UFUNCTION(Exec)
    void MythLivingWorldEncounters();

    UFUNCTION(Exec)
    void MythLivingWorldParty();


    UFUNCTION(Exec)
    void MythDeployPlaceable(int32 SlotIndex = 0);


    UFUNCTION(Exec)
    void MythToggleCoopDown();

    UFUNCTION(Exec)
    void MythReviveSelf();


    UFUNCTION(Exec)
    void MythAdvanceWorldTier();

    UFUNCTION(Exec)
    void MythDumpActions();
};

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
    void MythListRunes();

    // Slot -1 takes the first empty open socket. Runs the real server verb, so every refusal is the real refusal.
    UFUNCTION(Exec)
    void MythGiveRune(const FString &Name, int32 Slot = -1);

    UFUNCTION(Exec)
    void MythRemoveRune(int32 Slot);

    UFUNCTION(Exec)
    void MythMoveRune(int32 From, int32 To);

    // Both directions: closing a socket takes out whatever it held. The first socket never closes.
    UFUNCTION(Exec)
    void MythRuneSlots(int32 Count);

    // Earns every deed the rune library gates on, so the picker offers all of them.
    UFUNCTION(Exec)
    void MythUnlockAllRunes();

    UFUNCTION(Exec)
    void MythUnlockAllRuneSlots();

    // Reads the replicated HUD rows, so it answers on a client as well as the server.
    UFUNCTION(Exec)
    void MythRuneHud();

    UFUNCTION(Exec)
    void MythRuneRolls();

    // Pushes the deed's stat counters to their thresholds through the ledger, so the whole unlock chain runs.
    UFUNCTION(Exec)
    void MythGiveDeed(const FString &AchievementName);

    UFUNCTION(Exec)
    void MythClearDeed(const FString &AchievementName);

    UFUNCTION(Exec)
    void MythRecordStat(const FString &StatTag, int32 Delta = 1);

    UFUNCTION(Exec)
    void MythSetSeason(const FString &Season);

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

    // Goes through the life component so the death latch follows the write; MythSetAttribute Health leaves it stale.
    // A value above 1 reads as a percent.
    UFUNCTION(Exec)
    void MythSetHealth(float Fraction);


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

private:
    /** Drives one deed's counters and story tags through their real writers. Returns whether it ended up earned. */
    bool Cheat_GrantDeed(class AMythicPlayerState *PS, const class UMythicAchievementDefinition *Def, bool bVerbose);
};

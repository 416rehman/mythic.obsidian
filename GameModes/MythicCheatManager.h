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
    void MythGiveItem(const FString &ItemName, int32 Count = 1);

    UFUNCTION(Exec)
    void MythClearInventory();

    UFUNCTION(Exec)
    void MythStatus(const FString &StatusName, int32 bOn = 1);

    UFUNCTION(Exec)
    void MythClearStatus();


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
};

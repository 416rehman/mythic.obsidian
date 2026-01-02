#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "GameplayTagContainer.h"
#include "MythicCheatManager.generated.h"

class UItemDefinition;

/**
 * Mythic Cheat Manager
 * Provides console commands for debugging game systems.
 * 
 * Type "Help" in console to see all available commands.
 */
UCLASS()
class MYTHIC_API UMythicCheatManager : public UCheatManager {
    GENERATED_BODY()

public:
    // Show all available cheat commands
    // Example: Help
    UFUNCTION(Exec)
    void MythHelp();

    // === SAVE SYSTEM ===

    // Save character to slot
    // Example: SaveCharacter MyCharacter
    UFUNCTION(Exec)
    void MythSaveCharacter(const FString &SlotName = TEXT("DebugCharacter"));

    // Load character from slot
    // Example: LoadCharacter MyCharacter
    UFUNCTION(Exec)
    void MythLoadCharacter(const FString &SlotName = TEXT("DebugCharacter"));

    // Save world state
    // Example: SaveWorld MyWorld
    UFUNCTION(Exec)
    void MythSaveWorld(const FString &SlotName = TEXT("DebugWorld"));

    // Load world state
    // Example: LoadWorld MyWorld
    UFUNCTION(Exec)
    void MythLoadWorld(const FString &SlotName = TEXT("DebugWorld"));

    // List all save files
    // Example: ListSaves
    UFUNCTION(Exec)
    void MythListSaves();

    // === WEATHER ===

    // List available weather types
    // Example: ListWeather
    UFUNCTION(Exec)
    void MythListWeather();

    // Set weather (transitions)
    // Example: SetWeather Weather.Rain
    UFUNCTION(Exec)
    void MythSetWeather(const FString &WeatherTag);

    // Set weather instantly
    // Example: SetWeatherInstant Weather.Clear
    UFUNCTION(Exec)
    void MythSetWeatherInstant(const FString &WeatherTag);

    // === TIME OF DAY ===

    // Set the time of day to a specific hour
    // Example: SetTime 14.5 (2:30 PM)
    UFUNCTION(Exec)
    void MythSetTime(float Hour);

    // Add hours to the current time
    // Example: AddTime 1.0
    UFUNCTION(Exec)
    void MythAddTime(float Hours);

    // Pause time
    // Example: PauseTime
    UFUNCTION(Exec)
    void MythPauseTime();

    // Resume time
    // Example: ResumeTime
    UFUNCTION(Exec)
    void MythResumeTime();

    // Set time update speed
    // Example: SetTimeSpeed 0.01 (Fast)
    // Example: SetTimeSpeed 1.0 (Slow)
    UFUNCTION(Exec)
    void MythSetTimeSpeed(float NewFrequency);


    // === ITEMS ===

    // List all item definitions
    // Example: ListItems
    UFUNCTION(Exec)
    void MythListItems();

    // Give item by name (partial match)
    // Example: GiveItem Sword 5
    // Example: GiveItem IronOre 10
    UFUNCTION(Exec)
    void MythGiveItem(const FString &ItemName, int32 Count = 1);

    // Clear all items from inventory
    // Example: ClearInventory
    UFUNCTION(Exec)
    void MythClearInventory();

    // === ATTRIBUTES ===

    // List all attributes and their values
    // Example: ListAttributes
    UFUNCTION(Exec)
    void MythListAttributes();

    // Set an attribute value
    // Example: SetAttribute MaxHealth 500
    // Example: SetAttribute CurrentStamina 100
    UFUNCTION(Exec)
    void MythSetAttribute(const FString &AttributeName, float Value);

    // === PROFICIENCIES ===

    // List proficiencies and progress
    // Example: ListProficiencies
    UFUNCTION(Exec)
    void MythListProficiencies();

    // Give proficiency progress
    // Example: GiveProficiency Combat 100
    // Example: GiveProficiency Mining 50
    UFUNCTION(Exec)
    void MythGiveProficiency(const FString &ProficiencyName, float Amount);
};

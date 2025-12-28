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
    void Help();

    // === SAVE SYSTEM ===

    // Save character to slot
    // Example: SaveCharacter MyCharacter
    UFUNCTION(Exec)
    void SaveCharacter(const FString &SlotName = TEXT("DebugCharacter"));

    // Load character from slot
    // Example: LoadCharacter MyCharacter
    UFUNCTION(Exec)
    void LoadCharacter(const FString &SlotName = TEXT("DebugCharacter"));

    // Save world state
    // Example: SaveWorld MyWorld
    UFUNCTION(Exec)
    void SaveWorld(const FString &SlotName = TEXT("DebugWorld"));

    // Load world state
    // Example: LoadWorld MyWorld
    UFUNCTION(Exec)
    void LoadWorld(const FString &SlotName = TEXT("DebugWorld"));

    // List all save files
    // Example: ListSaves
    UFUNCTION(Exec)
    void ListSaves();

    // === WEATHER ===

    // List available weather types
    // Example: ListWeather
    UFUNCTION(Exec)
    void ListWeather();

    // Set weather (transitions)
    // Example: SetWeather Weather.Rain
    UFUNCTION(Exec)
    void SetWeather(const FString &WeatherTag);

    // Set weather instantly
    // Example: SetWeatherInstant Weather.Clear
    UFUNCTION(Exec)
    void SetWeatherInstant(const FString &WeatherTag);

    // === ITEMS ===

    // List all item definitions
    // Example: ListItems
    UFUNCTION(Exec)
    void ListItems();

    // Give item by name (partial match)
    // Example: GiveItem Sword 5
    // Example: GiveItem IronOre 10
    UFUNCTION(Exec)
    void GiveItem(const FString &ItemName, int32 Count = 1);

    // Clear all items from inventory
    // Example: ClearInventory
    UFUNCTION(Exec)
    void ClearInventory();

    // === ATTRIBUTES ===

    // List all attributes and their values
    // Example: ListAttributes
    UFUNCTION(Exec)
    void ListAttributes();

    // Set an attribute value
    // Example: SetAttribute MaxHealth 500
    // Example: SetAttribute CurrentStamina 100
    UFUNCTION(Exec)
    void SetAttribute(const FString &AttributeName, float Value);

    // === PROFICIENCIES ===

    // List proficiencies and progress
    // Example: ListProficiencies
    UFUNCTION(Exec)
    void ListProficiencies();

    // Give proficiency progress
    // Example: GiveProficiency Combat 100
    // Example: GiveProficiency Mining 50
    UFUNCTION(Exec)
    void GiveProficiency(const FString &ProficiencyName, float Amount);
};

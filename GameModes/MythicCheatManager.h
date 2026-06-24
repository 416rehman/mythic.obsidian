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
    void MythSaveCharacter(
        const FString &SlotName = TEXT("DebugCharacter") /* == UMythicSaveGameSubsystem::DebugCharacterSlot; UHT requires a literal default on Exec params */);

    // Load character from slot
    // Example: LoadCharacter MyCharacter
    UFUNCTION(Exec)
    void MythLoadCharacter(
        const FString &SlotName = TEXT("DebugCharacter") /* == UMythicSaveGameSubsystem::DebugCharacterSlot; UHT requires a literal default on Exec params */);

    // Save world state
    // Example: SaveWorld MyWorld
    UFUNCTION(Exec)
    void MythSaveWorld(
        const FString &SlotName = TEXT("DebugWorld") /* == UMythicSaveGameSubsystem::DebugWorldSlot; UHT requires a literal default on Exec params */);

    // Load world state
    // Example: LoadWorld MyWorld
    UFUNCTION(Exec)
    void MythLoadWorld(
        const FString &SlotName = TEXT("DebugWorld") /* == UMythicSaveGameSubsystem::DebugWorldSlot; UHT requires a literal default on Exec params */);

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

    // === LIVING WORLD ===

    // Show living world system status (subsystem, thread, fabric, factions, territory)
    // Example: LivingWorldStatus
    UFUNCTION(Exec)
    void MythLivingWorldStatus();

    // List all registered factions and their state
    // Example: LivingWorldFactions
    UFUNCTION(Exec)
    void MythLivingWorldFactions();

    // Show territory info for the player's current cell
    // Example: LivingWorldTerritory
    UFUNCTION(Exec)
    void MythLivingWorldTerritory();

    // Show MASS entity counts (NPCs vs Creatures, per-settlement breakdown)
    // Example: LivingWorldPopulation
    UFUNCTION(Exec)
    void MythLivingWorldPopulation();

    // List all registered settlements with governing faction and population info
    // Example: LivingWorldSettlements
    UFUNCTION(Exec)
    void MythLivingWorldSettlements();

    // Force-transfer a settlement to a different faction (by settlement ID and faction index)
    // Example: LivingWorldTransferSettlement 0 2
    UFUNCTION(Exec)
    void MythLivingWorldTransferSettlement(int32 SettlementId, int32 FactionIndex);

    // Toggle Living World Debug Visualization (Territory cells, Settlements)
    // Example: ToggleLivingWorldDebug
    UFUNCTION(Exec)
    void MythToggleLivingWorldDebug();

    // === LIVING WORLD: EVENT PIPELINE (Phase 4) ===

    // Fire a test action event at the player's location
    // Example: LivingWorldSimulateEvent Action.Combat.MeleeKill Violence 0.8
    UFUNCTION(Exec)
    void MythLivingWorldSimulateEvent(const FString &ActionTag, const FString &MoralAxis, float MoralValue);

    // Show pressure values of the nearest hydrated MASS entity
    // Example: LivingWorldPressure
    UFUNCTION(Exec)
    void MythLivingWorldPressure();

    // Show significance scores and tier of nearby MASS entities
    // Example: LivingWorldSignificance
    UFUNCTION(Exec)
    void MythLivingWorldSignificance();

    // Force-promote the nearest MASS entity to Tier 1 (hydrated)
    // Example: LivingWorldForcePromote
    UFUNCTION(Exec)
    void MythLivingWorldForcePromote();

    // === LIVING WORLD: PHASE 5 (Cognitive + Social) ===

    // Show social graph stats (total entities, edges, average degree)
    // Example: LivingWorldSocialGraph
    UFUNCTION(Exec)
    void MythLivingWorldSocialGraph();

    // Show active schemes (all factions)
    // Example: LivingWorldSchemes
    UFUNCTION(Exec)
    void MythLivingWorldSchemes();

    // Show active encounters
    // Example: LivingWorldEncounters
    UFUNCTION(Exec)
    void MythLivingWorldEncounters();

    // Show party status for the local player
    // Example: LivingWorldParty
    UFUNCTION(Exec)
    void MythLivingWorldParty();

    // === PLACEABLES ===

    // Deploy the placeable item in the given inventory slot at the player's aim (pawn eyes + forward). Drives the
    // real server deploy path (AMythicPlayerController::ServerDeployPlaceable) — the interim human / 2-client test
    // entry point until the player-facing deploy input + ghost-preview are wired.
    // Example: DeployPlaceable 0
    UFUNCTION(Exec)
    void MythDeployPlaceable(int32 SlotIndex = 0);

    // === CO-OP DOWN/REVIVE ===

    // Toggle the co-op down/revive policy at runtime (UMythicDeveloperSettings::bCoopDownStateEnabled). With it ON, a
    // lethal blow downs a player instead of killing. Example: ToggleCoopDown
    UFUNCTION(Exec)
    void MythToggleCoopDown();

    // Revive the local player's pawn if it is downed (drives UMythicLifeComponent::ServerReviveFromDowned).
    // Example: ReviveSelf
    UFUNCTION(Exec)
    void MythReviveSelf();
};

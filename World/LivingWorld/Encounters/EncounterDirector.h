// Mythic Living World — Encounter Director
// WorldSubsystem that evaluates world state and spawns data-driven encounters.
// Budget-capped: max active encounters per region.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "World/LivingWorld/Encounters/EncounterTemplate.h"
#include "EncounterDirector.generated.h"

class UMythicCausalFabric;
class UMythicFactionDatabase;
class UMythicTerritoryGrid;
class UMythicLivingWorldSettings;
class UMythicLivingWorldSubsystem;
class UObjectiveDefinition;

/**
 * Encounter Director — reads cached world state and spawns encounters.
 *
 * Architecture:
 * - Timer-driven evaluation (every 5-10s, configurable)
 * - Reads faction DB, territory grid, causal fabric for encounter eligibility
 * - Budget cap: max active encounters per cell cluster
 * - Encounter lifecycle: spawn → active → completing → cleanup
 *
 * Performance:
 * - Evaluation is O(templates × factions) with early-out on prerequisites
 * - Max 10 active encounters globally (configurable)
 * - No per-frame cost except timer callback (~every 5s)
 * - Spawns via existing NPC pooling system
 */
UCLASS()
class MYTHIC_API UMythicEncounterDirector : public UWorldSubsystem {
    GENERATED_BODY()

public:
    //~ Begin USubsystem Interface
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject *Outer) const override;
    //~ End USubsystem Interface

    // ─── Configuration ────────────────────────────────────

    /**
     * Register an encounter template. Templates are evaluated periodically
     * against world state. Call during setup (e.g., from data asset loading).
     */
    void RegisterTemplate(const FMythicEncounterTemplate &Template);

    // ─── Queries ──────────────────────────────────────────

    /** Get all currently active encounters */
    const TArray<FMythicActiveEncounter> &GetActiveEncounters() const { return ActiveEncounters; }

    /** Get active encounter count */
    int32 GetActiveEncounterCount() const { return ActiveEncounters.Num(); }

    /** Get the global active-encounter budget cap (for HUD/debug "active/cap" display) */
    int32 GetMaxActiveEncounters() const { return MaxActiveEncounters; }

    /** Check if any encounter is active in the given cell */
    bool HasEncounterInCell(const FMythicCellCoord &Cell) const;

    // ─── Encounter Lifecycle ──────────────────────────────

    /**
     * Force-complete an encounter (for cheat commands or testing).
     * @return true if the encounter was found and completed
     */
    bool ForceCompleteEncounter(uint32 EncounterId);

    // ─── Objective Hook (proximity → offer → reward, now WIRED) ──
    /**
     * The objective offered when a player enters a live encounter's cell (the "kill the bandits / clear the patrol"
     * quest). Resolution order:
     *   1. UMythicLivingWorldSettings::EncounterClearObjective (an authored TSoftObjectPtr) — wins when assigned.
     *   2. Otherwise a CODE-DEFAULT objective built once on first use (MythicEncounterObjectiveDefaults) and cached on
     *      DefaultClearObjective, so the loop is FUNCTIONAL out of the box with no authored asset.
     * Never returns null when Settings is available (the code default always backs it). Const-but-mutating: it lazily
     * populates the cached default, so DefaultClearObjective is mutable.
     *
     * HONEST GAP: the offer loop (MaybeOfferClearObjectives) and the reward-on-defeat path are now WIRED (see below).
     * What remains content-side: the code default grants NO reward (its Rewards holder is empty — a real reward needs an
     * authored XP/item asset), and kill-credit is not attributed to THIS encounter's specific members (any kill while
     * the objective is active advances it). Assigning an authored EncounterClearObjective with real rewards closes the
     * reward gap with zero code changes.
     */
    UObjectiveDefinition *GetEncounterClearObjective() const;

private:
    // ─── Evaluation ───────────────────────────────────────

    /** Timer callback — evaluates templates and manages encounter lifecycle */
    void EvaluationTick();

    /** Evaluate a template against current world state */
    bool EvaluateTemplate(const FMythicEncounterTemplate &Template, FMythicCellCoord &OutCell, FMythicFactionId &OutFaction) const;

    /** Spawn a new encounter from a template */
    void SpawnEncounter(const FMythicEncounterTemplate &Template, const FMythicCellCoord &Cell, FMythicFactionId Faction);

    /** Update active encounter lifecycle (timeout, completion) */
    void UpdateActiveEncounters();

    /**
     * SERVER: offer the encounter-clear objective to every player whose pawn currently occupies a live encounter's cell.
     * Mirrors AMythicPlayerController::CheckZoneEntry (a server-side cell poll) but driven off the director's existing
     * evaluation timer so no new timer is added. ServerAddObjective is idempotent (a re-offer to a player who already
     * has it is a no-op), so calling this every EvaluationTick is safe. Resolves the player's UObjectiveTracker via the
     * AMythicPlayerController accessor. Reward-on-completion is handled by the tracker's own GAS.Event.Kill subscription
     * (no extra wiring needed). No-op if there are no active encounters or GetEncounterClearObjective() is null.
     */
    void MaybeOfferClearObjectives();

    /** Clean up a completed encounter */
    void CleanupEncounter(int32 Index);

    /** Submit the ENCOUNTER_COMPLETED chronicle beat (mirror of the spawn beat). Single source — called from BOTH the
     *  UpdateActiveEncounters timeout/completion path AND ForceCompleteEncounter, so it fires exactly once per completed
     *  encounter regardless of which path completes it. NOT called from CleanupEncounter (which is also the deinit path). */
    void EmitEncounterCompletedEvent(const FMythicActiveEncounter &Encounter, bool bDefeated) const;

    // ─── Cooldown Tracking ────────────────────────────────

    /** Track template cooldowns — TemplateTag → last activation world time */
    TMap<FGameplayTag, double> TemplateCooldowns;

    /** Check if a template is on cooldown */
    bool IsOnCooldown(const FGameplayTag &TemplateTag, double WorldTime, float CooldownSeconds) const;

    /** Count active instances of a template */
    int32 CountActiveInstances(const FGameplayTag &TemplateTag) const;

    // ─── State ────────────────────────────────────────────

    /** Registered encounter templates */
    TArray<FMythicEncounterTemplate> Templates;

    /** Currently active encounters */
    TArray<FMythicActiveEncounter> ActiveEncounters;

    /** Max active encounters globally */
    int32 MaxActiveEncounters = 10;

    /** Evaluation interval in seconds */
    float EvaluationInterval = 5.0f;

    /** Next encounter ID */
    uint32 NextEncounterId = 1;

    /** Timer handle for evaluation */
    FTimerHandle EvaluationTimerHandle;

    // ─── Cached References ────────────────────────────────

    UPROPERTY()
    TObjectPtr<UMythicCausalFabric> CausalFabric;

    UPROPERTY()
    TObjectPtr<UMythicFactionDatabase> FactionDB;

    UPROPERTY()
    TObjectPtr<UMythicTerritoryGrid> TerritoryGrid;

    const UMythicLivingWorldSettings *Settings = nullptr;

    /** Cached CODE-DEFAULT clear objective, lazily built by GetEncounterClearObjective() when no authored
     *  EncounterClearObjective asset is assigned. Built ONCE (stable pointer for the ObjectiveTracker's pointer-keyed
     *  dedup); never used when an authored asset wins. The const accessor populates it via a const_cast of `this`
     *  (rather than a `mutable` UPROPERTY, which UHT can mis-parse) — keeping it a normal reflected (GC-kept) member. */
    UPROPERTY()
    TObjectPtr<UObjectiveDefinition> DefaultClearObjective;

    /** Owning subsystem. Game-thread event writes MUST go through its thread-safe SubmitWorldEvent queue — the
     *  CausalFabric is a lock-free SINGLE-writer drained by the sim thread, so a direct game-thread AppendEvent races
     *  it and corrupts the shared world log. */
    UPROPERTY()
    TObjectPtr<UMythicLivingWorldSubsystem> LivingWorld;
};

// Mythic Living World — Action Event Subsystem
// Game-thread coordinator for action event submission and witness queue management.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "World/LivingWorld/Events/ActionEventTypes.h"
#include "ActionEventSubsystem.generated.h"

class UMythicLivingWorldSubsystem;
class UMythicTerritoryGrid;

/**
 * Per-world subsystem that receives gameplay actions and feeds them into the
 * Living World event pipeline.
 *
 * Flow:
 * 1. Gameplay code calls SubmitAction() with an FMythicActionEvent
 * 2. This subsystem resolves actor positions → cell coords, factions
 * 3. Writes the event to the Causal Fabric via LivingWorldSubsystem
 * 4. Queues the resolved event for the WitnessPerceptionProcessor
 *
 * Server-only — clients do not run the event pipeline.
 */
UCLASS()
class MYTHIC_API UMythicActionEventSubsystem : public UWorldSubsystem {
    GENERATED_BODY()

public:
    virtual bool ShouldCreateSubsystem(UObject *Outer) const override;
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;

    /**
     * Submit a gameplay action into the event pipeline.
     * Resolves actors to faction/cell, writes to fabric, queues for witness processing.
     * Must be called on the game thread.
     */
    void SubmitAction(const FMythicActionEvent &Action);

    // ─── Queue Access (for WitnessPerceptionProcessor) ────

    /** Get pending action events awaiting witness processing. Called by processor each frame. */
    TArray<FMythicPendingActionEvent> &GetPendingEvents() { return PendingEvents; }

    /** Remove fully-processed events from the queue. Called after processor completes. */
    void FlushProcessedEvents();

    /** Get pending witness results for the pressure processor */
    TArray<FMythicWitnessResult> &GetPendingWitnessResults() { return PendingWitnessResults; }

    /** Remove consumed witness results */
    void FlushProcessedWitnessResults();

    /** Check if there are pending events or witness results to process */
    bool HasPendingWork() const { return PendingEvents.Num() > 0 || PendingWitnessResults.Num() > 0; }

private:
    /** Cached reference to the living world subsystem (GameInstance-level) */
    UPROPERTY()
    TObjectPtr<UMythicLivingWorldSubsystem> LivingWorldSubsystem;

    /** Events awaiting witness scanning (consumed by WitnessPerceptionProcessor) */
    TArray<FMythicPendingActionEvent> PendingEvents;

    /** Witness results awaiting pressure processing (produced by Witness, consumed by Pressure) */
    TArray<FMythicWitnessResult> PendingWitnessResults;

    /** Resolve an actor to a cell coordinate using the territory grid */
    FMythicCellCoord ResolveActorCell(const AActor *Actor) const;

    /** Resolve an actor's faction ID (from its faction component or default) */
    FMythicFactionId ResolveActorFaction(const AActor *Actor) const;
};

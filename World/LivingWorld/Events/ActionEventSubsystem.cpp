// Mythic Living World — Action Event Subsystem Implementation

#include "World/LivingWorld/Events/ActionEventSubsystem.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY_STATIC(LogMythActionEvent, Log, All);

bool UMythicActionEventSubsystem::ShouldCreateSubsystem(UObject *Outer) const {
    const UWorld *World = Cast<UWorld>(Outer);
    if (!World) {
        return false;
    }

    // Server-only — event pipeline runs on dedicated/listen server, not clients
    const ENetMode NetMode = World->GetNetMode();
    return NetMode != NM_Client;
}

void UMythicActionEventSubsystem::Initialize(FSubsystemCollectionBase &Collection) {
    Super::Initialize(Collection);

    // Cache the GameInstance-level living world subsystem
    if (const UGameInstance *GI = GetWorld()->GetGameInstance()) {
        LivingWorldSubsystem = GI->GetSubsystem<UMythicLivingWorldSubsystem>();
    }

    if (!LivingWorldSubsystem) {
        UE_LOG(LogMythActionEvent, Warning, TEXT("ActionEventSubsystem initialized but LivingWorldSubsystem not available — events will be discarded"));
    }

    // Pre-allocate typical frame budget worth of events to avoid per-frame allocation
    PendingEvents.Reserve(16);
    PendingWitnessResults.Reserve(64);
}

void UMythicActionEventSubsystem::SubmitAction(const FMythicActionEvent &Action) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicActionEvent_Submit);

    if (!LivingWorldSubsystem || !LivingWorldSubsystem->IsSystemActive()) {
        return;
    }

    // Resolve actor references to pure data
    const AActor *PerpActor = Action.Perpetrator.Get();
    const AActor *VictimActor = Action.Victim.Get();

    // Determine cell — use override if valid, otherwise derive from perpetrator position
    FMythicCellCoord EventCell;
    if (Action.OverrideCell.X >= 0) {
        EventCell = Action.OverrideCell;
    }
    else if (PerpActor) {
        EventCell = ResolveActorCell(PerpActor);
    }
    else {
        UE_LOG(LogMythActionEvent, Warning, TEXT("SubmitAction called with no perpetrator and no override cell — event discarded"));
        return;
    }

    // Resolve factions
    const FMythicFactionId PerpFaction = PerpActor ? ResolveActorFaction(PerpActor) : FMythicFactionId();
    const FMythicFactionId VictimFaction = VictimActor ? ResolveActorFaction(VictimActor) : FMythicFactionId();

    // Build the world event for the causal fabric
    FMythicWorldEvent WorldEvent;
    WorldEvent.WorldTime = GetWorld()->GetTimeSeconds();
    WorldEvent.Cell = EventCell;
    WorldEvent.PrimaryFaction = PerpFaction;
    WorldEvent.SecondaryFaction = VictimFaction;
    WorldEvent.EventTag = Action.ActionTag;
    WorldEvent.MoralVector = Action.MoralVector;
    WorldEvent.Significance = Action.Significance;
    WorldEvent.CategoryFlags = Action.CategoryFlags;
    WorldEvent.ActionCategory = Action.ActionCategory;

    // Submit to causal fabric (buffered write — background thread commits later)
    LivingWorldSubsystem->SubmitWorldEvent(WorldEvent);

    // Queue a pending event for the witness perception processor
    FMythicPendingActionEvent PendingEvent;
    PendingEvent.WorldEvent = WorldEvent;
    PendingEvent.WitnessesProcessed = 0;
    PendingEvent.bFullyProcessed = false;
    PendingEvents.Add(MoveTemp(PendingEvent));

    UE_LOG(LogMythActionEvent, Verbose, TEXT("Action submitted: Tag=%s Cell=%s Significance=%.2f"),
           *Action.ActionTag.ToString(), *EventCell.ToString(), Action.Significance);
}

void UMythicActionEventSubsystem::FlushProcessedEvents() {
    // Remove events that the witness processor has fully consumed
    PendingEvents.RemoveAll([](const FMythicPendingActionEvent &Evt) { return Evt.bFullyProcessed; });
}

void UMythicActionEventSubsystem::FlushProcessedWitnessResults() {
    PendingWitnessResults.Reset();
}

FMythicCellCoord UMythicActionEventSubsystem::ResolveActorCell(const AActor *Actor) const {
    if (!Actor || !LivingWorldSubsystem) {
        return FMythicCellCoord(0, 0);
    }

    const UMythicTerritoryGrid *Grid = LivingWorldSubsystem->GetTerritoryGrid();
    if (!Grid) {
        return FMythicCellCoord(0, 0);
    }

    return Grid->WorldToCell(Actor->GetActorLocation());
}

FMythicFactionId UMythicActionEventSubsystem::ResolveActorFaction(const AActor *Actor) const {
    // Phase 4 default: return invalid faction for actors without faction components.
    // Phase 5 will integrate with the NPC/player faction component system.
    // For now, gameplay systems should set faction on the action event directly if needed.
    return FMythicFactionId();
}

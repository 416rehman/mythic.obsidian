
#include "World/LivingWorld/Events/ActionEventSubsystem.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "AI/Cognition/CognitiveBrainComponent.h"
#include "Player/MythicCharacter.h"
#include "Engine/GameInstance.h"

DEFINE_LOG_CATEGORY_STATIC(LogMythActionEvent, Log, All);

bool UMythicActionEventSubsystem::ShouldCreateSubsystem(UObject *Outer) const {
    const UWorld *World = Cast<UWorld>(Outer);
    if (!World) {
        return false;
    }

    const ENetMode NetMode = World->GetNetMode();
    return NetMode != NM_Client;
}

void UMythicActionEventSubsystem::Initialize(FSubsystemCollectionBase &Collection) {
    Super::Initialize(Collection);

    ResolveLivingWorld();

    PendingEvents.Reserve(16);
    PendingWitnessResults.Reserve(64);
}

void UMythicActionEventSubsystem::SubmitAction(const FMythicActionEvent &Action) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicActionEvent_Submit);

    if (const UMythicLivingWorldSubsystem *LW = ResolveLivingWorld(); !LW || !LW->IsSystemActive()) {
        return;
    }

    const AActor *PerpActor = Action.Perpetrator.Get();
    const AActor *VictimActor = Action.Victim.Get();

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

    const FMythicFactionId PerpFaction = Action.PerpFactionOverride.IsValid()
        ? Action.PerpFactionOverride
        : (PerpActor ? ResolveActorFaction(PerpActor) : FMythicFactionId());
    const FMythicFactionId VictimFaction = Action.VictimFactionOverride.IsValid()
        ? Action.VictimFactionOverride
        : (VictimActor ? ResolveActorFaction(VictimActor) : FMythicFactionId());

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
    WorldEvent.VisibilityGroup = Action.VisibilityGroup;

    float StealthScale = Action.StealthPerceptionScale;
    if (const AMythicCharacter *PerpChar = Cast<AMythicCharacter>(PerpActor)) {
        StealthScale = FMath::Min(StealthScale, PerpChar->GetStealthPerceptionScale());
    }

    if (UMythicLivingWorldSubsystem *LW = ResolveLivingWorld()) {
        LW->SubmitWorldEvent(WorldEvent);
    }

    FMythicPendingActionEvent PendingEvent;
    PendingEvent.WorldEvent = WorldEvent;
    PendingEvent.WitnessesProcessed = 0;
    PendingEvent.bFullyProcessed = false;
    PendingEvent.PerpPlayerKey = Action.PerpPlayerKey;
    PendingEvent.StealthPerceptionScale = StealthScale;
    PendingEvents.Add(MoveTemp(PendingEvent));

    UE_LOG(LogMythActionEvent, Verbose, TEXT("Action submitted: Tag=%s Cell=%s Significance=%.2f"),
           *Action.ActionTag.ToString(), *EventCell.ToString(), Action.Significance);
}

void UMythicActionEventSubsystem::FlushProcessedEvents() {
    PendingEvents.RemoveAll([](const FMythicPendingActionEvent &Evt) { return Evt.bFullyProcessed; });
}

void UMythicActionEventSubsystem::FlushProcessedWitnessResults() {
    PendingWitnessResults.Reset();
}

FMythicCellCoord UMythicActionEventSubsystem::ResolveActorCell(const AActor *Actor) const {
    if (!Actor || !ResolveLivingWorld()) {
        return FMythicCellCoord(0, 0);
    }

    const UMythicTerritoryGrid *Grid = ResolveLivingWorld()->GetTerritoryGrid();
    if (!Grid) {
        return FMythicCellCoord(0, 0);
    }

    return Grid->WorldToCell(Actor->GetActorLocation());
}

FMythicFactionId UMythicActionEventSubsystem::ResolveActorFaction(const AActor *Actor) const {
    if (!Actor) {
        return FMythicFactionId();
    }
    if (UMythicCognitiveBrainComponent *Brain = Actor->FindComponentByClass<UMythicCognitiveBrainComponent>()) {
        return Brain->GetFaction();
    }
    return FMythicFactionId();
}

UMythicLivingWorldSubsystem *UMythicActionEventSubsystem::ResolveLivingWorld() const {
    if (LivingWorldSubsystem) {
        return LivingWorldSubsystem;
    }
    UMythicActionEventSubsystem *Self = const_cast<UMythicActionEventSubsystem *>(this);
    if (const UWorld *World = GetWorld()) {
        if (const UGameInstance *GI = World->GetGameInstance()) {
            Self->LivingWorldSubsystem = GI->GetSubsystem<UMythicLivingWorldSubsystem>();
        }
    }
    return LivingWorldSubsystem;
}

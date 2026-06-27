// Mythic Living World — Traveler Route Processor Implementation

#include "Mass/Processors/TravelerRouteProcessor.h"
#include "Mass/Processors/TravelerSpawnerProcessor.h" // StepToward (single deterministic step helper)
#include "MassExecutionContext.h"
#include "MassCommandBuffer.h"
#include "Mass/Fragments/MythicMassFragments.h"
#include "Mass/Fragments/MythicTravelerFragment.h"
#include "Mass/Tags/MythicMassTags.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "AI/NPCs/MythicNPCCharacter.h" // despawn tears down any embodied traveler actor before freeing its entity
#include "Engine/World.h"

UMythicTravelerRouteProcessor::UMythicTravelerRouteProcessor() {
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
    ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);
    bRequiresGameThreadExecution = true;
    bAutoRegisterWithProcessingPhases = true;

    // Process only traveler archetypes — but those may not exist at startup, so survive pruning (the spawner creates
    // them later). A pruned route processor would never advance the first travelers.
    QueryBasedPruning = EMassQueryBasedPruning::Never;

    // CRITICAL ordering: run AFTER the schedule transition processor. That processor re-stamps Schedule.Phase from the
    // time of day EVERY tick; if it ran after this one it would overwrite the traveler's Work phase (e.g. with Rest at
    // night) and the FollowSchedule desire would drop, stopping the traveler. Running after it, we RE-STAMP Phase=Work
    // each step so travelers move around the clock. (Also after the spawner for a stable, deterministic order.)
    ExecutionOrder.ExecuteAfter.Add(TEXT("UMythicScheduleTransitionProcessor"));
    ExecutionOrder.ExecuteAfter.Add(TEXT("UMythicTravelerSpawnerProcessor"));

    TravelerQuery.RegisterWithProcessor(*this);
}

void UMythicTravelerRouteProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) {
    // Identity RW: dead-reckon Cell forward when NOT embodied (when embodied, the AIController owns it — see Execute).
    TravelerQuery.AddRequirement<FMythicIdentityFragment>(EMassFragmentAccess::ReadWrite);
    // Schedule RW: keep WorkCell one step ahead + re-stamp Phase = Work each step.
    TravelerQuery.AddRequirement<FMythicScheduleFragment>(EMassFragmentAccess::ReadWrite);
    // Traveler RW: decrement StepsRemaining; advance per-entity step accumulator.
    TravelerQuery.AddRequirement<FMythicTravelerFragment>(EMassFragmentAccess::ReadWrite);
    // Significance RW: dirty the score on a dead-reckon cell change so the proximity rescore re-evaluates the moved
    // traveler (every traveler carries Significance — added by the spawner). Accessed via the chunk view, not a manager
    // lookup, so it's structurally safe mid-iteration.
    TravelerQuery.AddRequirement<FMythicSignificanceFragment>(EMassFragmentAccess::ReadWrite);
    TravelerQuery.AddTagRequirement<FMythicTravelerTag>(EMassFragmentPresence::All);
}

void UMythicTravelerRouteProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicTravelerRoute_Execute);

    UWorld *World = GetWorld();
    if (!World) {
        return;
    }
    UGameInstance *GI = World->GetGameInstance();
    if (!GI) {
        return;
    }
    UMythicLivingWorldSubsystem *LWS = GI->GetSubsystem<UMythicLivingWorldSubsystem>();
    if (!LWS || !LWS->IsSystemActive()) {
        return;
    }
    const UMythicLivingWorldSettings *Settings = LWS->GetSettings();
    if (!Settings) {
        return;
    }

    UMythicTerritoryGrid *Grid = LWS->GetTerritoryGrid();
    if (!Grid) {
        return;
    }

    // Throttle: advance ONE cell per (cell size / travel speed) — a believable walking pace that scales with cell size
    // and roughly matches the embodied AIController's movement, so a traveler crossing into a player's embodiment band
    // DWELLS there long enough to be promoted. The old fixed 0.75s per 5000cm cell was ~67 m/s — a teleport-walk that
    // both looked absurd in the debug view AND skipped the embodiment window so travelers never spawned near the player.
    // TravelerStepIntervalSeconds acts as a floor (a minimum step time / max wake rate).
    const float StepInterval = FMath::Max(Settings->TravelerStepIntervalSeconds,
                                          Grid->GetCellSize() / FMath::Max(1.0f, Settings->TravelerSpeedCmPerSec));
    TimeSinceLastTick += Context.GetDeltaTimeSeconds();
    if (TimeSinceLastTick < StepInterval) {
        return;
    }
    TimeSinceLastTick = 0.0f;

    TravelerQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext &ChunkContext) {
        const int32 NumEntities = ChunkContext.GetNumEntities();
        const auto IdentityView = ChunkContext.GetMutableFragmentView<FMythicIdentityFragment>();
        const auto ScheduleView = ChunkContext.GetMutableFragmentView<FMythicScheduleFragment>();
        const auto TravelerView = ChunkContext.GetMutableFragmentView<FMythicTravelerFragment>();
        const auto SignificanceView = ChunkContext.GetMutableFragmentView<FMythicSignificanceFragment>();

        for (int32 i = 0; i < NumEntities; ++i) {
            FMythicIdentityFragment &Identity = IdentityView[i];
            FMythicScheduleFragment &Schedule = ScheduleView[i];
            FMythicTravelerFragment &Traveler = TravelerView[i];

            const FMassEntityHandle Entity = ChunkContext.GetEntity(i);

            // ─── Arrival / route-cap → despawn ───
            const bool bArrived = (Identity.Cell == Traveler.DestinationCell);
            if (bArrived || Traveler.StepsRemaining == 0) {
                // Tear down any embodied actor FIRST (it registered into the shared EmbodiedActors map), then destroy
                // the entity — verbatim the population/creature spawner despawn idiom. Travelers are exempt from the
                // population spawner's far-despawn (FMythicTravelerTag), so this is their sole despawn path. Safe to
                // call directly: bRequiresGameThreadExecution guarantees the game thread; LWS is captured from Execute.
                if (AMythicNPCCharacter *Actor = LWS->FindEmbodiedActor(Entity)) {
                    Actor->Destroy();
                }
                LWS->UnregisterEmbodiedActor(Entity); // idempotent — also clears a stale key if no actor
                Context.Defer().DestroyEntity(Entity);
                continue;
            }

            // ─── Advance one greedy step toward the destination ───
            const FMythicCellCoord NextCell =
                UMythicTravelerSpawnerProcessor::StepToward(Identity.Cell, Traveler.DestinationCell);

            // Re-stamp Phase = Work + WorkCell = next step EVERY step so the embodied brain's FollowSchedule desire
            // (0.7, Work-phase-gated) keeps steering the AIController to CellToWorld(WorkCell). This is the load-bearing
            // re-stamp that defeats the ScheduleTransitionProcessor's time-of-day phase (we ExecuteAfter it).
            Schedule.Phase = EMythicSchedulePhase::Work;
            Schedule.WorkCell = NextCell;

            // Identity.Cell single-writer rule:
            //  - NOT embodied: no actor owns the position, so the route processor dead-reckons Cell forward itself
            //    (keeps spatial readers — significance proximity, witness hearing — tracking the off-screen traveler).
            //  - EMBODIED: the AIController::RefreshLiveCell move-loop is the SOLE Identity.Cell writer (it sets Cell
            //    from the pawn's real world position). We must NOT also write it here, or the two would fight in one
            //    frame. The AIController walks the pawn toward WorkCell; Cell follows the body.
            const bool bEmbodied = (LWS->FindEmbodiedActor(Entity) != nullptr);
            if (!bEmbodied) {
                Identity.Cell = NextCell;
                // Dirty the significance score so the proximity rescore re-evaluates this entity's new cell (mirrors
                // the AIController::RefreshLiveCell dirty — cheap, only on a cell change, which is every step here).
                // Via the chunk view (structurally safe mid-iteration), not a manager fragment lookup.
                SignificanceView[i].bDirty = true;
            }

            // Hard route cap: one less step allowed. Reaching 0 forces a despawn next tick even if the greedy step is
            // somehow blocked from converging — a traveler can never wander forever.
            --Traveler.StepsRemaining;
        }
    });
}

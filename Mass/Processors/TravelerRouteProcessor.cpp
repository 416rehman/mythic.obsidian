
#include "Mass/Processors/TravelerRouteProcessor.h"
#include "Mass/Processors/TravelerSpawnerProcessor.h"
#include "MassExecutionContext.h"
#include "MassCommandBuffer.h"
#include "Mass/Fragments/MythicMassFragments.h"
#include "Mass/Fragments/MythicTravelerFragment.h"
#include "Mass/Tags/MythicMassTags.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "AI/NPCs/MythicNPCCharacter.h"
#include "Engine/World.h"

UMythicTravelerRouteProcessor::UMythicTravelerRouteProcessor() {
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
    ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);
    bRequiresGameThreadExecution = true;
    bAutoRegisterWithProcessingPhases = true;

    QueryBasedPruning = EMassQueryBasedPruning::Never;

    ExecutionOrder.ExecuteAfter.Add(TEXT("UMythicScheduleTransitionProcessor"));
    ExecutionOrder.ExecuteAfter.Add(TEXT("UMythicTravelerSpawnerProcessor"));

    TravelerQuery.RegisterWithProcessor(*this);
}

void UMythicTravelerRouteProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) {
    TravelerQuery.AddRequirement<FMythicIdentityFragment>(EMassFragmentAccess::ReadWrite);
    TravelerQuery.AddRequirement<FMythicScheduleFragment>(EMassFragmentAccess::ReadWrite);
    TravelerQuery.AddRequirement<FMythicTravelerFragment>(EMassFragmentAccess::ReadWrite);
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

            const bool bArrived = (Identity.Cell == Traveler.DestinationCell);
            if (bArrived || Traveler.StepsRemaining == 0) {
                if (AMythicNPCCharacter *Actor = LWS->FindEmbodiedActor(Entity)) {
                    Actor->Destroy();
                }
                LWS->UnregisterEmbodiedActor(Entity);
                Context.Defer().DestroyEntity(Entity);
                continue;
            }

            const FMythicCellCoord NextCell =
                UMythicTravelerSpawnerProcessor::StepToward(Identity.Cell, Traveler.DestinationCell);

            Schedule.Phase = EMythicSchedulePhase::Work;
            Schedule.WorkCell = NextCell;

            const bool bEmbodied = (LWS->FindEmbodiedActor(Entity) != nullptr);
            if (!bEmbodied) {
                Identity.Cell = NextCell;
                SignificanceView[i].bDirty = true;
            }

            --Traveler.StepsRemaining;
        }
    });
}

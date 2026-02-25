// Mythic Living World — Schedule Transition Processor Implementation

#include "Mass/Processors/ScheduleTransitionProcessor.h"
#include "MassEntitySubsystem.h"
#include "MassExecutionContext.h"
#include "Mass/Fragments/MythicMassFragments.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "Engine/World.h"

UMythicScheduleTransitionProcessor::UMythicScheduleTransitionProcessor() {
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
    ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);
    bRequiresGameThreadExecution = true;
    bAutoRegisterWithProcessingPhases = true;

    // Run after population spawner (so newly spawned entities have schedule set)
    ExecutionOrder.ExecuteAfter.Add(TEXT("UMythicPopulationSpawnerProcessor"));

    ScheduleQuery.RegisterWithProcessor(*this);
}

void UMythicScheduleTransitionProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) {
    ScheduleQuery.AddRequirement<FMythicIdentityFragment>(EMassFragmentAccess::ReadOnly);
    ScheduleQuery.AddRequirement<FMythicScheduleFragment>(EMassFragmentAccess::ReadWrite);
    ScheduleQuery.AddRequirement<FMythicSignificanceFragment>(EMassFragmentAccess::ReadOnly);
}

EMythicSchedulePhase UMythicScheduleTransitionProcessor::GetPhaseForHour(float GameHour) {
    // Schedule layout (24h game time cycle):
    //   06:00 - 08:00  Travel (home → work)
    //   08:00 - 14:00  Work
    //   14:00 - 15:00  Travel (work → social)
    //   15:00 - 18:00  Social
    //   18:00 - 19:00  Travel (social → home)
    //   19:00 - 06:00  Rest (including night)

    if (GameHour < 6.0f) {
        return EMythicSchedulePhase::Rest;
    } else if (GameHour < 8.0f) {
        return EMythicSchedulePhase::Travel;
    } else if (GameHour < 14.0f) {
        return EMythicSchedulePhase::Work;
    } else if (GameHour < 15.0f) {
        return EMythicSchedulePhase::Travel;
    } else if (GameHour < 18.0f) {
        return EMythicSchedulePhase::Social;
    } else if (GameHour < 19.0f) {
        return EMythicSchedulePhase::Travel;
    } else {
        return EMythicSchedulePhase::Rest;
    }
}

void UMythicScheduleTransitionProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicScheduleTransition_Execute);

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

    // Timer throttle — schedule transitions don't need to run every frame
    TimeSinceLastTick += Context.GetDeltaTimeSeconds();
    if (TimeSinceLastTick < 2.0f) {
        return;
    }
    TimeSinceLastTick = 0.0f;

    // ─── Compute current game hour ───
    // UWorld::GetTimeSeconds() returns elapsed game time.
    // Convert to a 24-hour cycle using the configurable day length.
    // DayLengthSeconds from settings (default: 1200 = 20min real time = 1 game day).
    const float DayLengthSeconds = Settings->DayLengthSeconds;
    if (DayLengthSeconds <= 0.0f) {
        return;
    }

    const double GameTime = World->GetTimeSeconds();
    const float DayProgress = FMath::Fmod(static_cast<float>(GameTime), DayLengthSeconds) / DayLengthSeconds;
    const float GameHour = DayProgress * 24.0f; // [0.0, 24.0)

    const EMythicSchedulePhase TargetPhase = GetPhaseForHour(GameHour);

    // ─── Transition all entities to the target phase ───
    ScheduleQuery.ForEachEntityChunk(Context, [TargetPhase](FMassExecutionContext &ChunkContext) {
        const int32 NumEntities = ChunkContext.GetNumEntities();
        auto ScheduleView = ChunkContext.GetMutableFragmentView<FMythicScheduleFragment>();
        const auto SignificanceView = ChunkContext.GetFragmentView<FMythicSignificanceFragment>();

        for (int32 i = 0; i < NumEntities; ++i) {
            // Event disruption: if entity witnessed a recent event (bDirty=true),
            // skip schedule transition — let the entity resolve the event first.
            // This implements REQ-BEH-007: "Event disruption overrides schedule."
            if (SignificanceView[i].bDirty) {
                continue;
            }

            FMythicScheduleFragment &Schedule = ScheduleView[i];

            // Only transition if the phase actually changed
            if (Schedule.Phase != TargetPhase) {
                Schedule.Phase = TargetPhase;
            }
        }
    });
}


#include "Mass/Processors/ScheduleTransitionProcessor.h"
#include "MassEntitySubsystem.h"
#include "MassExecutionContext.h"
#include "Mass/Fragments/MythicMassFragments.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/EnvironmentController/MythicEnvironmentSubsystem.h"
#include "World/EnvironmentController/MythicEnvironmentController.h"
#include "Engine/World.h"

UMythicScheduleTransitionProcessor::UMythicScheduleTransitionProcessor() {
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
    ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);
    bRequiresGameThreadExecution = true;
    bAutoRegisterWithProcessingPhases = true;

    ExecutionOrder.ExecuteAfter.Add(TEXT("UMythicPopulationSpawnerProcessor"));

    ScheduleQuery.RegisterWithProcessor(*this);
}

void UMythicScheduleTransitionProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) {
    ScheduleQuery.AddRequirement<FMythicIdentityFragment>(EMassFragmentAccess::ReadOnly);
    ScheduleQuery.AddRequirement<FMythicScheduleFragment>(EMassFragmentAccess::ReadWrite);
    ScheduleQuery.AddRequirement<FMythicSignificanceFragment>(EMassFragmentAccess::ReadOnly);
}

EMythicSchedulePhase UMythicScheduleTransitionProcessor::GetPhaseForHour(float GameHour, const UMythicLivingWorldSettings *Settings) {
    if (!Settings) {
        return EMythicSchedulePhase::Rest;
    }
    if (GameHour < Settings->ScheduleDayStartHour) {
        return EMythicSchedulePhase::Rest;
    }
    if (GameHour < Settings->ScheduleWorkStartHour) {
        return EMythicSchedulePhase::Travel;
    }
    if (GameHour < Settings->ScheduleWorkEndHour) {
        return EMythicSchedulePhase::Work;
    }
    if (GameHour < Settings->ScheduleSocialStartHour) {
        return EMythicSchedulePhase::Travel;
    }
    if (GameHour < Settings->ScheduleSocialEndHour) {
        return EMythicSchedulePhase::Social;
    }
    if (GameHour < Settings->ScheduleDayEndHour) {
        return EMythicSchedulePhase::Travel;
    }
    return EMythicSchedulePhase::Rest;
}

float UMythicScheduleTransitionProcessor::ComputeStaggeredHour(float GameHour, uint32 NameHash, float MaxStaggerHours) {
    if (MaxStaggerHours <= 0.0f) {
        return GameHour;
    }
    const float Frac = static_cast<float>(NameHash % 100003u) / 100003.0f;
    const float Offset = (Frac * 2.0f - 1.0f) * MaxStaggerHours;
    float H = FMath::Fmod(GameHour + Offset, 24.0f);
    if (H < 0.0f) {
        H += 24.0f;
    }
    return H;
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

    TimeSinceLastTick += Context.GetDeltaTimeSeconds();
    if (TimeSinceLastTick < 2.0f) {
        return;
    }
    TimeSinceLastTick = 0.0f;

    float GameHour;
    const UMythicEnvironmentSubsystem *Env = GI->GetSubsystem<UMythicEnvironmentSubsystem>();
    if (Env && Env->GetEnvironmentController() != nullptr) {
        const FTimespan Timespan = Env->GetEnvironmentController()->GetTimespan();
        GameHour = Timespan.GetHours() + Timespan.GetMinutes() / 60.0f;
    }
    else {
        const float DayLengthSeconds = Settings->DayLengthSeconds;
        if (DayLengthSeconds <= 0.0f) {
            return;
        }
        const double GameTime = World->GetTimeSeconds();
        const float DayProgress = FMath::Fmod(static_cast<float>(GameTime), DayLengthSeconds) / DayLengthSeconds;
        GameHour = DayProgress * 24.0f;
    }

    const float StaggerHours = Settings->ScheduleStaggerHours;

    ScheduleQuery.ForEachEntityChunk(Context, [GameHour, StaggerHours, Settings](FMassExecutionContext &ChunkContext) {
        const int32 NumEntities = ChunkContext.GetNumEntities();
        auto ScheduleView = ChunkContext.GetMutableFragmentView<FMythicScheduleFragment>();
        const auto SignificanceView = ChunkContext.GetFragmentView<FMythicSignificanceFragment>();
        const auto IdentityView = ChunkContext.GetFragmentView<FMythicIdentityFragment>();

        for (int32 i = 0; i < NumEntities; ++i) {
            if (SignificanceView[i].bDirty) {
                continue;
            }

            const float StaggeredHour = ComputeStaggeredHour(GameHour, IdentityView[i].NameSeed, StaggerHours);
            const EMythicSchedulePhase TargetPhase = GetPhaseForHour(StaggeredHour, Settings);

            FMythicScheduleFragment &Schedule = ScheduleView[i];

            if (Schedule.Phase != TargetPhase) {
                Schedule.Phase = TargetPhase;
            }
        }
    });
}

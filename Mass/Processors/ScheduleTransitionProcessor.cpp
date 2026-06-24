// Mythic Living World — Schedule Transition Processor Implementation

#include "Mass/Processors/ScheduleTransitionProcessor.h"
#include "MassEntitySubsystem.h"
#include "MassExecutionContext.h"
#include "Mass/Fragments/MythicMassFragments.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/EnvironmentController/MythicEnvironmentSubsystem.h" // single-source game clock (matches day/night perception)
#include "World/EnvironmentController/MythicEnvironmentController.h" // GetTimespan() — read the hour without building an FDateTime
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

EMythicSchedulePhase UMythicScheduleTransitionProcessor::GetPhaseForHour(float GameHour, const UMythicLivingWorldSettings *Settings) {
    // Designer-tunable daily routine (defaults reproduce the prior hardcoded 6/8/14/15/18/19 layout):
    //   DayStart..WorkStart   Travel (home → work)
    //   WorkStart..WorkEnd     Work
    //   WorkEnd..SocialStart   Travel (work → social)
    //   SocialStart..SocialEnd Social
    //   SocialEnd..DayEnd      Travel (social → home)
    //   DayEnd..DayStart       Rest (including night)
    // Cascading thresholds — out-of-order hours collapse a window but stay well-defined. Null settings → Rest.
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
        return GameHour; // staggering disabled → global sync (prior behavior)
    }
    // Deterministic per-NPC offset in [-MaxStaggerHours, +MaxStaggerHours] from the stable NameHash, so each NPC keeps
    // a consistent personal routine offset across sessions with no extra per-entity state. A large prime modulus gives
    // a smooth spread; map [0,1) → [-Max,+Max].
    const float Frac = static_cast<float>(NameHash % 100003u) / 100003.0f; // [0,1)
    const float Offset = (Frac * 2.0f - 1.0f) * MaxStaggerHours;            // [-Max,+Max]
    float H = FMath::Fmod(GameHour + Offset, 24.0f);
    if (H < 0.0f) {
        H += 24.0f; // wrap a negative pre-dawn offset back into [0,24)
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

    // Timer throttle — schedule transitions don't need to run every frame
    TimeSinceLastTick += Context.GetDeltaTimeSeconds();
    if (TimeSinceLastTick < 2.0f) {
        return;
    }
    TimeSinceLastTick = 0.0f;

    // ─── Compute current game hour ───
    // SINGLE SOURCE OF TRUTH: prefer the ENVIRONMENT clock — the same clock that drives day/night perception
    // (UMythicEnvironmentSubsystem::GetDayTime, via GetTimespan) and the Environment.Time.* tags — so NPC schedules and
    // the visible sky AGREE on the hour. Previously this ran on its OWN GetTimeSeconds()/DayLengthSeconds cycle, which
    // drifted from the env controller's clock (the env controller advances its own Time independently) → NPCs could
    // work while the sky said night. GetDateTime() and GetTimespan() both read the controller's single `Time` member,
    // so the hour here matches GetDayTime exactly.
    float GameHour;
    const UMythicEnvironmentSubsystem *Env = GI->GetSubsystem<UMythicEnvironmentSubsystem>();
    if (Env && Env->GetEnvironmentController() != nullptr) {
        // Read the hour STRAIGHT from the controller's timespan — do NOT round-trip through GetCurrentTime()/GetDateTime(),
        // whose synthetic 30-day calendar can build an invalid real date (e.g. Feb 30) that FDateTime::Validate rejects
        // with a Fatal log (a hard crash, every 2s once the calendar reaches February). GetTimespan().GetHours() is
        // exactly what GetDayTime() consumes, so the schedule still agrees with day/night perception — no FDateTime.
        const FTimespan Timespan = Env->GetEnvironmentController()->GetTimespan();
        GameHour = Timespan.GetHours() + Timespan.GetMinutes() / 60.0f; // [0,24), env-clock-driven
    }
    else {
        // Fallback for a clock-less level (no EnvironmentController placed): derive the hour from elapsed game time and
        // the configurable day length so schedules still run. DayLengthSeconds default 1200 = 20min real = 1 game day.
        const float DayLengthSeconds = Settings->DayLengthSeconds;
        if (DayLengthSeconds <= 0.0f) {
            return;
        }
        const double GameTime = World->GetTimeSeconds();
        const float DayProgress = FMath::Fmod(static_cast<float>(GameTime), DayLengthSeconds) / DayLengthSeconds;
        GameHour = DayProgress * 24.0f; // [0.0, 24.0)
    }

    const float StaggerHours = Settings->ScheduleStaggerHours;

    // ─── Transition each entity to its (per-NPC staggered) target phase ───
    // The target phase is computed PER ENTITY from its NameHash-staggered hour, so the town's routines don't flip in
    // lockstep (organic commute/rest spread). Cost stays trivial: a handful of float compares per entity, timer-throttled.
    ScheduleQuery.ForEachEntityChunk(Context, [GameHour, StaggerHours, Settings](FMassExecutionContext &ChunkContext) {
        const int32 NumEntities = ChunkContext.GetNumEntities();
        auto ScheduleView = ChunkContext.GetMutableFragmentView<FMythicScheduleFragment>();
        const auto SignificanceView = ChunkContext.GetFragmentView<FMythicSignificanceFragment>();
        const auto IdentityView = ChunkContext.GetFragmentView<FMythicIdentityFragment>();

        for (int32 i = 0; i < NumEntities; ++i) {
            // Event disruption: if entity witnessed a recent event (bDirty=true),
            // skip schedule transition — let the entity resolve the event first.
            // This implements REQ-BEH-007: "Event disruption overrides schedule."
            if (SignificanceView[i].bDirty) {
                continue;
            }

            const float StaggeredHour = ComputeStaggeredHour(GameHour, IdentityView[i].NameHash, StaggerHours);
            const EMythicSchedulePhase TargetPhase = GetPhaseForHour(StaggeredHour, Settings);

            FMythicScheduleFragment &Schedule = ScheduleView[i];

            // Only transition if the phase actually changed
            if (Schedule.Phase != TargetPhase) {
                Schedule.Phase = TargetPhase;
            }
        }
    });
}

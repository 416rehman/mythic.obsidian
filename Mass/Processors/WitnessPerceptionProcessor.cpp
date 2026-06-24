// Mythic Living World — Witness Perception Processor Implementation

#include "Mass/Processors/WitnessPerceptionProcessor.h"
#include "MassEntitySubsystem.h"
#include "MassExecutionContext.h"
#include "MassCommandBuffer.h"
#include "Mass/Fragments/MythicMassFragments.h"
#include "Mass/Tags/MythicMassTags.h"
#include "World/LivingWorld/Events/ActionEventSubsystem.h"
#include "World/LivingWorld/Events/ActionEventTypes.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Morality/MoralSignature.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/EnvironmentController/MythicEnvironmentSubsystem.h" // GetDayTime for REQ-BEH-007 night perception
#include "World/EnvironmentController/MythicEnvironmentController.h" // GetCurrentWeather → UWeatherType::bImpairsPerception
#include "Engine/World.h"

UMythicWitnessPerceptionProcessor::UMythicWitnessPerceptionProcessor() {
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
    ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);
    bRequiresGameThreadExecution = true; // Accesses world subsystems
    bAutoRegisterWithProcessingPhases = true;

    // Run after population spawner and creature ecology
    ExecutionOrder.ExecuteAfter.Add(TEXT("UMythicPopulationSpawnerProcessor"));
    ExecutionOrder.ExecuteAfter.Add(TEXT("UMythicCreatureEcologyProcessor"));

    AllEntitiesQuery.RegisterWithProcessor(*this);
}

void UMythicWitnessPerceptionProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) {
    // Potential witnesses: any entity with an identity fragment (all tiers)
    AllEntitiesQuery.AddRequirement<FMythicIdentityFragment>(EMassFragmentAccess::ReadOnly);
    AllEntitiesQuery.AddRequirement<FMythicSignificanceFragment>(EMassFragmentAccess::ReadWrite);
}

void UMythicWitnessPerceptionProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicWitnessPerception_Execute);

    UWorld *World = GetWorld();
    if (!World) {
        return;
    }

    // Resolve action event subsystem (cached for performance)
    if (!CachedActionSubsystem.IsValid()) {
        CachedActionSubsystem = World->GetSubsystem<UMythicActionEventSubsystem>();
    }

    UMythicActionEventSubsystem *ActionSub = CachedActionSubsystem.Get();
    if (!ActionSub) {
        return;
    }

    TArray<FMythicPendingActionEvent> &PendingEvents = ActionSub->GetPendingEvents();
    if (PendingEvents.Num() == 0) {
        return; // Event-driven: no pending events = zero cost
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

    UMythicFactionDatabase *FactionDB = LWS->GetFactionDatabase();
    if (!FactionDB) {
        return;
    }

    // REQ-BEH-007: NPC perception (hearing range) degrades at NIGHT and in designer-flagged bad WEATHER. Drive the
    // ActionEventSubsystem's perception multiplier from the real time-of-day + active weather (the env subsystem owns
    // the clock; the controller owns the weather). Night and weather STACK multiplicatively, so a storm at night is the
    // worst (crimes go least noticed). The weather factor is per-weather data-driven — UWeatherType::bImpairsPerception
    // chosen by the designer (no baked fog-density threshold), with the shared WeatherPerceptionMultiplier magnitude.
    float EnvPerceptionMul = 1.0f;
    if (const UMythicEnvironmentSubsystem *Env = GI->GetSubsystem<UMythicEnvironmentSubsystem>()) {
        // Gate on a REGISTERED controller: GetDayTime() FAIL-UNSAFE-returns Night (and logs) when the controller is
        // absent (a startup/teardown transient), which would wrongly halve hearing AND spam a per-frame log. Only
        // degrade when the clock is genuinely known; "no clock yet" means full daylight perception.
        if (const AMythicEnvironmentController *Ctrl = Env->GetEnvironmentController()) {
            if (Env->GetDayTime() == EDayTime::Night) {
                EnvPerceptionMul *= Settings->NightPerceptionMultiplier;
            }
            // Active bad weather (designer-flagged) further reduces perception — finally wires the previously-dead
            // WeatherPerceptionMultiplier setting through a real per-weather signal.
            if (const UWeatherType *Weather = Ctrl->GetCurrentWeather()) {
                if (Weather->bImpairsPerception) {
                    EnvPerceptionMul *= Settings->WeatherPerceptionMultiplier;
                }
            }
        }
    }
    ActionSub->SetPerceptionMultiplier(EnvPerceptionMul);

    // Apply perception multiplier — weather/night/stealth reduce effective hearing range
    const float PerceptionMul = ActionSub->GetPerceptionMultiplier();
    const int32 BaseHearingRadius = Settings->WitnessHearingRadius;
    const int32 EffectiveHearingRadius = FMath::Max(1, FMath::RoundToInt32(BaseHearingRadius * PerceptionMul));
    int32 WitnessBudget = Settings->MaxWitnessEvalsPerFrame;
    TArray<FMythicWitnessResult> &WitnessResults = ActionSub->GetPendingWitnessResults();
    FMythicCrimeReportQueue &CrimeQueue = ActionSub->GetCrimeReportQueue();

    // Build the broad-phase cell→entity index ONCE this tick (single O(N) pass over the SAME entity set
    // AllEntitiesQuery scanned before), so the per-event witness lookup below queries only the hearing-radius
    // neighborhood instead of re-scanning ALL entities for every event — the O(events × N) → O(N + events × box) fix.
    // Reset retains allocations (no per-tick churn). Indexing the same set means witnesses are unchanged vs the old scan.
    SpatialIndex.Reset();
    AllEntitiesQuery.ForEachEntityChunk(Context, [this](FMassExecutionContext &ChunkContext) {
        const int32 NumEntities = ChunkContext.GetNumEntities();
        const auto IdentityView = ChunkContext.GetFragmentView<FMythicIdentityFragment>();
        for (int32 i = 0; i < NumEntities; ++i) {
            SpatialIndex.Insert(IdentityView[i].Cell, ChunkContext.GetEntity(i));
        }
    });

    // Process each pending event ATOMICALLY: once an event is STARTED it is scanned to completion this frame, so its
    // witness results + crime records are emitted exactly once. The budget caps how many EVENTS begin per frame, NOT
    // the scan within one. Why atomic: the previous code aborted an event's scan mid-way when the per-frame budget ran
    // out but left it un-flushed (bFullyProcessed only set when budget remained), so next frame the scan restarted at
    // entity 0 and RE-EMITTED witness results + crime records for witnesses it had already counted — duplicate pressure
    // and inflated crime confidence under exactly the heavy-witness load (a crowd watching a massacre) this system
    // exists for. `WitnessesProcessed` was added "for budget-capped multi-frame processing" but was never read as a
    // resume cursor, and a raw entity index is not a safe cursor (MASS chunk order shifts as entities spawn/despawn/
    // migrate between frames → it would skip or double-count). Atomic per-event fulfils that multi-frame intent
    // correctly: unstarted events wait for a later frame; a started event never leaves partial state. NOTE: this makes
    // MaxWitnessEvalsPerFrame a SOFT per-event-granular cap — a single high-witness event may overshoot it in one frame
    // (bounded by witnesses near ONE event, not all entities). A hard within-event cap needs a stable resume cursor,
    // which the cell-ordered spatial index (see BACKLOG: O(E×N) witness scan) provides — tracked as the follow-on.
    for (FMythicPendingActionEvent &PendingEvent : PendingEvents) {
        if (PendingEvent.bFullyProcessed) {
            continue; // already emitted on an earlier frame; awaiting FlushProcessedEvents
        }
        if (WitnessBudget <= 0) {
            break; // no budget left to START another event this frame — it stays queued, processed next frame
        }

        const FMythicWorldEvent &Event = PendingEvent.WorldEvent;
        const FMythicCellCoord &EventCell = Event.Cell;

        // Track per-event crime witness count for confidence scoring. int32 accumulator, saturated into the uint16
        // DirectWitnessCount field below — a >65535-witness crowd clamps instead of wrapping (the uint8 it replaced
        // wrapped at 256, silently corrupting the tally for large public crimes).
        int32 EventCrimeWitnessCount = 0;

        // BROAD PHASE: candidate witnesses = entities in cells within the hearing-radius BOX of the event cell. The
        // Chebyshev box (|dx|,|dy| <= r) is a SUPERSET of the Manhattan hearing ball the narrow phase re-checks below,
        // so the witnesses produced are IDENTICAL to the former full-entity scan — this only shrinks the examined set
        // from ALL entities to those near the event. NO mid-scan budget abort — a started event is scanned fully so its
        // emissions are complete and never duplicated (WitnessBudget may go negative here; the between-events check
        // above then prevents the NEXT event from starting this frame). Fragments are fetched by handle from the
        // EntityManager (entities don't despawn mid-Execute; the null guard is defensive).
        WitnessCandidates.Reset();
        SpatialIndex.QueryRange(EventCell, EffectiveHearingRadius, WitnessCandidates);

        for (const FMassEntityHandle WitnessEntity : WitnessCandidates) {
                const FMythicIdentityFragment *IdPtr = EntityManager.GetFragmentDataPtr<FMythicIdentityFragment>(WitnessEntity);
                FMythicSignificanceFragment *SigPtr = EntityManager.GetFragmentDataPtr<FMythicSignificanceFragment>(WitnessEntity);
                if (!IdPtr || !SigPtr) {
                    continue;
                }
                const FMythicIdentityFragment &Identity = *IdPtr;

                // Cell-distance hearing check — O(1) integer math
                const int32 CellDist = FMath::Abs(Identity.Cell.X - EventCell.X)
                    + FMath::Abs(Identity.Cell.Y - EventCell.Y);

                if (CellDist > EffectiveHearingRadius) {
                    continue; // Out of hearing range (reduced by weather/night perception)
                }

                // ─── Visibility Group Check (REQ-CRM-002) ───
                // Byte compare — O(1), no raycasts. Entities in different visibility
                // groups cannot see each other. Group 0 = default (visible to all).
                // Cover/stealth sets player to a non-zero group → byte compare fails → not witnessed.
                if (Identity.VisibilityGroup != 0 && Identity.VisibilityGroup != Event.VisibilityGroup) {
                    continue; // Line-of-sight blocked (different visibility groups)
                }

                // Reject entities that can contribute no witness/crime output BEFORE spending budget — a
                // faction-less entity (e.g. an unaffiliated creature) or one whose faction isn't in the DB produces
                // nothing downstream, so charging it budget only starves real witnesses (e.g. a crowd near a kill).
                if (!Identity.Faction.IsValid()) {
                    continue;
                }
                FMythicFactionMoralProfile FactionProfile;
                if (!FactionDB->GetFactionMoralProfile(Identity.Faction, FactionProfile)) {
                    continue;
                }

                --WitnessBudget;
                ++PendingEvent.WitnessesProcessed;

                // Moral evaluation — dot product of action vs witness's faction ideology
                FMythicMoralAction EvaluatedVector = Event.MoralVector;

                // Category-specific moral axis mapping (REQ-BEH-010)
                // Inject additional moral dimensions based on the action type
                switch (Event.ActionCategory) {
                case EMythicActionCategory::Magic_Damage:
                    // Destructive magic is viewed through both Violence and Arcane lenses
                    EvaluatedVector.AxisValues[static_cast<int32>(EMythicMoralAxis::Arcane)] += 0.5f;
                    break;
                case EMythicActionCategory::Magic_Healing:
                    // Healing magic is Mercy + Arcane
                    EvaluatedVector.AxisValues[static_cast<int32>(EMythicMoralAxis::Arcane)] += 0.3f;
                    EvaluatedVector.AxisValues[static_cast<int32>(EMythicMoralAxis::Mercy)] += 0.5f;
                    break;
                case EMythicActionCategory::Magic_Forbidden:
                    // Forbidden magic DESECRATES — negative Sanctity (the absence of sanctity, exactly as a kill is
                    // negative Mercy) — and transgresses anti-magic ideology (positive Arcane). Severity = -dot, so a
                    // sanctity-protecting faction (Ideology.Sanctity = +1) only condemns a NEGATIVE-Sanctity action.
                    // The prior += 1.0f inverted the sign, so protector factions never condemned necromancy.
                    EvaluatedVector.AxisValues[static_cast<int32>(EMythicMoralAxis::Sanctity)] -= 1.0f;
                    EvaluatedVector.AxisValues[static_cast<int32>(EMythicMoralAxis::Arcane)] += 0.8f;
                    break;
                default:
                    break;
                }

                const EMythicMoralSeverity Severity = FMythicMoralSignature::EvaluateActionSeverity(
                    EvaluatedVector,
                    FactionProfile.Ideology,
                    FactionProfile.DisapproveThreshold,
                    FactionProfile.CondemnThreshold,
                    FactionProfile.HostileThreshold
                    );

                if (Severity == EMythicMoralSeverity::Ignore) {
                    continue; // No reaction needed
                }

                // Dirty the significance score — this entity is now contextually relevant
                FMythicSignificanceFragment &Significance = *SigPtr;
                Significance.bDirty = true;
                // Saturate in 32-bit before narrowing (FMath::Min<uint16> truncates the +1 sum first: at 65535 -> 65536 -> 0).
                Significance.RelevantEventCount = static_cast<uint16>(FMath::Min<uint32>(static_cast<uint32>(Significance.RelevantEventCount) + 1u, 0xFFFFu));

                // Produce witness result for the pressure processor
                FMythicWitnessResult Result;
                Result.WitnessEntity = WitnessEntity;
                Result.Severity = Severity;
                Result.ActionMoralVector = Event.MoralVector;
                Result.EventCategoryFlags = Event.CategoryFlags;
                Result.EventSignificance = Event.Significance;
                Result.EventWorldTime = Event.WorldTime;
                Result.EventCell = Event.Cell;
                Result.PerpFaction = Event.PrimaryFaction;
                WitnessResults.Add(Result);

                // ─── Crime Record Generation (REQ-CRM-001, REQ-CRM-004) ───
                // When severity >= Condemn, this action IS a crime for the witness's faction.
                // Crime is defined by faction moral surface, not hardcoded categories.
                // The record is fed into belief propagation for rate-limited reporting.
                if (Severity >= EMythicMoralSeverity::Condemn) {
                    ++EventCrimeWitnessCount;
                    FMythicCrimeRecord Crime;
                    Crime.PerpFaction = Event.PrimaryFaction;
                    Crime.ViolatedFaction = Identity.Faction;
                    Crime.Severity = Severity;
                    Crime.ActionMoralVector = Event.MoralVector;
                    Crime.Cell = EventCell;
                    Crime.WorldTime = Event.WorldTime;
                    Crime.DirectWitnessCount = static_cast<uint16>(FMath::Min(EventCrimeWitnessCount, 0xFFFF));
                    Crime.Confidence = 1.0f; // Direct witness = full confidence
                    CrimeQueue.Enqueue(Crime);
                }

                UE_LOG(LogMythLivingWorld, Verbose, TEXT("Witness: Entity in cell %s saw event at %s — Severity=%d"),
                       *Identity.Cell.ToString(), *EventCell.ToString(), static_cast<int32>(Severity));
        }

        // The event was scanned to completion (no mid-scan abort), so its witnesses are fully emitted exactly once.
        // Always mark processed → FlushProcessedEvents removes it and it is never re-scanned (the source of the former
        // duplicate emissions). WitnessesProcessed now holds this event's final processed-witness tally (a diagnostic).
        PendingEvent.bFullyProcessed = true;
    }

    // Flush fully processed events
    ActionSub->FlushProcessedEvents();
}

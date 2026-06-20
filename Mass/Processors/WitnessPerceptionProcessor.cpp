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

    // REQ-BEH-007: NPC perception (hearing range) degrades at NIGHT. Drive the ActionEventSubsystem's perception
    // multiplier from the real time-of-day (the env subsystem owns the clock) — SetPerceptionMultiplier was previously
    // NEVER called, so the multiplier sat at 1.0 regardless of the clock, and NightPerceptionMultiplier was dead.
    // (Weather-based degradation is deferred: UWeatherType exposes only visual fog ranges, not a gameplay perception
    // scalar — a per-weather perception multiplier should be designer-authored before WeatherPerceptionMultiplier is
    // wired, rather than baking in a fog-density threshold here.)
    float EnvPerceptionMul = 1.0f;
    if (const UMythicEnvironmentSubsystem *Env = GI->GetSubsystem<UMythicEnvironmentSubsystem>()) {
        // Gate on a REGISTERED controller: GetDayTime() FAIL-UNSAFE-returns Night (and logs) when the controller is
        // absent (a startup/teardown transient), which would wrongly halve hearing AND spam a per-frame log. Only
        // degrade when the clock is genuinely known; "no clock yet" means full daylight perception.
        if (Env->GetEnvironmentController() != nullptr && Env->GetDayTime() == EDayTime::Night) {
            EnvPerceptionMul = Settings->NightPerceptionMultiplier;
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

    // Process each pending event, budget-capped
    for (FMythicPendingActionEvent &PendingEvent : PendingEvents) {
        if (PendingEvent.bFullyProcessed || WitnessBudget <= 0) {
            continue;
        }

        const FMythicWorldEvent &Event = PendingEvent.WorldEvent;
        const FMythicCellCoord &EventCell = Event.Cell;

        // Track per-event crime witness count for confidence scoring
        uint8 EventCrimeWitnessCount = 0;

        // For each entity chunk, scan for witnesses near the event cell
        AllEntitiesQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext &ChunkContext) {
            if (WitnessBudget <= 0) {
                return;
            }

            const int32 NumEntities = ChunkContext.GetNumEntities();
            const auto IdentityView = ChunkContext.GetFragmentView<FMythicIdentityFragment>();
            auto SignificanceView = ChunkContext.GetMutableFragmentView<FMythicSignificanceFragment>();

            for (int32 i = 0; i < NumEntities && WitnessBudget > 0; ++i) {
                const FMythicIdentityFragment &Identity = IdentityView[i];

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
                FMythicFactionData FactionData;
                if (!FactionDB->GetFaction(Identity.Faction, FactionData)) {
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
                    FactionData.Ideology,
                    FactionData.DisapproveThreshold,
                    FactionData.CondemnThreshold,
                    FactionData.HostileThreshold
                    );

                if (Severity == EMythicMoralSeverity::Ignore) {
                    continue; // No reaction needed
                }

                // Dirty the significance score — this entity is now contextually relevant
                FMythicSignificanceFragment &Significance = SignificanceView[i];
                Significance.bDirty = true;
                // Saturate in 32-bit before narrowing (FMath::Min<uint16> truncates the +1 sum first: at 65535 -> 65536 -> 0).
                Significance.RelevantEventCount = static_cast<uint16>(FMath::Min<uint32>(static_cast<uint32>(Significance.RelevantEventCount) + 1u, 0xFFFFu));

                // Produce witness result for the pressure processor
                FMythicWitnessResult Result;
                Result.WitnessEntity = ChunkContext.GetEntity(i);
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
                    Crime.DirectWitnessCount = EventCrimeWitnessCount;
                    Crime.Confidence = 1.0f; // Direct witness = full confidence
                    CrimeQueue.Enqueue(Crime);
                }

                UE_LOG(LogMythLivingWorld, Verbose, TEXT("Witness: Entity in cell %s saw event at %s — Severity=%d"),
                       *Identity.Cell.ToString(), *EventCell.ToString(), static_cast<int32>(Severity));
            }
        });

        // Mark event as fully processed if budget allowed full scan
        if (WitnessBudget > 0) {
            PendingEvent.bFullyProcessed = true;
        }
    }

    // Flush fully processed events
    ActionSub->FlushProcessedEvents();
}

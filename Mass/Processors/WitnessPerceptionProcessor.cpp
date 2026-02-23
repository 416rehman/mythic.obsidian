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

    const int32 HearingRadius = Settings->WitnessHearingRadius;
    int32 WitnessBudget = Settings->MaxWitnessEvalsPerFrame;
    TArray<FMythicWitnessResult> &WitnessResults = ActionSub->GetPendingWitnessResults();

    // Process each pending event, budget-capped
    for (FMythicPendingActionEvent &PendingEvent : PendingEvents) {
        if (PendingEvent.bFullyProcessed || WitnessBudget <= 0) {
            continue;
        }

        const FMythicWorldEvent &Event = PendingEvent.WorldEvent;
        const FMythicCellCoord &EventCell = Event.Cell;

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

                if (CellDist > HearingRadius) {
                    continue; // Out of hearing range
                }

                // Skip entities from the perpetrator's faction witnessing their own faction's sanctioned actions
                // (guards don't report their own faction's law enforcement, etc.)
                // Full same-faction logic deferred to Phase 5 with role-based responses

                --WitnessBudget;
                ++PendingEvent.WitnessesProcessed;

                // Moral evaluation — dot product of action vs witness's faction ideology
                if (!Identity.Faction.IsValid()) {
                    continue;
                }

                FMythicFactionData FactionData;
                if (!FactionDB->GetFaction(Identity.Faction, FactionData)) {
                    continue;
                }

                const EMythicMoralSeverity Severity = FMythicMoralSignature::EvaluateActionSeverity(
                    Event.MoralVector,
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
                Significance.RelevantEventCount = FMath::Min<uint16>(Significance.RelevantEventCount + 1, 0xFFFF);

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

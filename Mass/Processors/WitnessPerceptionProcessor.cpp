
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
#include "World/EnvironmentController/MythicEnvironmentSubsystem.h"
#include "World/EnvironmentController/MythicEnvironmentController.h"
#include "Engine/World.h"

UMythicWitnessPerceptionProcessor::UMythicWitnessPerceptionProcessor() {
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
    ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);
    bRequiresGameThreadExecution = true;
    bAutoRegisterWithProcessingPhases = true;

    ExecutionOrder.ExecuteAfter.Add(TEXT("UMythicPopulationSpawnerProcessor"));
    ExecutionOrder.ExecuteAfter.Add(TEXT("UMythicCreatureEcologyProcessor"));

    AllEntitiesQuery.RegisterWithProcessor(*this);
}

void UMythicWitnessPerceptionProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) {
    AllEntitiesQuery.AddRequirement<FMythicIdentityFragment>(EMassFragmentAccess::ReadOnly);
    AllEntitiesQuery.AddRequirement<FMythicSignificanceFragment>(EMassFragmentAccess::ReadWrite);
}

void UMythicWitnessPerceptionProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicWitnessPerception_Execute);

    UWorld *World = GetWorld();
    if (!World) {
        return;
    }

    if (!CachedActionSubsystem.IsValid()) {
        CachedActionSubsystem = World->GetSubsystem<UMythicActionEventSubsystem>();
    }

    UMythicActionEventSubsystem *ActionSub = CachedActionSubsystem.Get();
    if (!ActionSub) {
        return;
    }

    TArray<FMythicPendingActionEvent> &PendingEvents = ActionSub->GetPendingEvents();
    if (PendingEvents.Num() == 0) {
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

    UMythicFactionDatabase *FactionDB = LWS->GetFactionDatabase();
    if (!FactionDB) {
        return;
    }

    float EnvPerceptionMul = 1.0f;
    if (const UMythicEnvironmentSubsystem *Env = GI->GetSubsystem<UMythicEnvironmentSubsystem>()) {
        if (const AMythicEnvironmentController *Ctrl = Env->GetEnvironmentController()) {
            if (Env->GetDayTime() == EDayTime::Night) {
                EnvPerceptionMul *= Settings->NightPerceptionMultiplier;
            }
            if (const UWeatherType *Weather = Ctrl->GetCurrentWeather()) {
                if (Weather->bImpairsPerception) {
                    EnvPerceptionMul *= Settings->WeatherPerceptionMultiplier;
                }
            }
        }
    }
    ActionSub->SetPerceptionMultiplier(EnvPerceptionMul);

    const float PerceptionMul = ActionSub->GetPerceptionMultiplier();
    const int32 BaseHearingRadius = Settings->WitnessHearingRadius;
    const int32 EffectiveHearingRadius = FMath::Max(1, FMath::RoundToInt32(BaseHearingRadius * PerceptionMul));
    int32 WitnessBudget = Settings->MaxWitnessEvalsPerFrame;
    TArray<FMythicWitnessResult> &WitnessResults = ActionSub->GetPendingWitnessResults();
    FMythicCrimeReportQueue &CrimeQueue = ActionSub->GetCrimeReportQueue();

    SpatialIndex.Reset();
    AllEntitiesQuery.ForEachEntityChunk(Context, [this](FMassExecutionContext &ChunkContext) {
        const int32 NumEntities = ChunkContext.GetNumEntities();
        const auto IdentityView = ChunkContext.GetFragmentView<FMythicIdentityFragment>();
        for (int32 i = 0; i < NumEntities; ++i) {
            SpatialIndex.Insert(IdentityView[i].Cell, ChunkContext.GetEntity(i));
        }
    });

    for (FMythicPendingActionEvent &PendingEvent : PendingEvents) {
        if (PendingEvent.bFullyProcessed) {
            continue;
        }
        if (WitnessBudget <= 0) {
            break;
        }

        const FMythicWorldEvent &Event = PendingEvent.WorldEvent;
        const FMythicCellCoord &EventCell = Event.Cell;

        const int32 EventHearingRadius = FMath::Max(1, FMath::RoundToInt32(
            ComputeStealthPerceptionRange(static_cast<float>(EffectiveHearingRadius), PendingEvent.StealthPerceptionScale)));

        int32 EventCrimeWitnessCount = 0;

        WitnessCandidates.Reset();
        SpatialIndex.QueryRange(EventCell, EventHearingRadius, WitnessCandidates);

        for (const FMassEntityHandle WitnessEntity : WitnessCandidates) {
                const FMythicIdentityFragment *IdPtr = EntityManager.GetFragmentDataPtr<FMythicIdentityFragment>(WitnessEntity);
                FMythicSignificanceFragment *SigPtr = EntityManager.GetFragmentDataPtr<FMythicSignificanceFragment>(WitnessEntity);
                if (!IdPtr || !SigPtr) {
                    continue;
                }
                const FMythicIdentityFragment &Identity = *IdPtr;

                const int32 CellDist = FMath::Abs(Identity.Cell.X - EventCell.X)
                    + FMath::Abs(Identity.Cell.Y - EventCell.Y);

                if (CellDist > EventHearingRadius) {
                    continue;
                }

                if (Identity.VisibilityGroup != 0 && Identity.VisibilityGroup != Event.VisibilityGroup) {
                    continue;
                }

                if (!Identity.Faction.IsValid()) {
                    continue;
                }
                FMythicFactionMoralProfile FactionProfile;
                if (!FactionDB->GetFactionMoralProfile(Identity.Faction, FactionProfile)) {
                    continue;
                }

                --WitnessBudget;
                ++PendingEvent.WitnessesProcessed;

                FMythicMoralAction EvaluatedVector = Event.MoralVector;

                switch (Event.ActionCategory) {
                case EMythicActionCategory::Magic_Damage:
                    EvaluatedVector.AxisValues[static_cast<int32>(EMythicMoralAxis::Arcane)] += 0.5f;
                    break;
                case EMythicActionCategory::Magic_Healing:
                    EvaluatedVector.AxisValues[static_cast<int32>(EMythicMoralAxis::Arcane)] += 0.3f;
                    EvaluatedVector.AxisValues[static_cast<int32>(EMythicMoralAxis::Mercy)] += 0.5f;
                    break;
                case EMythicActionCategory::Magic_Forbidden:
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
                    continue;
                }

                FMythicSignificanceFragment &Significance = *SigPtr;
                Significance.bDirty = true;
                Significance.RelevantEventCount = static_cast<uint16>(FMath::Min<uint32>(static_cast<uint32>(Significance.RelevantEventCount) + 1u, 0xFFFFu));

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

                if (Severity >= EMythicMoralSeverity::Condemn) {
                    ++EventCrimeWitnessCount;
                    FMythicCrimeRecord Crime;
                    Crime.PerpFaction = Event.PrimaryFaction;
                    Crime.PerpPlayerKey = PendingEvent.PerpPlayerKey;
                    Crime.ViolatedFaction = Identity.Faction;
                    Crime.Severity = Severity;
                    Crime.ActionMoralVector = Event.MoralVector;
                    Crime.Cell = EventCell;
                    Crime.WorldTime = Event.WorldTime;
                    Crime.DirectWitnessCount = static_cast<uint16>(FMath::Min(EventCrimeWitnessCount, 0xFFFF));
                    Crime.Confidence = 1.0f;
                    CrimeQueue.Enqueue(Crime);
                }

                UE_LOG(LogMythLivingWorld, Verbose, TEXT("Witness: Entity in cell %s saw event at %s — Severity=%d"),
                       *Identity.Cell.ToString(), *EventCell.ToString(), static_cast<int32>(Severity));
        }

        PendingEvent.bFullyProcessed = true;
    }

    ActionSub->FlushProcessedEvents();
}

float UMythicWitnessPerceptionProcessor::ComputeStealthPerceptionRange(float BaseRange, float StealthScale) {
    return BaseRange * FMath::Clamp(StealthScale, 0.0f, 1.0f);
}

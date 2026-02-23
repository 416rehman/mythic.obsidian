// Mythic Living World — Pressure Accumulation & Venting Implementation

#include "Mass/Processors/PressureProcessor.h"
#include "MassEntitySubsystem.h"
#include "MassExecutionContext.h"
#include "MassCommandBuffer.h"
#include "Mass/Fragments/MythicMassFragments.h"
#include "Mass/Tags/MythicMassTags.h"
#include "World/LivingWorld/Events/ActionEventSubsystem.h"
#include "World/LivingWorld/Events/ActionEventTypes.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "Engine/World.h"

UMythicPressureProcessor::UMythicPressureProcessor() {
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
    ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);
    bRequiresGameThreadExecution = true; // Accesses world subsystems
    bAutoRegisterWithProcessingPhases = true;

    // Run after witness perception has produced results
    ExecutionOrder.ExecuteAfter.Add(TEXT("UMythicWitnessPerceptionProcessor"));

    HydratedEntityQuery.RegisterWithProcessor(*this);
}

void UMythicPressureProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) {
    HydratedEntityQuery.AddRequirement<FMythicIdentityFragment>(EMassFragmentAccess::ReadOnly);
    HydratedEntityQuery.AddRequirement<FMythicPsychodynamicFragment>(EMassFragmentAccess::ReadWrite);
    HydratedEntityQuery.AddRequirement<FMythicPersonalityFragment>(EMassFragmentAccess::ReadOnly);
    HydratedEntityQuery.AddRequirement<FMythicSignificanceFragment>(EMassFragmentAccess::ReadWrite);
    HydratedEntityQuery.AddTagRequirement<FMythicHydratedTag>(EMassFragmentPresence::All);
}

void UMythicPressureProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicPressure_Execute);

    UWorld *World = GetWorld();
    if (!World) {
        return;
    }

    // Resolve action event subsystem
    if (!CachedActionSubsystem.IsValid()) {
        CachedActionSubsystem = World->GetSubsystem<UMythicActionEventSubsystem>();
    }

    UMythicActionEventSubsystem *ActionSub = CachedActionSubsystem.Get();
    if (!ActionSub) {
        return;
    }

    TArray<FMythicWitnessResult> &WitnessResults = ActionSub->GetPendingWitnessResults();
    if (WitnessResults.Num() == 0) {
        return; // Event-driven: zero cost when idle
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

    const float DecayRate = Settings->PressureDecayRate;
    const float VentThreshold = Settings->VentThreshold;
    int32 PressureBudget = Settings->MaxPressureEvalsPerFrame;
    const double CurrentWorldTime = World->GetTimeSeconds();

    const int32 ThreatIdx = static_cast<int32>(EMythicPressureChannel::Threat);
    const int32 InjusticeIdx = static_cast<int32>(EMythicPressureChannel::Injustice);
    const int32 GriefIdx = static_cast<int32>(EMythicPressureChannel::Grief);
    const int32 WrathIdx = static_cast<int32>(EMythicPressureChannel::Wrath);

    // Build a map from entity handle → witness results for O(1) lookup during chunk iteration
    TMap<FMassEntityHandle, TArray<const FMythicWitnessResult *>> EntityWitnessMap;
    EntityWitnessMap.Reserve(WitnessResults.Num());

    const int32 MaxToProcess = FMath::Min(WitnessResults.Num(), PressureBudget);
    for (int32 i = 0; i < MaxToProcess; ++i) {
        EntityWitnessMap.FindOrAdd(WitnessResults[i].WitnessEntity).Add(&WitnessResults[i]);
    }

    // Process entities that have pending witness results
    HydratedEntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext &ChunkContext) {
        if (PressureBudget <= 0) {
            return;
        }

        const int32 NumEntities = ChunkContext.GetNumEntities();
        auto PsychoView = ChunkContext.GetMutableFragmentView<FMythicPsychodynamicFragment>();
        const auto PersonalityView = ChunkContext.GetFragmentView<FMythicPersonalityFragment>();
        auto SignificanceView = ChunkContext.GetMutableFragmentView<FMythicSignificanceFragment>();

        for (int32 i = 0; i < NumEntities && PressureBudget > 0; ++i) {
            const FMassEntityHandle Entity = ChunkContext.GetEntity(i);
            const TArray<const FMythicWitnessResult *> *Results = EntityWitnessMap.Find(Entity);
            if (!Results) {
                continue; // This entity has no pending witness results
            }

            FMythicPsychodynamicFragment &Psycho = PsychoView[i];
            const FMythicPersonalityFragment &Personality = PersonalityView[i];
            FMythicSignificanceFragment &Significance = SignificanceView[i];

            // ─── Lazy Decay ───
            // O(1): exponential decay since last event, computed only when touched
            const double Elapsed = CurrentWorldTime - Psycho.LastEventTime;
            if (Elapsed > 0.0 && Psycho.LastEventTime > 0.0) {
                const float DecayMultiplier = FMath::Exp(-DecayRate * static_cast<float>(Elapsed));
                for (int32 c = 0; c < PressureChannelCount; ++c) {
                    Psycho.Pressure[c] *= DecayMultiplier;
                }
            }

            // ─── Accumulate Pressure ───
            for (const FMythicWitnessResult *Result : *Results) {
                --PressureBudget;

                // Severity-based magnitude scaling
                float SeverityMagnitude = 0.0f;
                switch (Result->Severity) {
                case EMythicMoralSeverity::Disapprove:
                    SeverityMagnitude = 0.3f * Result->EventSignificance;
                    break;
                case EMythicMoralSeverity::Condemn:
                    SeverityMagnitude = 0.7f * Result->EventSignificance;
                    break;
                case EMythicMoralSeverity::Hostile:
                    SeverityMagnitude = 1.0f * Result->EventSignificance;
                    break;
                default:
                    continue; // Ignore — should never reach here
                }

                // Map category flags → pressure channels
                // Combat/violence events → Threat + Wrath
                const bool bIsCombat = (Result->EventCategoryFlags & 0x01) != 0;
                // Crime events → Injustice
                const bool bIsCrime = (Result->EventCategoryFlags & 0x02) != 0;
                // Death events → Grief
                const bool bIsDeath = (Result->EventCategoryFlags & 0x04) != 0;

                if (bIsCombat) {
                    Psycho.Pressure[ThreatIdx] += SeverityMagnitude;
                    Psycho.Pressure[WrathIdx] += SeverityMagnitude * 0.5f;
                }
                if (bIsCrime) {
                    Psycho.Pressure[InjusticeIdx] += SeverityMagnitude;
                }
                if (bIsDeath) {
                    Psycho.Pressure[GriefIdx] += SeverityMagnitude * 0.8f;
                }
                // Default: if no specific category, apply to Threat at half magnitude
                if (!bIsCombat && !bIsCrime && !bIsDeath) {
                    Psycho.Pressure[ThreatIdx] += SeverityMagnitude * 0.5f;
                }
            }

            Psycho.LastEventTime = CurrentWorldTime;

            // ─── Vent Check ───
            // Find the pressure channel with the highest value
            float MaxPressure = 0.0f;
            int32 MaxPressureChannel = -1;
            for (int32 c = 0; c < PressureChannelCount; ++c) {
                if (Psycho.Pressure[c] > MaxPressure) {
                    MaxPressure = Psycho.Pressure[c];
                    MaxPressureChannel = c;
                }
            }

            if (MaxPressure >= VentThreshold && MaxPressureChannel >= 0) {
                // Route through personality — pick the vent channel with highest weight
                float BestVentWeight = -1.0f;
                int32 BestVentChannel = 0;
                for (int32 v = 0; v < VentChannelCount; ++v) {
                    if (Personality.VentWeights[v] > BestVentWeight) {
                        BestVentWeight = Personality.VentWeights[v];
                        BestVentChannel = v;
                    }
                }

                // Reduce pressure after venting (release half the peak pressure)
                Psycho.Pressure[MaxPressureChannel] *= 0.5f;

                UE_LOG(LogMythLivingWorld, Verbose, TEXT("Pressure vent: Entity vented via channel %d (pressure=%.2f, threshold=%.2f)"),
                       BestVentChannel, MaxPressure, VentThreshold);
            }

            // Dirty significance — pressure change affects significance score
            Significance.bDirty = true;
        }
    });

    // Flush consumed witness results
    ActionSub->FlushProcessedWitnessResults();
}

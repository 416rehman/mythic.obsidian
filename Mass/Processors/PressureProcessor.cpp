// Mythic Living World — Pressure Accumulation & Venting Implementation
// Handles: pressure accumulation, lazy decay, personality-routed venting,
// guard Enforce, emotional contagion, mob dynamics, and despair detection.

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
    const float DespairThreshold = Settings->DespairThreshold;
    const float GuardAssistRadius = Settings->GuardAssistRadius;
    const float EmotionalContagionRadius = Settings->EmotionalContagionRadius;
    const int32 MobFormationThreshold = Settings->MobFormationThreshold;
    int32 PressureBudget = Settings->MaxPressureEvalsPerFrame;
    const double CurrentWorldTime = World->GetTimeSeconds();

    const int32 ThreatIdx = static_cast<int32>(EMythicPressureChannel::Threat);
    const int32 InjusticeIdx = static_cast<int32>(EMythicPressureChannel::Injustice);
    const int32 GriefIdx = static_cast<int32>(EMythicPressureChannel::Grief);
    const int32 WrathIdx = static_cast<int32>(EMythicPressureChannel::Wrath);
    const int32 ShameIdx = static_cast<int32>(EMythicPressureChannel::Shame);
    const int32 DesireIdx = static_cast<int32>(EMythicPressureChannel::Desire);

    // Vent channel indices
    const int32 EnforceVent = static_cast<int32>(EMythicVentChannel::Enforce);
    const int32 FightVent = static_cast<int32>(EMythicVentChannel::Fight);
    const int32 FleeVent = static_cast<int32>(EMythicVentChannel::Flee);

    // ─── Phase 1: Build entity → witness results map ───
    TMap<FMassEntityHandle, TArray<const FMythicWitnessResult *>> EntityWitnessMap;
    EntityWitnessMap.Reserve(WitnessResults.Num());

    const int32 MaxToProcess = FMath::Min(WitnessResults.Num(), PressureBudget);
    for (int32 i = 0; i < MaxToProcess; ++i) {
        EntityWitnessMap.FindOrAdd(WitnessResults[i].WitnessEntity).Add(&WitnessResults[i]);
    }

    // ─── Phase 2: Track venting for secondary effects (guard assist, contagion, mobs) ───
    // Deferred secondary pressure effects — applied next frame to avoid O(n²) within same frame
    struct FDeferredPressureBoost {
        FMythicCellCoord Cell;
        FMythicFactionId Faction;
        int32 PressureChannel;
        float Amount;
    };
    TArray<FDeferredPressureBoost> DeferredBoosts;
    DeferredBoosts.Reserve(16);

    // Track Fight targets for mob dynamics
    TMap<int32, int32> FightTargetCounts; // target entity index → count of fighters

    // ─── Phase 3: Process entities with pending witness results ───
    HydratedEntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext &ChunkContext) {
        if (PressureBudget <= 0) {
            return;
        }

        const int32 NumEntities = ChunkContext.GetNumEntities();
        const auto IdentityView = ChunkContext.GetFragmentView<FMythicIdentityFragment>();
        auto PsychoView = ChunkContext.GetMutableFragmentView<FMythicPsychodynamicFragment>();
        const auto PersonalityView = ChunkContext.GetFragmentView<FMythicPersonalityFragment>();
        auto SignificanceView = ChunkContext.GetMutableFragmentView<FMythicSignificanceFragment>();

        for (int32 i = 0; i < NumEntities && PressureBudget > 0; ++i) {
            const FMassEntityHandle Entity = ChunkContext.GetEntity(i);
            const TArray<const FMythicWitnessResult *> *Results = EntityWitnessMap.Find(Entity);
            if (!Results) {
                continue; // This entity has no pending witness results
            }

            const FMythicIdentityFragment &Identity = IdentityView[i];
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
                const bool bIsCombat = (Result->EventCategoryFlags & EMythicEventCategory::Combat) != 0;
                const bool bIsCrime = (Result->EventCategoryFlags & EMythicEventCategory::Crime) != 0;
                const bool bIsDeath = (Result->EventCategoryFlags & EMythicEventCategory::Death) != 0;
                const bool bIsMagic = (Result->EventCategoryFlags & EMythicEventCategory::Magic) != 0;
                const bool bIsEnvironment = (Result->EventCategoryFlags & EMythicEventCategory::Environment) != 0;

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
                // Magic/ability spectacle → extra Threat for non-magic NPCs
                if (bIsMagic) {
                    Psycho.Pressure[ThreatIdx] += SeverityMagnitude * 0.6f;
                }
                // Environmental danger → pure Threat
                if (bIsEnvironment) {
                    Psycho.Pressure[ThreatIdx] += SeverityMagnitude * 0.7f;
                }
                // Default: if no specific category, apply to Threat at half magnitude
                if (!bIsCombat && !bIsCrime && !bIsDeath && !bIsMagic && !bIsEnvironment) {
                    Psycho.Pressure[ThreatIdx] += SeverityMagnitude * 0.5f;
                }
            }

            Psycho.LastEventTime = CurrentWorldTime;

            // ─── Despair Detection (REQ-BEH-009) ───
            // Total unvented pressure across all channels exceeds threshold → despair
            float TotalPressure = 0.0f;
            for (int32 c = 0; c < PressureChannelCount; ++c) {
                TotalPressure += Psycho.Pressure[c];
            }
            if (TotalPressure >= DespairThreshold && !Psycho.bDespaired) {
                Psycho.bDespaired = true;
                UE_LOG(LogMythLivingWorld, Log, TEXT("Despair: Entity in cell %s reached despair (total=%.2f, threshold=%.2f)"),
                       *Identity.Cell.ToString(), TotalPressure, DespairThreshold);
            }

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
                // Guards (role tag) have elevated Enforce weight from personality generation
                float BestVentWeight = -1.0f;
                int32 BestVentChannel = 0;
                for (int32 v = 0; v < VentChannelCount; ++v) {
                    if (Personality.VentWeights[v] > BestVentWeight) {
                        BestVentWeight = Personality.VentWeights[v];
                        BestVentChannel = v;
                    }
                }

                // ─── Guard Assist Propagation (REQ-BEH-002) ───
                // When a guard vents via Enforce, nearby same-faction guards get assist boost
                if (BestVentChannel == EnforceVent) {
                    FDeferredPressureBoost Boost;
                    Boost.Cell = Identity.Cell;
                    Boost.Faction = Identity.Faction;
                    Boost.PressureChannel = InjusticeIdx;
                    Boost.Amount = MaxPressure * 0.3f; // 30% of enforcer's pressure
                    DeferredBoosts.Add(Boost);
                }

                // ─── Emotional Contagion (REQ-BEH-003) ───
                // Flee venting spreads Threat to nearby entities (budget-capped, 1 hop)
                if (BestVentChannel == FleeVent) {
                    FDeferredPressureBoost Boost;
                    Boost.Cell = Identity.Cell;
                    Boost.Faction = FMythicFactionId(); // Any faction — contagion crosses faction lines
                    Boost.PressureChannel = ThreatIdx;
                    Boost.Amount = MaxPressure * 0.2f; // 20% contagion transfer
                    DeferredBoosts.Add(Boost);
                }

                // ─── Mob Dynamics (REQ-BEH-005) ───
                // Track Fight target for mob formation
                if (BestVentChannel == FightVent && Psycho.FightTargetEntity != INDEX_NONE) {
                    FightTargetCounts.FindOrAdd(Psycho.FightTargetEntity)++;
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

    // ─── Phase 4: Apply mob bonuses ───
    // When enough entities share the same Fight target, grant mob bonus
    if (FightTargetCounts.Num() > 0) {
        HydratedEntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext &ChunkContext) {
            const int32 NumEntities = ChunkContext.GetNumEntities();
            auto PsychoView = ChunkContext.GetMutableFragmentView<FMythicPsychodynamicFragment>();

            for (int32 i = 0; i < NumEntities; ++i) {
                FMythicPsychodynamicFragment &Psycho = PsychoView[i];
                if (Psycho.FightTargetEntity == INDEX_NONE) {
                    continue;
                }

                const int32 *Count = FightTargetCounts.Find(Psycho.FightTargetEntity);
                if (Count && *Count >= MobFormationThreshold) {
                    // Mob formed: boost Fight pressure, reduce Threat (safety in numbers)
                    Psycho.Pressure[WrathIdx] += 0.3f;
                    Psycho.Pressure[ThreatIdx] *= 0.7f; // 30% Threat reduction from mob
                }
            }
        });
    }

    // ─── Phase 5: Store deferred boosts for next frame processing ───
    // Guard assist and emotional contagion are applied via the ActionEventSubsystem's
    // deferred pressure boost queue. This prevents O(n²) per-frame cascades.
    // The boosts are processed in the NEXT frame's pressure pass.
    // (For now, we process inline with a secondary chunk pass — budget-capped)
    if (DeferredBoosts.Num() > 0) {
        int32 DeferredBudget = 16; // Max deferred pressure applications per frame
        HydratedEntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext &ChunkContext) {
            if (DeferredBudget <= 0) {
                return;
            }
            const int32 NumEntities = ChunkContext.GetNumEntities();
            const auto IdentityView = ChunkContext.GetFragmentView<FMythicIdentityFragment>();
            auto PsychoView = ChunkContext.GetMutableFragmentView<FMythicPsychodynamicFragment>();

            for (int32 i = 0; i < NumEntities && DeferredBudget > 0; ++i) {
                const FMythicIdentityFragment &Identity = IdentityView[i];
                for (const FDeferredPressureBoost &Boost : DeferredBoosts) {
                    // Check cell radius
                    const int32 Dist = FMath::Abs(Identity.Cell.X - Boost.Cell.X)
                        + FMath::Abs(Identity.Cell.Y - Boost.Cell.Y);
                    if (Dist > 2) { // ~2 cell radius for secondary effects
                        continue;
                    }
                    // Faction check (if specified — contagion is faction-agnostic)
                    if (Boost.Faction.IsValid() && Identity.Faction.Index != Boost.Faction.Index) {
                        continue;
                    }
                    PsychoView[i].Pressure[Boost.PressureChannel] += Boost.Amount;
                    --DeferredBudget;
                }
            }
        });
    }

    // Flush consumed witness results
    ActionSub->FlushProcessedWitnessResults();
}


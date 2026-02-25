// Mythic Living World — Significance Rescore & Promotion Implementation

#include "Mass/Processors/SignificanceProcessor.h"
#include "MassEntitySubsystem.h"
#include "MassExecutionContext.h"
#include "MassCommandBuffer.h"
#include "Mass/Fragments/MythicMassFragments.h"
#include "Mass/Tags/MythicMassTags.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Persistence/PersistentNPCRegistry.h"
#include "World/LivingWorld/NPCGeneration/NPCGenerator.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "GameFramework/PlayerController.h"

UMythicSignificanceProcessor::UMythicSignificanceProcessor() {
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
    ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);
    bRequiresGameThreadExecution = true; // Accesses player controllers + fragment mutation
    bAutoRegisterWithProcessingPhases = true;

    // Run after pressure processing completes (so scores include latest pressure data)
    ExecutionOrder.ExecuteAfter.Add(TEXT("UMythicPressureProcessor"));

    AllSignificanceQuery.RegisterWithProcessor(*this);
}

void UMythicSignificanceProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) {
    AllSignificanceQuery.AddRequirement<FMythicIdentityFragment>(EMassFragmentAccess::ReadOnly);
    AllSignificanceQuery.AddRequirement<FMythicSignificanceFragment>(EMassFragmentAccess::ReadWrite);
    // Optional: present on Tier1+ (hydrated) entities, absent on Tier0 (ambient).
    // GetFragmentView returns an empty view when the chunk lacks this fragment.
    AllSignificanceQuery.AddRequirement<FMythicPsychodynamicFragment>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::Optional);
}

void UMythicSignificanceProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicSignificance_Execute);

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

    // Timer throttle — don't rescore every frame
    TimeSinceLastTick += Context.GetDeltaTimeSeconds();
    if (TimeSinceLastTick < Settings->SignificanceIntervalSeconds) {
        return;
    }
    TimeSinceLastTick = 0.0f;

    const float PromotionThreshold = Settings->PromotionThreshold;
    const float DemotionThreshold = Settings->DemotionThreshold;
    const float Hysteresis = Settings->SignificanceHysteresisMargin;
    const float ProxWeight = Settings->ProximityWeight;
    const float EventWeight = Settings->EventCountWeight;
    const float EmotionWeight = Settings->EmotionalIntensityWeight;
    const float SpawnRadius = Settings->PopulationSpawnRadius;
    int32 RescoreBudget = Settings->MaxRescoresPerFrame;
    int32 PromotionBudget = Settings->MaxPromotionsPerFrame;

    // Cache player cell coordinates for proximity computation
    TArray<FMythicCellCoord, TInlineAllocator<4>> PlayerCells;
    const UMythicTerritoryGrid *Grid = LWS->GetTerritoryGrid();
    if (Grid) {
        for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It) {
            const APlayerController *PC = It->Get();
            if (PC && PC->GetPawn()) {
                PlayerCells.Add(Grid->WorldToCell(PC->GetPawn()->GetActorLocation()));
            }
        }
    }

    // ─── Pass 1: Rescore dirty entities ───
    AllSignificanceQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext &ChunkContext) {
        if (RescoreBudget <= 0) {
            return;
        }

        const int32 NumEntities = ChunkContext.GetNumEntities();
        const auto IdentityView = ChunkContext.GetFragmentView<FMythicIdentityFragment>();
        auto SignificanceView = ChunkContext.GetMutableFragmentView<FMythicSignificanceFragment>();

        // Check if this chunk has psychodynamic fragments (hydrated entities)
        const bool bHasPsycho = ChunkContext.GetFragmentView<FMythicPsychodynamicFragment>().Num() > 0;
        const FMythicPsychodynamicFragment *PsychoData = nullptr;
        if (bHasPsycho) {
            PsychoData = ChunkContext.GetFragmentView<FMythicPsychodynamicFragment>().GetData();
        }

        for (int32 i = 0; i < NumEntities && RescoreBudget > 0; ++i) {
            FMythicSignificanceFragment &Sig = SignificanceView[i];
            if (!Sig.bDirty) {
                continue;
            }

            --RescoreBudget;

            // ─── Proximity Score ───
            // Inverse cell distance to nearest player [0.0, 1.0]
            float ProximityScore = 0.0f;
            const FMythicCellCoord &EntityCell = IdentityView[i].Cell;
            for (const FMythicCellCoord &PlayerCell : PlayerCells) {
                const float CellDist = static_cast<float>(
                    FMath::Abs(EntityCell.X - PlayerCell.X) + FMath::Abs(EntityCell.Y - PlayerCell.Y)
                );
                const float Prox = FMath::Max(0.0f, 1.0f - (CellDist / FMath::Max(SpawnRadius, 1.0f)));
                ProximityScore = FMath::Max(ProximityScore, Prox);
            }

            // ─── Event Count Score ───
            // Normalized by a scaling factor (16 events = 1.0)
            constexpr float EventScalingFactor = 16.0f;
            const float EventScore = FMath::Min(1.0f, static_cast<float>(Sig.RelevantEventCount) / EventScalingFactor);

            // ─── Emotional Intensity Score ───
            // Sum of all pressure channels (only for hydrated entities)
            float EmotionScore = 0.0f;
            if (bHasPsycho && PsychoData) {
                float TotalPressure = 0.0f;
                for (int32 c = 0; c < PressureChannelCount; ++c) {
                    TotalPressure += PsychoData[i].Pressure[c];
                }
                // Normalize: 3.0 total pressure = 1.0 score
                constexpr float PressureScalingFactor = 3.0f;
                EmotionScore = FMath::Min(1.0f, TotalPressure / PressureScalingFactor);
            }

            // ─── Weighted Final Score ───
            Sig.Score = FMath::Clamp(
                ProxWeight * ProximityScore + EventWeight * EventScore + EmotionWeight * EmotionScore,
                0.0f, 1.0f
                );
            Sig.bDirty = false;
        }
    });

    // ─── Pass 2: Promotion / Demotion ───
    // First, count existing cognitive actors (Tier 1+) to enforce hard cap
    int32 CognitiveActorCount = 0;
    AllSignificanceQuery.ForEachEntityChunk(Context, [&CognitiveActorCount](FMassExecutionContext &ChunkContext) {
        const int32 NumEntities = ChunkContext.GetNumEntities();
        const auto SignificanceView = ChunkContext.GetFragmentView<FMythicSignificanceFragment>();
        for (int32 i = 0; i < NumEntities; ++i) {
            if (SignificanceView[i].Tier >= EMythicSignificanceTier::Tier1_Reactive) {
                ++CognitiveActorCount;
            }
        }
    });

    const int32 MaxCognitiveActors = Settings->MaxCognitiveActors;

    AllSignificanceQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext &ChunkContext) {
        if (PromotionBudget <= 0) {
            return;
        }

        const int32 NumEntities = ChunkContext.GetNumEntities();
        auto SignificanceView = ChunkContext.GetMutableFragmentView<FMythicSignificanceFragment>();

        for (int32 i = 0; i < NumEntities && PromotionBudget > 0; ++i) {
            FMythicSignificanceFragment &Sig = SignificanceView[i];

            // ─── Tier 0 → Tier 1 Promotion ───
            if (Sig.Tier == EMythicSignificanceTier::Tier0_Ambient
                && Sig.Score >= (PromotionThreshold + Hysteresis)) {

                // Enforce hard cap on cognitive actors
                if (CognitiveActorCount >= MaxCognitiveActors) {
                    continue;
                }

                --PromotionBudget;
                ++CognitiveActorCount;

                // Promote: add hydration fragments via deferred commands
                const FMassEntityHandle Entity = ChunkContext.GetEntity(i);
                const FMythicIdentityFragment& Identity = ChunkContext.GetFragmentView<FMythicIdentityFragment>()[i];

                // Check PersistentNPCRegistry — dead NPCs cannot be promoted
                UMythicPersistentNPCRegistry* Registry = LWS->GetPersistentNPCRegistry();
                if (Registry && Registry->IsPermaDead(Identity.NameHash)) {
                    Sig.Score = 0.0f; // Suppress score
                    continue; // Skip promotion, this NPC is dead forever
                }



                Context.Defer().AddTag<FMythicHydratedTag>(Entity);
                Context.Defer().AddFragment<FMythicPsychodynamicFragment>(Entity);
                Context.Defer().AddFragment<FMythicPersonalityFragment>(Entity);
                Context.Defer().AddFragment<FMythicSocialFragment>(Entity);

                // ─── Generate personality from faction ideology ───
                // Full NPC generation pipeline: NameHash + faction ideology → personality
                UMythicFactionDatabase* FactionDB = LWS->GetFactionDatabase();
                if (FactionDB && Identity.Faction.IsValid()) {
                    FMythicFactionData FData;
                    if (FactionDB->GetFaction(Identity.Faction, FData)) {
                        // Generate personality biased by faction ideology
                        FMythicPersonalityFragment GenPersonality = FMythicNPCGenerator::GeneratePersonality(
                            Identity.NameHash, FData.Ideology, Identity.RoleTag);

                        // Apply via deferred command (fragment data is set after archetype change)
                        Context.Defer().PushCommand<FMassDeferredChangeCompositionCommand>(
                            [Entity, GenPersonality](FMassEntityManager& Manager) {
                                if (Manager.IsEntityValid(Entity)) {
                                    FMythicPersonalityFragment* Personality = Manager.GetFragmentDataPtr<FMythicPersonalityFragment>(Entity);
                                    if (Personality) {
                                        *Personality = GenPersonality;
                                    }
                                }
                            });
                    }
                }

                // ─── Leadership succession reporting ───
                // Report this entity as a potential leader candidate for its faction.
                // ReportLeaderCandidate only accepts if the score exceeds the current leader's.
                if (FactionDB && Identity.Faction.IsValid()) {
                    FactionDB->ReportLeaderCandidate(
                        Identity.Faction,
                        Entity.Index,
                        Sig.Score);
                }

                // ─── Promotion to Tier 2 (Actor Spawn) ───
                if (Sig.Score >= Settings->Tier2PromotionThreshold) {
                    Sig.Tier = EMythicSignificanceTier::Tier2_Cognitive;
                    
                    // Trigger actual actor spawn here or in PopulationSpawnerProcessor
                    Context.Defer().AddTag<FMythicActorSpawnRequestTag>(Entity);
                    
                    UE_LOG(LogMythLivingWorld, Log, TEXT("Significance: Promoted entity DIRECT to Tier2_Cognitive (score=%.2f, NameHash=%u)"),
                           Sig.Score, Identity.NameHash);
                } else {
                    Sig.Tier = EMythicSignificanceTier::Tier1_Reactive;

                    UE_LOG(LogMythLivingWorld, Log, TEXT("Significance: Promoted entity to Tier1_Reactive (score=%.2f, cognitive=%d/%d)"),
                           Sig.Score, CognitiveActorCount, MaxCognitiveActors);
                }
            }
            // ─── Tier 1 → Tier 2 Promotion ───
            else if (Sig.Tier == EMythicSignificanceTier::Tier1_Reactive
                     && Sig.Score >= Settings->Tier2PromotionThreshold) {
                     
                const FMythicIdentityFragment& Identity = ChunkContext.GetFragmentView<FMythicIdentityFragment>()[i];
                 
                // Check PersistentNPCRegistry — dead NPCs cannot be promoted
                UMythicPersistentNPCRegistry* Registry = LWS->GetPersistentNPCRegistry();
                if (Registry && Registry->IsPermaDead(Identity.NameHash)) {
                    Sig.Score = 0.0f; // Suppress score
                    continue; // Skip promotion, this NPC is dead forever
                }

                Sig.Tier = EMythicSignificanceTier::Tier2_Cognitive;
                
                const FMassEntityHandle Entity = ChunkContext.GetEntity(i);
                Context.Defer().AddTag<FMythicActorSpawnRequestTag>(Entity);
                
                UE_LOG(LogMythLivingWorld, Log, TEXT("Significance: Promoted entity to Tier2_Cognitive (score=%.2f, NameHash=%u)"),
                       Sig.Score, Identity.NameHash);
            }
            // ─── Tier 1 → Tier 0 Demotion ───
            else if (Sig.Tier == EMythicSignificanceTier::Tier1_Reactive
                && Sig.Score <= (DemotionThreshold - Hysteresis)) {
                --PromotionBudget;
                --CognitiveActorCount;

                // Demote: remove hydration fragments
                const FMassEntityHandle Entity = ChunkContext.GetEntity(i);
                Context.Defer().RemoveTag<FMythicHydratedTag>(Entity);
                Context.Defer().RemoveFragment<FMythicPsychodynamicFragment>(Entity);
                Context.Defer().RemoveFragment<FMythicPersonalityFragment>(Entity);
                Context.Defer().RemoveFragment<FMythicSocialFragment>(Entity);

                Sig.Tier = EMythicSignificanceTier::Tier0_Ambient;
                Sig.RelevantEventCount = 0;

                UE_LOG(LogMythLivingWorld, Log, TEXT("Significance: Demoted entity to Tier0_Ambient (score=%.2f, cognitive=%d/%d)"),
                       Sig.Score, CognitiveActorCount, MaxCognitiveActors);
            }
            // Tier 2+ promotion/demotion deferred to Phase 5 (cognitive actors)
        }
    });
}

float UMythicSignificanceProcessor::ComputeProximityScore(const FMythicCellCoord &EntityCell, float SpawnRadius) const {
    // This helper is available if needed by subclasses; main logic uses inline calculation.
    return 0.0f;
}

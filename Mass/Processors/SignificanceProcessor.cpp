
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
#include "AI/Party/PartySubsystem.h"
#include "AI/NPCs/MythicNPCCharacter.h"
#include "GameFramework/Pawn.h"
#include "World/LivingWorld/NPCGeneration/NPCGenerator.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"

namespace {
    bool IsEmbodiedActorInCloseView(
        const UMythicLivingWorldSubsystem *LWS,
        FMassEntityHandle Entity,
        bool bDespawnGateActive,
        TConstArrayView<FMythicPlayerView> PlayerViews,
        float MinSpawnDistance) {
        if (!bDespawnGateActive || !LWS) {
            return false;
        }
        const AMythicNPCCharacter *Actor = LWS->FindEmbodiedActor(Entity);
        if (!Actor) {
            return false;
        }
        return UMythicSignificanceProcessor::IsInCloseView(Actor->GetActorLocation(), PlayerViews, MinSpawnDistance);
    }
}

UMythicSignificanceProcessor::UMythicSignificanceProcessor() {
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
    ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);
    bRequiresGameThreadExecution = true;
    bAutoRegisterWithProcessingPhases = true;

    ExecutionOrder.ExecuteAfter.Add(TEXT("UMythicPressureProcessor"));

    AllSignificanceQuery.RegisterWithProcessor(*this);
}

void UMythicSignificanceProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) {
    AllSignificanceQuery.AddRequirement<FMythicIdentityFragment>(EMassFragmentAccess::ReadOnly);
    AllSignificanceQuery.AddRequirement<FMythicSignificanceFragment>(EMassFragmentAccess::ReadWrite);
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

    const UMythicPartySubsystem *PartySubsystem = World->GetSubsystem<UMythicPartySubsystem>();

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
    const float EventCountFullScore = Settings->EventCountFullScore;
    const float EmotionalPressureFullScore = Settings->EmotionalPressureFullScore;
    const bool bProxEmbodiment = Settings->bProximityForcesEmbodiment;
    const int32 EmbodimentRadiusCells = Settings->EmbodimentRadiusCells;
    int32 RescoreBudget = Settings->MaxRescoresPerFrame;
    int32 PromotionBudget = Settings->MaxPromotionsPerFrame;
    int32 DemotionBudget = Settings->MaxPromotionsPerFrame;

    const bool bViewGate = Settings->bViewGateEmbodiment;
    const float ViewGateMinDist = Settings->ViewGateMinSpawnDistance;
    const float ViewConeMarginRad = FMath::DegreesToRadians(FMath::Max(0.0f, Settings->ViewConeMarginDeg));
    const float StreamInGrace = Settings->StreamInGraceSeconds;
    const int32 RegionEnterJump = FMath::Max(1, Settings->RegionEnterCellJump);
    const double NowSeconds = World->GetTimeSeconds();

    TArray<FMythicCellCoord, TInlineAllocator<4>> PlayerCells;
    TArray<FMythicPlayerView, TInlineAllocator<4>> PlayerViews;
    TSet<TWeakObjectPtr<const APlayerController>> LivePlayers;
    bool bGraceActive = false;

    const UMythicTerritoryGrid *Grid = LWS->GetTerritoryGrid();
    if (Grid) {
        for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It) {
            const APlayerController *PC = It->Get();
            if (!PC || !PC->GetPawn()) {
                continue;
            }

            const FVector PawnLoc = PC->GetPawn()->GetActorLocation();
            const FMythicCellCoord PlayerCell = Grid->WorldToCell(PawnLoc);
            PlayerCells.Add(PlayerCell);

            if (bViewGate && PC->PlayerCameraManager) {
                FMythicPlayerView View;
                View.CamLocation = PC->PlayerCameraManager->GetCameraLocation();
                View.CamForward = PC->PlayerCameraManager->GetCameraRotation().Vector();
                const float HalfFOVRad = FMath::DegreesToRadians(0.5f * PC->PlayerCameraManager->GetFOVAngle());
                const float HalfConeRad = FMath::Min(PI, HalfFOVRad + ViewConeMarginRad);
                View.CosHalfCone = FMath::Cos(HalfConeRad);
                PlayerViews.Add(View);
            }

            if (bViewGate) {
                const TWeakObjectPtr<const APlayerController> Key(PC);
                LivePlayers.Add(Key);

                bool bArmGrace = false;
                if (const FMythicCellCoord *LastCell = PlayerLastCell.Find(Key)) {
                    const int32 CellJump = FMath::Abs(PlayerCell.X - LastCell->X) + FMath::Abs(PlayerCell.Y - LastCell->Y);
                    if (CellJump >= RegionEnterJump) {
                        bArmGrace = true;
                    }
                }
                else {
                    bArmGrace = true;
                }
                PlayerLastCell.Add(Key, PlayerCell);

                if (bArmGrace) {
                    PlayerFirstSeenTime.Add(Key, NowSeconds);
                }

                if (const double *FirstSeen = PlayerFirstSeenTime.Find(Key)) {
                    if (NowSeconds - *FirstSeen < static_cast<double>(StreamInGrace)) {
                        bGraceActive = true;
                    }
                }
            }
        }
    }

    if (bViewGate && (PlayerFirstSeenTime.Num() > 0 || PlayerLastCell.Num() > 0)) {
        for (auto MapIt = PlayerFirstSeenTime.CreateIterator(); MapIt; ++MapIt) {
            if (!LivePlayers.Contains(MapIt.Key())) {
                MapIt.RemoveCurrent();
            }
        }
        for (auto MapIt = PlayerLastCell.CreateIterator(); MapIt; ++MapIt) {
            if (!LivePlayers.Contains(MapIt.Key())) {
                MapIt.RemoveCurrent();
            }
        }
    }

    const bool bSpawnGateActive = bViewGate && !bGraceActive && PlayerViews.Num() > 0;
    const bool bDespawnGateActive = bViewGate && PlayerViews.Num() > 0;

    AllSignificanceQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext &ChunkContext) {
        if (RescoreBudget <= 0) {
            return;
        }

        const int32 NumEntities = ChunkContext.GetNumEntities();
        const auto IdentityView = ChunkContext.GetFragmentView<FMythicIdentityFragment>();
        auto SignificanceView = ChunkContext.GetMutableFragmentView<FMythicSignificanceFragment>();

        const bool bHasPsycho = ChunkContext.GetFragmentView<FMythicPsychodynamicFragment>().Num() > 0;
        const FMythicPsychodynamicFragment *PsychoData = nullptr;
        if (bHasPsycho) {
            PsychoData = ChunkContext.GetFragmentView<FMythicPsychodynamicFragment>().GetData();
        }

        for (int32 i = 0; i < NumEntities && RescoreBudget > 0; ++i) {
            FMythicSignificanceFragment &Sig = SignificanceView[i];

            int32 NearestCellDist = MAX_int32;
            for (const FMythicCellCoord &PCell : PlayerCells) {
                NearestCellDist = FMath::Min(NearestCellDist,
                    FMath::Abs(IdentityView[i].Cell.X - PCell.X) + FMath::Abs(IdentityView[i].Cell.Y - PCell.Y));
            }

            const bool bNearForEmbody = bProxEmbodiment && NearestCellDist <= EmbodimentRadiusCells + 1;
            if (!ShouldRescore(Sig.bDirty, Sig.Tier) && !bNearForEmbody) {
                continue;
            }

            --RescoreBudget;

            const float ProximityScore = ComputeProximityScore(IdentityView[i].Cell, PlayerCells, SpawnRadius);

            const float EventScore = FMath::Min(1.0f, static_cast<float>(Sig.RelevantEventCount) / EventCountFullScore);

            float EmotionScore = 0.0f;
            if (bHasPsycho && PsychoData) {
                float TotalPressure = 0.0f;
                for (int32 c = 0; c < PressureChannelCount; ++c) {
                    TotalPressure += PsychoData[i].Pressure[c];
                }
                EmotionScore = FMath::Min(1.0f, TotalPressure / EmotionalPressureFullScore);
            }

            const float WeightedScore = FMath::Clamp(
                ProxWeight * ProximityScore + EventWeight * EventScore + EmotionWeight * EmotionScore,
                0.0f, 1.0f);

            float EmbodimentScore = 0.0f;
            if (bNearForEmbody) {
                const int32 Cutoff = EmbodimentRadiusCells + (Sig.Tier == EMythicSignificanceTier::Tier2_Cognitive ? 1 : 0);
                if (NearestCellDist <= Cutoff) {
                    EmbodimentScore = 1.0f;
                }
            }

            Sig.Score = FMath::Max(WeightedScore, EmbodimentScore);
            Sig.bDirty = false;
        }
    });

    int32 CognitiveActorCount = 0;
    int32 EmbodiedActorCount = 0;
    int32 CreatureActiveCount = 0;
    AllSignificanceQuery.ForEachEntityChunk(Context, [&CognitiveActorCount, &EmbodiedActorCount, &CreatureActiveCount](FMassExecutionContext &ChunkContext) {
        const bool bCreatureChunk = ChunkContext.DoesArchetypeHaveTag<FMythicCreatureTag>();
        const int32 NumEntities = ChunkContext.GetNumEntities();
        const auto SignificanceView = ChunkContext.GetFragmentView<FMythicSignificanceFragment>();
        for (int32 i = 0; i < NumEntities; ++i) {
            if (SignificanceView[i].Tier >= EMythicSignificanceTier::Tier1_Reactive) {
                if (bCreatureChunk) { ++CreatureActiveCount; } else { ++CognitiveActorCount; }
            }
            if (!bCreatureChunk && SignificanceView[i].Tier >= EMythicSignificanceTier::Tier2_Cognitive) {
                ++EmbodiedActorCount;
            }
        }
    });

    const int32 MaxCognitiveActors = Settings->MaxCognitiveActors;
    const int32 MaxEmbodiedActors = Settings->MaxEmbodiedActors;
    const int32 MaxCreatureActors = Settings->MaxCreatureActors;

    AllSignificanceQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext &ChunkContext) {
        if (PromotionBudget <= 0 && DemotionBudget <= 0) {
            return;
        }

        const int32 NumEntities = ChunkContext.GetNumEntities();
        auto SignificanceView = ChunkContext.GetMutableFragmentView<FMythicSignificanceFragment>();

        const bool bChunkIsCreature = ChunkContext.DoesArchetypeHaveTag<FMythicCreatureTag>();

        auto WouldPopInView = [&](const FMythicIdentityFragment &Id) -> bool {
            if (!bSpawnGateActive || !Grid) {
                return false;
            }
            const FVector CandidatePos = Grid->CellToWorld(Id.Cell);
            return IsInCloseView(CandidatePos, PlayerViews, ViewGateMinDist);
        };

        for (int32 i = 0; i < NumEntities && (PromotionBudget > 0 || DemotionBudget > 0); ++i) {
            FMythicSignificanceFragment &Sig = SignificanceView[i];

            if (Sig.Tier >= EMythicSignificanceTier::Tier1_Reactive) {
                const FMythicIdentityFragment &DeadId = ChunkContext.GetFragmentView<FMythicIdentityFragment>()[i];
                UMythicPersistentNPCRegistry *DeadReg = LWS->GetPersistentNPCRegistry();
                if (DeadReg && DeadReg->IsPermaDead(DeadId.EntityId)) {
                    const FMassEntityHandle Entity = ChunkContext.GetEntity(i);
                    if (Sig.Tier == EMythicSignificanceTier::Tier2_Cognitive) {
                        Context.Defer().AddTag<FMythicActorDespawnRequestTag>(Entity);
                        if (!bChunkIsCreature) {
                            --EmbodiedActorCount;
                        }
                    }
                    Context.Defer().RemoveTag<FMythicHydratedTag>(Entity);
                    if (!bChunkIsCreature) {
                        Context.Defer().RemoveFragment<FMythicPsychodynamicFragment>(Entity);
                        Context.Defer().RemoveFragment<FMythicPersonalityFragment>(Entity);
                        Context.Defer().RemoveFragment<FMythicSocialFragment>(Entity);
                    }
                    Sig.Tier = EMythicSignificanceTier::Tier0_Ambient;
                    Sig.Score = 0.0f;
                    Sig.RelevantEventCount = 0;
                    if (bChunkIsCreature) { --CreatureActiveCount; } else { --CognitiveActorCount; }
                    continue;
                }
            }

            if (PromotionBudget > 0
                && Sig.Tier == EMythicSignificanceTier::Tier0_Ambient
                && QualifiesForPromotion(Sig.Score, PromotionThreshold, Hysteresis)) {
                if (bChunkIsCreature ? (CreatureActiveCount >= MaxCreatureActors)
                                     : (CognitiveActorCount >= MaxCognitiveActors)) {
                    continue;
                }

                const FMassEntityHandle Entity = ChunkContext.GetEntity(i);
                const FMythicIdentityFragment &Identity = ChunkContext.GetFragmentView<FMythicIdentityFragment>()[i];

                UMythicPersistentNPCRegistry *Registry = LWS->GetPersistentNPCRegistry();
                if (Registry && Registry->IsPermaDead(Identity.EntityId)) {
                    Sig.Score = 0.0f;
                    continue;
                }

                --PromotionBudget;
                if (bChunkIsCreature) { ++CreatureActiveCount; } else { ++CognitiveActorCount; }


                Context.Defer().AddTag<FMythicHydratedTag>(Entity);

                if (!bChunkIsCreature) {
                    Context.Defer().AddFragment<FMythicPsychodynamicFragment>(Entity);
                    Context.Defer().AddFragment<FMythicPersonalityFragment>(Entity);
                    Context.Defer().AddFragment<FMythicSocialFragment>(Entity);

                    UMythicFactionDatabase *FactionDB = LWS->GetFactionDatabase();
                    if (FactionDB && Identity.Faction.IsValid()) {
                        FMythicFactionData FData;
                        if (FactionDB->GetFaction(Identity.Faction, FData)) {
                            FMythicPersonalityFragment GenPersonality = FMythicNPCGenerator::GeneratePersonality(
                                Identity.NameSeed, FData.Ideology, Identity.RoleTag);

                            Context.Defer().PushCommand<FMassDeferredChangeCompositionCommand>(
                                [Entity, GenPersonality](FMassEntityManager &Manager) {
                                    if (Manager.IsEntityValid(Entity)) {
                                        FMythicPersonalityFragment *Personality = Manager.GetFragmentDataPtr<FMythicPersonalityFragment>(Entity);
                                        if (Personality) {
                                            *Personality = GenPersonality;
                                        }
                                    }
                                });
                        }
                    }

                    if (FactionDB && Identity.Faction.IsValid()) {
                        LWS->ReportLeaderCandidate(
                            Identity.Faction,
                            Identity.EntityId,
                            Sig.Score);
                    }
                }

                if (QualifiesForPromotion(Sig.Score, Settings->Tier2PromotionThreshold, 0.0f)
                    && (bChunkIsCreature || EmbodiedActorCount < MaxEmbodiedActors)
                    && !WouldPopInView(Identity)) {
                    Sig.Tier = EMythicSignificanceTier::Tier2_Cognitive;
                    if (!bChunkIsCreature) { ++EmbodiedActorCount; }

                    Context.Defer().AddTag<FMythicActorSpawnRequestTag>(Entity);

                    UE_LOG(LogMythLivingWorld, Log, TEXT("Significance: Promoted entity DIRECT to Tier2_Cognitive (score=%.2f, NameHash=%u, embodied=%d/%d)"),
                           Sig.Score, Identity.NameSeed, EmbodiedActorCount, MaxEmbodiedActors);
                }
                else {
                    Sig.Tier = EMythicSignificanceTier::Tier1_Reactive;

                    UE_LOG(LogMythLivingWorld, Log, TEXT("Significance: Promoted entity to Tier1_Reactive (score=%.2f, cognitive=%d/%d)"),
                           Sig.Score, CognitiveActorCount, MaxCognitiveActors);
                }
            }
            else if (PromotionBudget > 0
                && Sig.Tier == EMythicSignificanceTier::Tier1_Reactive
                && QualifiesForPromotion(Sig.Score, Settings->Tier2PromotionThreshold, 0.0f)
                && (bChunkIsCreature || EmbodiedActorCount < MaxEmbodiedActors)
                && !WouldPopInView(ChunkContext.GetFragmentView<FMythicIdentityFragment>()[i])) {
                const FMythicIdentityFragment &Identity = ChunkContext.GetFragmentView<FMythicIdentityFragment>()[i];

                UMythicPersistentNPCRegistry *Registry = LWS->GetPersistentNPCRegistry();
                if (Registry && Registry->IsPermaDead(Identity.EntityId)) {
                    Sig.Score = 0.0f;
                    continue;
                }

                --PromotionBudget;
                Sig.Tier = EMythicSignificanceTier::Tier2_Cognitive;
                if (!bChunkIsCreature) { ++EmbodiedActorCount; }

                const FMassEntityHandle Entity = ChunkContext.GetEntity(i);
                Context.Defer().AddTag<FMythicActorSpawnRequestTag>(Entity);

                UE_LOG(LogMythLivingWorld, Log, TEXT("Significance: Promoted entity to Tier2_Cognitive (score=%.2f, NameHash=%u, embodied=%d/%d)"),
                       Sig.Score, Identity.NameSeed, EmbodiedActorCount, MaxEmbodiedActors);
            }
            else if (DemotionBudget > 0
                && Sig.Tier == EMythicSignificanceTier::Tier1_Reactive
                && QualifiesForDemotion(Sig.Score, DemotionThreshold, Hysteresis)) {
                --DemotionBudget;
                if (bChunkIsCreature) { --CreatureActiveCount; } else { --CognitiveActorCount; }

                const FMassEntityHandle Entity = ChunkContext.GetEntity(i);
                Context.Defer().RemoveTag<FMythicHydratedTag>(Entity);
                if (!bChunkIsCreature) {
                    Context.Defer().RemoveFragment<FMythicPsychodynamicFragment>(Entity);
                    Context.Defer().RemoveFragment<FMythicPersonalityFragment>(Entity);
                    Context.Defer().RemoveFragment<FMythicSocialFragment>(Entity);
                }

                Sig.Tier = EMythicSignificanceTier::Tier0_Ambient;
                Sig.RelevantEventCount = 0;

                UE_LOG(LogMythLivingWorld, Log, TEXT("Significance: Demoted entity to Tier0_Ambient (score=%.2f, cognitive=%d/%d)"),
                       Sig.Score, CognitiveActorCount, MaxCognitiveActors);
            }
            else if (DemotionBudget > 0
                && Sig.Tier == EMythicSignificanceTier::Tier2_Cognitive
                && QualifiesForDemotion(Sig.Score, DemotionThreshold, Hysteresis)
                && !(PartySubsystem && PartySubsystem->IsCompanionEntity(ChunkContext.GetEntity(i)))
                && !IsEmbodiedActorInCloseView(LWS, ChunkContext.GetEntity(i), bDespawnGateActive, PlayerViews, ViewGateMinDist)) {
                --DemotionBudget;
                if (!bChunkIsCreature) { --EmbodiedActorCount; }
                const FMassEntityHandle Entity = ChunkContext.GetEntity(i);
                Context.Defer().AddTag<FMythicActorDespawnRequestTag>(Entity);
                Sig.Tier = EMythicSignificanceTier::Tier1_Reactive;

                UE_LOG(LogMythLivingWorld, Log, TEXT("Significance: Demoted entity Tier2->Tier1, despawn requested (score=%.2f, cognitive=%d/%d)"),
                       Sig.Score, CognitiveActorCount, MaxCognitiveActors);
            }
        }
    });
}

float UMythicSignificanceProcessor::ComputeProximityScore(const FMythicCellCoord &EntityCell, TConstArrayView<FMythicCellCoord> PlayerCells, float SpawnRadius) {
    float ProximityScore = 0.0f;
    const float SafeRadius = FMath::Max(SpawnRadius, 1.0f);
    for (const FMythicCellCoord &PlayerCell : PlayerCells) {
        const float CellDist = static_cast<float>(
            FMath::Abs(EntityCell.X - PlayerCell.X) + FMath::Abs(EntityCell.Y - PlayerCell.Y));
        ProximityScore = FMath::Max(ProximityScore, FMath::Max(0.0f, 1.0f - (CellDist / SafeRadius)));
    }
    return ProximityScore;
}

bool UMythicSignificanceProcessor::IsInCloseView(const FVector &WorldPos, TConstArrayView<FMythicPlayerView> PlayerViews, float MinSpawnDistance) {
    const float MinDistSq = MinSpawnDistance * MinSpawnDistance;

    for (const FMythicPlayerView &View : PlayerViews) {
        const FVector ToPos = WorldPos - View.CamLocation;
        const float DistSq = ToPos.SizeSquared();

        if (DistSq > MinDistSq) {
            continue;
        }

        const FVector DirToPos = ToPos.GetSafeNormal();
        if (DirToPos.IsNearlyZero()) {
            return true;
        }
        const float CosAngle = FVector::DotProduct(View.CamForward, DirToPos);
        if (CosAngle >= View.CosHalfCone) {
            return true;
        }
    }

    return false;
}

bool UMythicSignificanceProcessor::QualifiesForPromotion(float Score, float Threshold, float Hysteresis) {
    return Score >= Threshold + Hysteresis;
}

bool UMythicSignificanceProcessor::QualifiesForDemotion(float Score, float Threshold, float Hysteresis) {
    return Score <= Threshold - Hysteresis;
}

bool UMythicSignificanceProcessor::ShouldRescore(bool bDirty, EMythicSignificanceTier Tier) {
    return bDirty || Tier != EMythicSignificanceTier::Tier0_Ambient;
}

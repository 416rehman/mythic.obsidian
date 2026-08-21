
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
    bRequiresGameThreadExecution = true;
    bAutoRegisterWithProcessingPhases = true;

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

bool UMythicPressureProcessor::ComputeDespairState(float TotalPressure, float DespairThreshold, bool bWasDespaired) {
    constexpr float RecoveryFraction = 0.75f;
    if (!bWasDespaired) {
        return TotalPressure >= DespairThreshold;
    }
    return TotalPressure >= DespairThreshold * RecoveryFraction;
}

void UMythicPressureProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicPressure_Execute);

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

    TArray<FMythicWitnessResult> &WitnessResults = ActionSub->GetPendingWitnessResults();
    if (WitnessResults.Num() == 0) {
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

    const int32 EnforceVent = static_cast<int32>(EMythicVentChannel::Enforce);
    const int32 FightVent = static_cast<int32>(EMythicVentChannel::Fight);
    const int32 FleeVent = static_cast<int32>(EMythicVentChannel::Flee);

    TMap<FMassEntityHandle, TArray<const FMythicWitnessResult *>> EntityWitnessMap;
    EntityWitnessMap.Reserve(WitnessResults.Num());

    const int32 MaxToProcess = FMath::Min(WitnessResults.Num(), PressureBudget);
    for (int32 i = 0; i < MaxToProcess; ++i) {
        EntityWitnessMap.FindOrAdd(WitnessResults[i].WitnessEntity).Add(&WitnessResults[i]);
    }

    struct FDeferredPressureBoost {
        FMythicCellCoord Cell;
        FMythicFactionId Faction;
        int32 PressureChannel;
        float Amount;
        int32 Radius;
    };
    TArray<FDeferredPressureBoost> DeferredBoosts;
    DeferredBoosts.Reserve(16);

    TMap<int32, int32> FightTargetCounts;

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
                continue;
            }

            const FMythicIdentityFragment &Identity = IdentityView[i];
            FMythicPsychodynamicFragment &Psycho = PsychoView[i];
            const FMythicPersonalityFragment &Personality = PersonalityView[i];
            FMythicSignificanceFragment &Significance = SignificanceView[i];

            const double Elapsed = CurrentWorldTime - Psycho.LastEventTime;
            if (Elapsed > 0.0 && Psycho.LastEventTime > 0.0) {
                const float DecayMultiplier = FMath::Exp(-DecayRate * static_cast<float>(Elapsed));
                for (int32 c = 0; c < PressureChannelCount; ++c) {
                    Psycho.Pressure[c] *= DecayMultiplier;
                }
            }

            for (const FMythicWitnessResult *Result : *Results) {
                --PressureBudget;

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
                    continue;
                }

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
                if (bIsMagic) {
                    Psycho.Pressure[ThreatIdx] += SeverityMagnitude * 0.6f;
                }
                if (bIsEnvironment) {
                    Psycho.Pressure[ThreatIdx] += SeverityMagnitude * 0.7f;
                }
                if (!bIsCombat && !bIsCrime && !bIsDeath && !bIsMagic && !bIsEnvironment) {
                    Psycho.Pressure[ThreatIdx] += SeverityMagnitude * 0.5f;
                }
            }

            Psycho.LastEventTime = CurrentWorldTime;

            float TotalPressure = 0.0f;
            for (int32 c = 0; c < PressureChannelCount; ++c) {
                TotalPressure += Psycho.Pressure[c];
            }
            const bool bNowDespaired = ComputeDespairState(TotalPressure, DespairThreshold, Psycho.bDespaired);
            if (bNowDespaired != Psycho.bDespaired) {
                Psycho.bDespaired = bNowDespaired;
                UE_LOG(LogMythLivingWorld, Log, TEXT("Despair: Entity in cell %s %s (total=%.2f, threshold=%.2f)"),
                       *Identity.Cell.ToString(),
                       bNowDespaired ? TEXT("reached despair") : TEXT("recovered from despair"),
                       TotalPressure, DespairThreshold);
            }

            float MaxPressure = 0.0f;
            int32 MaxPressureChannel = -1;
            for (int32 c = 0; c < PressureChannelCount; ++c) {
                if (Psycho.Pressure[c] > MaxPressure) {
                    MaxPressure = Psycho.Pressure[c];
                    MaxPressureChannel = c;
                }
            }

            if (MaxPressure >= VentThreshold && MaxPressureChannel >= 0) {
                float BestVentWeight = -1.0f;
                int32 BestVentChannel = 0;
                for (int32 v = 0; v < VentChannelCount; ++v) {
                    if (Personality.VentWeights[v] > BestVentWeight) {
                        BestVentWeight = Personality.VentWeights[v];
                        BestVentChannel = v;
                    }
                }

                if (BestVentChannel == EnforceVent) {
                    FDeferredPressureBoost Boost;
                    Boost.Cell = Identity.Cell;
                    Boost.Faction = Identity.Faction;
                    Boost.PressureChannel = InjusticeIdx;
                    Boost.Amount = MaxPressure * 0.3f;
                    Boost.Radius = FMath::RoundToInt(GuardAssistRadius);
                    DeferredBoosts.Add(Boost);
                }

                if (BestVentChannel == FleeVent) {
                    FDeferredPressureBoost Boost;
                    Boost.Cell = Identity.Cell;
                    Boost.Faction = FMythicFactionId();
                    Boost.PressureChannel = ThreatIdx;
                    Boost.Amount = MaxPressure * 0.2f;
                    Boost.Radius = FMath::RoundToInt(EmotionalContagionRadius);
                    DeferredBoosts.Add(Boost);
                }

                if (BestVentChannel == FightVent && Psycho.FightTargetEntity != INDEX_NONE) {
                    FightTargetCounts.FindOrAdd(Psycho.FightTargetEntity)++;
                }

                Psycho.Pressure[MaxPressureChannel] *= 0.5f;

                UE_LOG(LogMythLivingWorld, Verbose, TEXT("Pressure vent: Entity vented via channel %d (pressure=%.2f, threshold=%.2f)"),
                       BestVentChannel, MaxPressure, VentThreshold);
            }

            Significance.bDirty = true;
        }
    });

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
                    Psycho.Pressure[WrathIdx] += 0.3f;
                    Psycho.Pressure[ThreatIdx] *= 0.7f;
                }
            }
        });
    }

    if (DeferredBoosts.Num() > 0) {
        int32 DeferredBudget = 16;
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
                    const int32 Dist = FMath::Abs(Identity.Cell.X - Boost.Cell.X)
                        + FMath::Abs(Identity.Cell.Y - Boost.Cell.Y);
                    if (Dist > Boost.Radius) {
                        continue;
                    }
                    if (Boost.Faction.IsValid() && Identity.Faction.Index != Boost.Faction.Index) {
                        continue;
                    }
                    PsychoView[i].Pressure[Boost.PressureChannel] += Boost.Amount;
                    --DeferredBudget;
                }
            }
        });
    }

    ActionSub->FlushProcessedWitnessResults();
}

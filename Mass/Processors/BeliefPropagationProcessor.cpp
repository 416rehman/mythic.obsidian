
#include "Mass/Processors/BeliefPropagationProcessor.h"
#include "MassEntitySubsystem.h"
#include "MassExecutionContext.h"
#include "Mass/Fragments/MythicMassFragments.h"
#include "Mass/Tags/MythicMassTags.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/LivingWorld/Social/SocialGraph.h"
#include "Engine/World.h"

UMythicBeliefPropagationProcessor::UMythicBeliefPropagationProcessor() {
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
    ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);
    bRequiresGameThreadExecution = true;
    bAutoRegisterWithProcessingPhases = true;

    ExecutionOrder.ExecuteAfter.Add(TEXT("UMythicPressureProcessor"));

    HydratedSocialQuery.RegisterWithProcessor(*this);
}

void UMythicBeliefPropagationProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) {
    HydratedSocialQuery.AddRequirement<FMythicIdentityFragment>(EMassFragmentAccess::ReadOnly);
    HydratedSocialQuery.AddRequirement<FMythicSignificanceFragment>(EMassFragmentAccess::ReadWrite);
    HydratedSocialQuery.AddRequirement<FMythicSocialFragment>(EMassFragmentAccess::ReadWrite);
    HydratedSocialQuery.AddRequirement<FMythicPsychodynamicFragment>(EMassFragmentAccess::ReadOnly);
    HydratedSocialQuery.AddTagRequirement<FMythicHydratedTag>(EMassFragmentPresence::All);
}

void UMythicBeliefPropagationProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicBeliefPropagation_Execute);

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

    UMythicSocialGraph *SocialGraph = LWS->GetSocialGraph();
    if (!SocialGraph) {
        return;
    }

    TimeSinceLastTick += Context.GetDeltaTimeSeconds();
    if (TimeSinceLastTick < 2.0f) {
        return;
    }
    TimeSinceLastTick = 0.0f;

    int32 PropagationBudget = Settings->MaxBeliefPropagationsPerTick;
    const float DecayPerHop = Settings->BeliefPropagationDecay;


    HydratedSocialQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext &ChunkContext) {
        if (PropagationBudget <= 0) {
            return;
        }

        const int32 NumEntities = ChunkContext.GetNumEntities();
        const auto IdentityView = ChunkContext.GetFragmentView<FMythicIdentityFragment>();
        auto SignificanceView = ChunkContext.GetMutableFragmentView<FMythicSignificanceFragment>();
        const auto PsychoView = ChunkContext.GetFragmentView<FMythicPsychodynamicFragment>();
        const auto SocialView = ChunkContext.GetFragmentView<FMythicSocialFragment>();

        for (int32 i = 0; i < NumEntities && PropagationBudget > 0; ++i) {
            const FMythicSignificanceFragment &Sig = SignificanceView[i];
            const FMythicSocialFragment &Social = SocialView[i];

            if (Sig.RelevantEventCount == 0 || Social.EdgeCount == 0) {
                continue;
            }

            float TotalPressure = 0.0f;
            for (int32 c = 0; c < PressureChannelCount; ++c) {
                TotalPressure += PsychoView[i].Pressure[c];
            }

            if (TotalPressure < 0.5f) {
                continue;
            }

            TArray<FMythicSocialEdge> SocialEdges;
            SocialGraph->GetEdges(ChunkContext.GetEntity(i), World->GetTimeSeconds(), SocialEdges);

            for (const FMythicSocialEdge &Edge : SocialEdges) {
                if (PropagationBudget <= 0) {
                    break;
                }

                if (Edge.Relation == EMythicSocialRelation::Rival) {
                    continue;
                }

                --PropagationBudget;
                const FMassEntityHandle NeighborEntity = Edge.TargetEntity;

                const uint16 PropagatedEventCount = FMath::Max<uint16>(1,
                                                                       static_cast<uint16>(static_cast<float>(Sig.RelevantEventCount) * (1.0f - DecayPerHop)));

                Context.Defer().PushCommand<FMassDeferredChangeCompositionCommand>(
                    [NeighborEntity, PropagatedEventCount](FMassEntityManager &Manager) {
                        if (!Manager.IsEntityValid(NeighborEntity)) {
                            return;
                        }

                        FMythicSignificanceFragment *NeighborSig =
                            Manager.GetFragmentDataPtr<FMythicSignificanceFragment>(NeighborEntity);

                        if (NeighborSig) {
                            NeighborSig->bDirty = true;

                            const uint32 NewCount = static_cast<uint32>(NeighborSig->RelevantEventCount) + PropagatedEventCount;
                            NeighborSig->RelevantEventCount = static_cast<uint16>(FMath::Min<uint32>(NewCount, 0xFFFFu));
                        }
                    });

                UE_LOG(LogMythLivingWorld, Verbose,
                       TEXT("BeliefPropagation: Entity propagated %d events to neighbor (relation=%d, pressure=%.1f)"),
                       PropagatedEventCount, static_cast<int32>(Edge.Relation), TotalPressure);
            }
        }
    });
}

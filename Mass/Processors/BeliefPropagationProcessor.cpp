// Mythic Living World — Belief Propagation Processor Implementation
// Spreads beliefs through social graph edges. Budget-capped, hop-decaying.

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

    // Run after pressure processor (beliefs form from pressure evaluation)
    ExecutionOrder.ExecuteAfter.Add(TEXT("UMythicPressureProcessor"));

    HydratedSocialQuery.RegisterWithProcessor(*this);
}

void UMythicBeliefPropagationProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) {
    // Only hydrated entities (Tier 1+) with social edges participate in propagation
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

    // Timer throttle — belief propagation runs at 2s intervals (not per-frame)
    TimeSinceLastTick += Context.GetDeltaTimeSeconds();
    if (TimeSinceLastTick < 2.0f) {
        return;
    }
    TimeSinceLastTick = 0.0f;

    int32 PropagationBudget = Settings->MaxBeliefPropagationsPerTick;
    const float DecayPerHop = Settings->BeliefPropagationDecay;
    // NOTE: MaxBeliefPropagationHops is intentionally NOT applied here. This processor spreads significance-AWARENESS
    // one hop to direct social neighbors and is naturally bounded by the pressure gate below (a propagated-to entity
    // gains awareness but no pressure, so it does not re-propagate). The per-belief hop cap lives in the cognitive
    // belief path: UMythicPartySubsystem::ShouldShareBelief.

    // ─── Belief Propagation Pass ───
    // For each hydrated entity with social edges, check if it has high pressure
    // (indicating it witnessed something significant). If so, propagate the
    // "significance dirty" flag to its neighbors so they become aware.
    //
    // This implements the NPC-to-NPC information pipeline:
    //   1. Entity A witnesses event → pressure accumulated → significance dirty
    //   2. Entity A has social edge to Entity B (friend/ally)
    //   3. This processor propagates the dirty flag to Entity B
    //   4. Entity B's significance score increases → may promote to higher tier
    //   5. If Entity B is a guard (high Enforce weight), it investigates
    //
    // Budget-capped: only MaxBeliefPropagationsPerTick propagations per tick.
    // Hop decay: each propagation reduces the event count contribution.
    // This prevents instant omniscience — information takes time to spread.

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

            // Only propagate from entities that recently witnessed events
            // (RelevantEventCount > 0 indicates witnessed events)
            if (Sig.RelevantEventCount == 0 || Social.EdgeCount == 0) {
                continue;
            }

            // Check if this entity has enough pressure to want to share information
            // (sum of all pressure channels must exceed a minimum threshold)
            float TotalPressure = 0.0f;
            for (int32 c = 0; c < PressureChannelCount; ++c) {
                TotalPressure += PsychoView[i].Pressure[c];
            }

            // Minimum pressure threshold for propagation
            // Low-pressure entities don't gossip about trivial events
            if (TotalPressure < 0.5f) {
                continue;
            }

            // ─── Propagate to each social neighbor ───
            TArray<FMythicSocialEdge> SocialEdges;
            SocialGraph->GetEdges(ChunkContext.GetEntity(i), World->GetTimeSeconds(), SocialEdges);

            for (const FMythicSocialEdge &Edge : SocialEdges) {
                if (PropagationBudget <= 0) {
                    break;
                }

                // Don't share info with enemies — Rival is the only antagonistic relation. (The prior
                // static_cast<EMythicSocialRelation>(5) "Hostile" was a magic-ordinal bug: 5 is Subordinate and no
                // Hostile relation exists, so it wrongly muted gossip down authority/mentorship chains.)
                if (Edge.Relation == EMythicSocialRelation::Rival) {
                    continue; // Don't share info with enemies
                }

                --PropagationBudget;
                const FMassEntityHandle NeighborEntity = Edge.TargetEntity;

                // Calculate propagated event count with hop decay
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
                            // Mark as dirty so significance is recalculated
                            NeighborSig->bDirty = true;

                            // Add propagated event count (saturate at 0xFFFF). Clamp in 32-bit BEFORE narrowing:
                            // FMath::Min<uint16> would truncate the int sum to uint16 first (e.g. 131070 -> 65534),
                            // wrapping instead of pinning at the ceiling.
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

// Mythic Living World — Creature Ecology Processor Implementation

#include "Mass/Processors/CreatureEcologyProcessor.h"
#include "MassEntitySubsystem.h"
#include "MassExecutionContext.h"
#include "MassCommandBuffer.h"
#include "Mass/Fragments/MythicMassFragments.h"
#include "Mass/Tags/MythicMassTags.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "Engine/World.h"

UMythicCreatureEcologyProcessor::UMythicCreatureEcologyProcessor() {
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
    ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);
    bRequiresGameThreadExecution = false;
    bAutoRegisterWithProcessingPhases = true;

    // Run after population spawner
    ExecutionOrder.ExecuteAfter.Add(TEXT("UMythicPopulationSpawnerProcessor"));

    // Register queries in constructor so CallInitialize can bind the entity manager before ConfigureQueries
    CreatureQuery.RegisterWithProcessor(*this);
    HydratedCreatureQuery.RegisterWithProcessor(*this);
}

void UMythicCreatureEcologyProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) {
    // All creatures: identity + creature fragment + creature tag
    CreatureQuery.AddRequirement<FMythicIdentityFragment>(EMassFragmentAccess::ReadOnly);
    CreatureQuery.AddRequirement<FMythicCreatureFragment>(EMassFragmentAccess::ReadWrite);
    CreatureQuery.AddTagRequirement<FMythicCreatureTag>(EMassFragmentPresence::All);

    // Hydrated creatures: also have psychodynamic fragments for pressure
    HydratedCreatureQuery.AddRequirement<FMythicIdentityFragment>(EMassFragmentAccess::ReadOnly);
    HydratedCreatureQuery.AddRequirement<FMythicCreatureFragment>(EMassFragmentAccess::ReadWrite);
    HydratedCreatureQuery.AddRequirement<FMythicPsychodynamicFragment>(EMassFragmentAccess::ReadWrite);
    HydratedCreatureQuery.AddTagRequirement<FMythicCreatureTag>(EMassFragmentPresence::All);
    HydratedCreatureQuery.AddTagRequirement<FMythicHydratedTag>(EMassFragmentPresence::All);
}

float UMythicCreatureEcologyProcessor::ComputeTerritorialAggression(float BaseAggression, bool bNearDen, float TerritorialBoost) {
    // Transient boost near the den, clamped to 1; the bare authored base elsewhere. Recomputed from BaseAggression
    // every tick (idempotent — no accumulation), so aggression relaxes the moment the creature leaves its territory.
    return bNearDen ? FMath::Min(1.0f, BaseAggression + TerritorialBoost) : BaseAggression;
}

void UMythicCreatureEcologyProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicCreatureEcology_Execute);

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

    // Throttle
    TimeSinceLastTick += Context.GetDeltaTimeSeconds();
    if (TimeSinceLastTick < Settings->CreatureEcologyIntervalSeconds) {
        return;
    }
    TimeSinceLastTick = 0.0f;

    const float PackRadiusSq = FMath::Square(Settings->PackPressureShareRadius);
    const float TerritorialBoost = Settings->TerritorialAggressionBoost;
    int32 ContagionBudget = Settings->MaxHerdContagionPerTick;
    const int32 ThreatIdx = static_cast<int32>(EMythicPressureChannel::Threat);

    // ─── Step 1: Territorial Aggression Boost ──────────
    // For all creatures, boost aggression when near den cell

    CreatureQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext &ChunkContext) {
        const int32 NumEntities = ChunkContext.GetNumEntities();
        const auto IdentityView = ChunkContext.GetFragmentView<FMythicIdentityFragment>();
        auto CreatureView = ChunkContext.GetMutableFragmentView<FMythicCreatureFragment>();

        for (int32 i = 0; i < NumEntities; ++i) {
            FMythicCreatureFragment &Creature = CreatureView[i];
            const FMythicCellCoord &CurrentCell = IdentityView[i].Cell;
            const FMythicCellCoord &DenCell = Creature.DenCell;

            const int32 DistToDen = FMath::Abs(CurrentCell.X - DenCell.X) + FMath::Abs(CurrentCell.Y - DenCell.Y);
            const bool bNearDen = DistToDen <= static_cast<int32>(Creature.TerritorialRadius);

            // Recompute the EFFECTIVE aggression transiently from the authored base — boosted near the den, relaxed
            // away. NEVER mutate BaseAggression (the prior `BaseAggression += Boost` had no decay, so it ratcheted to
            // 1.0 and stuck — territorial aggression that never relaxed once the creature had visited its den).
            Creature.CurrentAggression = ComputeTerritorialAggression(Creature.BaseAggression, bNearDen, TerritorialBoost);
        }
    });

    // ─── Step 2: Pack Pressure Sharing + Herd-Flee Contagion ──
    // Only for hydrated creatures that have psychodynamic fragments

    // First pass: per pack, find the highest Threat AND the cell of the member holding it — pressure sharing is gated
    // on distance to that source cell (PackRadiusSq) so distant pack members don't telepathically inherit a far-off
    // scare. (Previously PackRadiusSq was computed but never used and the max threat was shared pack-wide regardless of
    // distance, defeating PackPressureShareRadius — Mass chunks are archetype-grouped, not spatial, so there was no
    // implicit spatial bound.)
    TMap<uint16, TPair<float, FMythicCellCoord>> PackMaxThreat;

    HydratedCreatureQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext &ChunkContext) {
        const int32 NumEntities = ChunkContext.GetNumEntities();
        const auto CreatureView = ChunkContext.GetFragmentView<FMythicCreatureFragment>();
        const auto IdentityView = ChunkContext.GetFragmentView<FMythicIdentityFragment>();
        const auto PsychoView = ChunkContext.GetFragmentView<FMythicPsychodynamicFragment>();

        for (int32 i = 0; i < NumEntities; ++i) {
            const uint16 PackId = CreatureView[i].PackId;
            if (PackId == 0) {
                continue; // Solitary creature, no pack
            }

            const float Threat = PsychoView[i].Pressure[ThreatIdx];
            TPair<float, FMythicCellCoord> &Entry = PackMaxThreat.FindOrAdd(PackId, TPair<float, FMythicCellCoord>(0.0f, FMythicCellCoord()));
            if (Threat > Entry.Key) {
                Entry.Key = Threat;
                Entry.Value = IdentityView[i].Cell;
            }
        }
    });

    // Second pass: share max threat back to pack members + herd-flee contagion
    HydratedCreatureQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext &ChunkContext) {
        const int32 NumEntities = ChunkContext.GetNumEntities();
        const auto CreatureView = ChunkContext.GetFragmentView<FMythicCreatureFragment>();
        const auto IdentityView = ChunkContext.GetFragmentView<FMythicIdentityFragment>();
        auto PsychoView = ChunkContext.GetMutableFragmentView<FMythicPsychodynamicFragment>();

        for (int32 i = 0; i < NumEntities && ContagionBudget > 0; ++i) {
            const uint16 PackId = CreatureView[i].PackId;
            if (PackId == 0) {
                continue;
            }

            const TPair<float, FMythicCellCoord> *PackMax = PackMaxThreat.Find(PackId);
            if (!PackMax) {
                continue;
            }

            // Distance-gate: only members within PackPressureShareRadius of the threat source share its pressure
            // (the documented contract). Squared-Euclidean so we consume PackRadiusSq without a sqrt.
            const FMythicCellCoord &Cell = IdentityView[i].Cell;
            const int32 dX = Cell.X - PackMax->Value.X;
            const int32 dY = Cell.Y - PackMax->Value.Y;
            if (static_cast<float>(dX * dX + dY * dY) > PackRadiusSq) {
                continue;
            }

            FMythicPsychodynamicFragment &Psycho = PsychoView[i];

            // Pack pressure sharing: raise this member's threat toward pack max (dampened)
            if (PackMax->Key > Psycho.Pressure[ThreatIdx]) {
                const float SharedFraction = 0.5f;
                Psycho.Pressure[ThreatIdx] = FMath::Lerp(Psycho.Pressure[ThreatIdx], PackMax->Key, SharedFraction);
                --ContagionBudget;
            }
        }
    });
}

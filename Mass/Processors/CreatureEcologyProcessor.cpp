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

            // Boost/decay aggression based on den proximity
            if (bNearDen) {
                Creature.BaseAggression = FMath::Min(1.0f, Creature.BaseAggression + TerritorialBoost);
            }
        }
    });

    // ─── Step 2: Pack Pressure Sharing + Herd-Flee Contagion ──
    // Only for hydrated creatures that have psychodynamic fragments

    // First pass: collect pack pressure state into a map by PackId
    TMap<uint16, float> PackMaxThreat;

    HydratedCreatureQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext &ChunkContext) {
        const int32 NumEntities = ChunkContext.GetNumEntities();
        const auto CreatureView = ChunkContext.GetFragmentView<FMythicCreatureFragment>();
        const auto PsychoView = ChunkContext.GetFragmentView<FMythicPsychodynamicFragment>();

        for (int32 i = 0; i < NumEntities; ++i) {
            const uint16 PackId = CreatureView[i].PackId;
            if (PackId == 0) {
                continue; // Solitary creature, no pack
            }

            const float Threat = PsychoView[i].Pressure[ThreatIdx];
            float &MaxThreat = PackMaxThreat.FindOrAdd(PackId, 0.0f);
            MaxThreat = FMath::Max(MaxThreat, Threat);
        }
    });

    // Second pass: share max threat back to pack members + herd-flee contagion
    HydratedCreatureQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext &ChunkContext) {
        const int32 NumEntities = ChunkContext.GetNumEntities();
        const auto CreatureView = ChunkContext.GetFragmentView<FMythicCreatureFragment>();
        auto PsychoView = ChunkContext.GetMutableFragmentView<FMythicPsychodynamicFragment>();

        for (int32 i = 0; i < NumEntities && ContagionBudget > 0; ++i) {
            const uint16 PackId = CreatureView[i].PackId;
            if (PackId == 0) {
                continue;
            }

            const float *MaxThreat = PackMaxThreat.Find(PackId);
            if (!MaxThreat) {
                continue;
            }

            FMythicPsychodynamicFragment &Psycho = PsychoView[i];

            // Pack pressure sharing: raise this member's threat toward pack max (dampened)
            if (*MaxThreat > Psycho.Pressure[ThreatIdx]) {
                const float SharedFraction = 0.5f;
                Psycho.Pressure[ThreatIdx] = FMath::Lerp(Psycho.Pressure[ThreatIdx], *MaxThreat, SharedFraction);
                --ContagionBudget;
            }
        }
    });
}


#include "Mass/Processors/CreatureEcologyProcessor.h"
#include "MassEntitySubsystem.h"
#include "MassExecutionContext.h"
#include "MassCommandBuffer.h"
#include "Mass/Fragments/MythicMassFragments.h"
#include "Mass/Tags/MythicMassTags.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "World/LivingWorld/Creatures/CreatureAggressionTypes.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"

UMythicCreatureEcologyProcessor::UMythicCreatureEcologyProcessor() {
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
    ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);
    bRequiresGameThreadExecution = false;
    bAutoRegisterWithProcessingPhases = true;

    ExecutionOrder.ExecuteAfter.Add(TEXT("UMythicPopulationSpawnerProcessor"));

    CreatureQuery.RegisterWithProcessor(*this);
    HydratedCreatureQuery.RegisterWithProcessor(*this);
}

void UMythicCreatureEcologyProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) {
    CreatureQuery.AddRequirement<FMythicIdentityFragment>(EMassFragmentAccess::ReadOnly);
    CreatureQuery.AddRequirement<FMythicCreatureFragment>(EMassFragmentAccess::ReadWrite);
    CreatureQuery.AddTagRequirement<FMythicCreatureTag>(EMassFragmentPresence::All);

    HydratedCreatureQuery.AddRequirement<FMythicIdentityFragment>(EMassFragmentAccess::ReadOnly);
    HydratedCreatureQuery.AddRequirement<FMythicCreatureFragment>(EMassFragmentAccess::ReadWrite);
    HydratedCreatureQuery.AddRequirement<FMythicPsychodynamicFragment>(EMassFragmentAccess::ReadWrite);
    HydratedCreatureQuery.AddTagRequirement<FMythicCreatureTag>(EMassFragmentPresence::All);
    HydratedCreatureQuery.AddTagRequirement<FMythicHydratedTag>(EMassFragmentPresence::All);
}

float UMythicCreatureEcologyProcessor::ComputeTerritorialAggression(float BaseAggression, bool bNearDen, float TerritorialBoost) {
    return bNearDen ? FMath::Min(1.0f, BaseAggression + TerritorialBoost) : BaseAggression;
}

void UMythicCreatureEcologyProcessor::BuildAggressionMatrix(const UDataTable *Table, FMythicCreatureAggressionMatrix &OutMatrix) {
    OutMatrix.Entries.Reset();
    if (!Table) {
        return;
    }
    TArray<FMythicCreatureAggressionRow *> Rows;
    Table->GetAllRows<FMythicCreatureAggressionRow>(TEXT("CreatureAggressionMatrix"), Rows);
    OutMatrix.Entries.Reserve(Rows.Num());
    for (const FMythicCreatureAggressionRow *Row : Rows) {
        if (!Row) {
            continue;
        }
        OutMatrix.Entries.Add(FMythicCreatureAggressionMatrix::PackKey(Row->AttackerSpeciesId, Row->TargetSpeciesId),
                              FMath::Clamp(Row->Aggression, 0.0f, 1.0f));
    }
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

    TimeSinceLastTick += Context.GetDeltaTimeSeconds();
    if (TimeSinceLastTick < Settings->CreatureEcologyIntervalSeconds) {
        return;
    }
    TimeSinceLastTick = 0.0f;

    if (!bAggressionMatrixResolved && IsInGameThread()) {
        bAggressionMatrixResolved = true;
        const UDataTable *Table = Settings->CreatureAggressionMatrix.LoadSynchronous();
        BuildAggressionMatrix(Table, AggressionMatrix);
    }

    const float PackRadiusSq = FMath::Square(Settings->PackPressureShareRadius);
    const float TerritorialBoost = Settings->TerritorialAggressionBoost;
    int32 ContagionBudget = Settings->MaxHerdContagionPerTick;
    const int32 ThreatIdx = static_cast<int32>(EMythicPressureChannel::Threat);

    TMap<FMythicCellCoord, TSet<uint8>> CellSpecies;
    if (!AggressionMatrix.IsEmpty()) {
        CreatureQuery.ForEachEntityChunk(Context, [&CellSpecies](FMassExecutionContext &ChunkContext) {
            const int32 NumEntities = ChunkContext.GetNumEntities();
            const auto IdentityView = ChunkContext.GetFragmentView<FMythicIdentityFragment>();
            const auto CreatureView = ChunkContext.GetFragmentView<FMythicCreatureFragment>();
            for (int32 i = 0; i < NumEntities; ++i) {
                CellSpecies.FindOrAdd(IdentityView[i].Cell).Add(CreatureView[i].SpeciesId);
            }
        });
    }


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

            Creature.CurrentAggression = ComputeTerritorialAggression(Creature.BaseAggression, bNearDen, TerritorialBoost);

            if (!AggressionMatrix.IsEmpty()) {
                if (const TSet<uint8> *Here = CellSpecies.Find(CurrentCell)) {
                    float MaxCross = 0.0f;
                    for (const uint8 OtherSpecies : *Here) {
                        if (OtherSpecies == Creature.SpeciesId) {
                            continue;
                        }
                        MaxCross = FMath::Max(MaxCross, AggressionMatrix.Get(Creature.SpeciesId, OtherSpecies));
                    }
                    Creature.CurrentAggression = FMath::Max(Creature.CurrentAggression, MaxCross);
                }
            }
        }
    });


    TMap<uint16, TPair<float, FMythicCellCoord>> PackMaxThreat;

    HydratedCreatureQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext &ChunkContext) {
        const int32 NumEntities = ChunkContext.GetNumEntities();
        const auto CreatureView = ChunkContext.GetFragmentView<FMythicCreatureFragment>();
        const auto IdentityView = ChunkContext.GetFragmentView<FMythicIdentityFragment>();
        const auto PsychoView = ChunkContext.GetFragmentView<FMythicPsychodynamicFragment>();

        for (int32 i = 0; i < NumEntities; ++i) {
            const uint16 PackId = CreatureView[i].PackId;
            if (PackId == 0) {
                continue;
            }

            const float Threat = PsychoView[i].Pressure[ThreatIdx];
            TPair<float, FMythicCellCoord> &Entry = PackMaxThreat.FindOrAdd(PackId, TPair<float, FMythicCellCoord>(0.0f, FMythicCellCoord()));
            if (Threat > Entry.Key) {
                Entry.Key = Threat;
                Entry.Value = IdentityView[i].Cell;
            }
        }
    });

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

            const FMythicCellCoord &Cell = IdentityView[i].Cell;
            const int32 dX = Cell.X - PackMax->Value.X;
            const int32 dY = Cell.Y - PackMax->Value.Y;
            if (static_cast<float>(dX * dX + dY * dY) > PackRadiusSq) {
                continue;
            }

            FMythicPsychodynamicFragment &Psycho = PsychoView[i];

            if (PackMax->Key > Psycho.Pressure[ThreatIdx]) {
                const float SharedFraction = 0.5f;
                Psycho.Pressure[ThreatIdx] = FMath::Lerp(Psycho.Pressure[ThreatIdx], PackMax->Key, SharedFraction);
                --ContagionBudget;
            }
        }
    });
}

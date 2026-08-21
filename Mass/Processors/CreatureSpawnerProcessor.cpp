
#include "Mass/Processors/CreatureSpawnerProcessor.h"
#include "MassExecutionContext.h"
#include "MassCommandBuffer.h"
#include "MassCommands.h"
#include "Mass/Fragments/MythicMassFragments.h"
#include "Mass/Tags/MythicMassTags.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/Settlements/MythicSettlement.h"
#include "World/LivingWorld/Creatures/CreatureSpeciesTypes.h"
#include "AI/NPCs/MythicNPCCharacter.h"
#include "World/Death/MythicCorpseHazardSubsystem.h"
#include "Engine/DataTable.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

UMythicCreatureSpawnerProcessor::UMythicCreatureSpawnerProcessor() {
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
    ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);
    bRequiresGameThreadExecution = true;
    bAutoRegisterWithProcessingPhases = true;

    QueryBasedPruning = EMassQueryBasedPruning::Never;

    ExecutionOrder.ExecuteAfter.Add(TEXT("UMythicTerritoryPatrolSpawnerProcessor"));

    ExistingCreatureQuery.RegisterWithProcessor(*this);
}

void UMythicCreatureSpawnerProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) {
    ExistingCreatureQuery.AddRequirement<FMythicIdentityFragment>(EMassFragmentAccess::ReadOnly);
    ExistingCreatureQuery.AddRequirement<FMythicCreatureFragment>(EMassFragmentAccess::ReadOnly);
    ExistingCreatureQuery.AddTagRequirement<FMythicCreatureTag>(EMassFragmentPresence::All);
}

uint16 UMythicCreatureSpawnerProcessor::AllocatePackId() {
    const uint16 Id = NextPackId++;
    if (NextPackId == 0) {
        NextPackId = 1;
    }
    return Id;
}

int32 UMythicCreatureSpawnerProcessor::ComputeCreatureTargetDensity(int32 MaxCreaturesPerBiomeCell, float DensityScale, int32 SystemCap) {
    if (MaxCreaturesPerBiomeCell <= 0 || DensityScale <= 0.0f) {
        return 0;
    }
    const int32 Scaled = FMath::RoundToInt(static_cast<float>(MaxCreaturesPerBiomeCell) * DensityScale);
    return FMath::Clamp(Scaled, 0, FMath::Max(0, SystemCap));
}

void UMythicCreatureSpawnerProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicCreatureSpawner_Execute);

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
    if (TimeSinceLastTick < Settings->CreatureSpawnIntervalSeconds) {
        return;
    }
    TimeSinceLastTick = 0.0f;

    UMythicTerritoryGrid *Grid = LWS->GetTerritoryGrid();
    if (!Grid) {
        return;
    }

    TConstArrayView<FMythicCreatureSpeciesRow> CodeDefaults;
    TArray<const FMythicCreatureSpeciesRow *> SpeciesByBiome[MythicBiomeCount];
    {
        if (const UDataTable *Table = Settings->CreatureSpeciesTable.LoadSynchronous()) {
            TArray<FMythicCreatureSpeciesRow *> Rows;
            Table->GetAllRows<FMythicCreatureSpeciesRow>(TEXT("CreatureSpawner"), Rows);
            for (const FMythicCreatureSpeciesRow *Row : Rows) {
                if (!Row) {
                    continue;
                }
                const int32 BiomeIdx = static_cast<int32>(Row->Biome);
                if (BiomeIdx >= 0 && BiomeIdx < MythicBiomeCount) {
                    SpeciesByBiome[BiomeIdx].Add(Row);
                }
            }
        } else {
            CodeDefaults = MythicCreatureDefaults::GetCodeDefaultSpecies();
            for (const FMythicCreatureSpeciesRow &Row : CodeDefaults) {
                const int32 BiomeIdx = static_cast<int32>(Row.Biome);
                if (BiomeIdx >= 0 && BiomeIdx < MythicBiomeCount) {
                    SpeciesByBiome[BiomeIdx].Add(&Row);
                }
            }
        }
    }

    TArray<FMythicCellCoord> PlayerCells;
    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It) {
        if (const APlayerController *PC = It->Get()) {
            if (const APawn *Pawn = PC->GetPawn()) {
                PlayerCells.Add(Grid->WorldToCell(Pawn->GetActorLocation()));
            }
        }
    }
    if (PlayerCells.IsEmpty()) {
        return;
    }

    TMap<FMythicCellCoord, int32> CellCreatureCounts;
    ExistingCreatureQuery.ForEachEntityChunk(Context, [&CellCreatureCounts](FMassExecutionContext &ChunkContext) {
        const int32 NumEntities = ChunkContext.GetNumEntities();
        const auto IdentityView = ChunkContext.GetFragmentView<FMythicIdentityFragment>();
        for (int32 i = 0; i < NumEntities; ++i) {
            CellCreatureCounts.FindOrAdd(IdentityView[i].Cell)++;
        }
    });

    const float SpawnRadius = Settings->CreatureSpawnRadius;
    const float SpawnRadiusSq = FMath::Square(SpawnRadius);
    const int32 SpawnRadiusCells = FMath::CeilToInt(SpawnRadius);
    int32 SpawnBudget = Settings->MaxCreatureSpawnsPerTick;

    struct FMythicCreatureSpawnData {
        FMythicIdentityFragment Identity;
        FMythicCreatureFragment Creature;
        FMythicSignificanceFragment Significance;
    };
    TArray<FMythicCreatureSpawnData> SpawnDataArray;
    SpawnDataArray.Reserve(SpawnBudget);

    const UMythicCorpseHazardSubsystem *CarrionHazard = World->GetSubsystem<UMythicCorpseHazardSubsystem>();
    if (CarrionHazard && CarrionHazard->GetRegisteredCorpseCount() <= 0) {
        CarrionHazard = nullptr;
    }
    const float CarrionQueryRadius = Grid->GetCellSize();

    TSet<FMythicCellCoord> ConsideredCells;
    for (const FMythicCellCoord &PlayerCell : PlayerCells) {
        for (int32 DY = -SpawnRadiusCells; DY <= SpawnRadiusCells && SpawnBudget > 0; ++DY) {
            for (int32 DX = -SpawnRadiusCells; DX <= SpawnRadiusCells && SpawnBudget > 0; ++DX) {
                if ((DX * DX + DY * DY) > SpawnRadiusSq) {
                    continue;
                }

                const FMythicCellCoord Cell(PlayerCell.X + DX, PlayerCell.Y + DY);
                if (!Grid->IsValidCoord(Cell)) {
                    continue;
                }

                if (ConsideredCells.Contains(Cell)) {
                    continue;
                }
                ConsideredCells.Add(Cell);

                FMythicSettlementData Settlement;
                if (LWS->CopySettlementAtCell(Cell, Settlement)) {
                    continue;
                }
                const FMythicTerritoryCell TC = Grid->GetCell(Cell);
                if (TC.DominantFaction.IsValid()) {
                    continue;
                }

                const EMythicBiome Biome = Grid->GetBiomeAtCell(Cell);
                const int32 BiomeIdx = static_cast<int32>(Biome);
                if (BiomeIdx < 0 || BiomeIdx >= MythicBiomeCount) {
                    continue;
                }
                const TArray<const FMythicCreatureSpeciesRow *> &Eligible = SpeciesByBiome[BiomeIdx];
                if (Eligible.IsEmpty()) {
                    continue;
                }

                const int32 TargetCount = ComputeCreatureTargetDensity(
                    Settings->MaxCreaturesPerBiomeCell, Settings->CreatureSpawnDensityScale, Settings->MaxEntitiesPerCell);
                const int32 CurrentCount = CellCreatureCounts.FindRef(Cell);

                float Carrion = 0.0f;
                if (CarrionHazard) {
                    Carrion = CarrionHazard->GetTotalCarrionAttractivenessNear(Grid->CellToWorld(Cell), CarrionQueryRadius);
                }
                const int32 CarrionBonus = (Carrion > 0.0f)
                    ? FMath::Clamp(FMath::RoundToInt(Carrion), 0, FMath::Max(0, Settings->MaxEntitiesPerCell - TargetCount))
                    : 0;

                const int32 Deficit = (TargetCount + CarrionBonus) - CurrentCount;
                if (Deficit <= 0) {
                    continue;
                }

                const uint32 CellSeed = HashCombine(GetTypeHash(Cell), 0x53706563u);
                FRandomStream Stream(static_cast<int32>(CellSeed));

                auto EffectiveWeight = [Carrion](const FMythicCreatureSpeciesRow *Row) -> float {
                    const float Base = FMath::Max(0.0f, Row->SpawnWeight);
                    return (Row->bIsScavenger && Carrion > 0.0f) ? Base * (1.0f + Carrion) : Base;
                };

                float TotalWeight = 0.0f;
                for (const FMythicCreatureSpeciesRow *Row : Eligible) {
                    TotalWeight += EffectiveWeight(Row);
                }
                const FMythicCreatureSpeciesRow *Picked = Eligible[0];
                if (TotalWeight > UE_KINDA_SMALL_NUMBER) {
                    float Roll = Stream.FRandRange(0.0f, TotalWeight);
                    for (const FMythicCreatureSpeciesRow *Row : Eligible) {
                        Roll -= EffectiveWeight(Row);
                        if (Roll <= 0.0f) {
                            Picked = Row;
                            break;
                        }
                    }
                }

                const bool bPack = Picked->bIsPackAnimal;
                uint16 PackId = 0;
                int32 DesiredMembers = 1;
                if (bPack) {
                    const int32 MinPack = FMath::Max<int32>(1, Picked->MinPackSize);
                    const int32 MaxPack = FMath::Max<int32>(MinPack, Picked->MaxPackSize);
                    DesiredMembers = Stream.RandRange(MinPack, MaxPack);
                    PackId = AllocatePackId();
                }

                const int32 ToSpawn = FMath::Min3(Deficit, DesiredMembers, SpawnBudget);
                for (int32 SpawnIdx = 0; SpawnIdx < ToSpawn; ++SpawnIdx) {
                    FMythicCreatureSpawnData Data;

                    Data.Identity.Cell = Cell;
                    uint32 Hash = HashCombine(GetTypeHash(Cell), static_cast<uint32>(Picked->SpeciesId));
                    Hash = HashCombine(Hash, ++SpawnCounter);
                    Hash = (Hash ^ 61u) ^ (Hash >> 16u);
                    Hash *= 9u;
                    Hash = Hash ^ (Hash >> 4u);
                    Hash *= 0x27d4eb2du;
                    Hash = Hash ^ (Hash >> 15u);
                    Data.Identity.NameHash = Hash;
                    Data.Identity.VisualArchetype = static_cast<uint8>(Hash % 8);

                    Data.Creature.SpeciesId = Picked->SpeciesId;
                    Data.Creature.PackId = PackId;
                    Data.Creature.BaseAggression = FMath::Clamp(Picked->BaseAggression, 0.0f, 1.0f);
                    Data.Creature.CurrentAggression = Data.Creature.BaseAggression;
                    Data.Creature.DenCell = Cell;
                    Data.Creature.TerritorialRadius = Picked->DefaultTerritorialRadius;

                    Data.Significance.Tier = EMythicSignificanceTier::Tier0_Ambient;

                    SpawnDataArray.Add(MoveTemp(Data));
                }
                SpawnBudget -= ToSpawn;
            }
        }
    }

    if (SpawnDataArray.Num() > 0) {
        Context.Defer().PushCommand<FMassDeferredCreateCommand>([SpawnDataArray](FMassEntityManager &Manager) {
            TRACE_CPUPROFILER_EVENT_SCOPE(MythicCreatureSpawner_DeferredSpawn);

            const UScriptStruct *Composition[] = {
                FMythicIdentityFragment::StaticStruct(),
                FMythicCreatureFragment::StaticStruct(),
                FMythicSignificanceFragment::StaticStruct(),
                FMythicCreatureTag::StaticStruct()
            };
            FMassArchetypeHandle Archetype = Manager.CreateArchetype(MakeArrayView(Composition));

            TArray<FMassEntityHandle> SpawnedEntities;
            Manager.BatchCreateEntities(Archetype, SpawnDataArray.Num(), SpawnedEntities);

            for (int32 i = 0; i < SpawnDataArray.Num(); ++i) {
                const FMythicCreatureSpawnData &Data = SpawnDataArray[i];
                const FMassEntityHandle Entity = SpawnedEntities[i];
                Manager.GetFragmentDataChecked<FMythicIdentityFragment>(Entity) = Data.Identity;
                Manager.GetFragmentDataChecked<FMythicCreatureFragment>(Entity) = Data.Creature;
                Manager.GetFragmentDataChecked<FMythicSignificanceFragment>(Entity) = Data.Significance;
            }
        });
    }

    const float DespawnRadiusSq = FMath::Square(Settings->CreatureDespawnDistance);
    int32 DespawnBudget = Settings->MaxCreatureSpawnsPerTick;

    ExistingCreatureQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext &ChunkContext) {
        if (DespawnBudget <= 0) {
            return;
        }
        const int32 NumEntities = ChunkContext.GetNumEntities();
        const auto IdentityView = ChunkContext.GetFragmentView<FMythicIdentityFragment>();

        for (int32 i = 0; i < NumEntities && DespawnBudget > 0; ++i) {
            const FMythicCellCoord &EntityCell = IdentityView[i].Cell;

            bool bNearPlayer = false;
            for (const FMythicCellCoord &PlayerCell : PlayerCells) {
                const float DistSq = static_cast<float>(
                    FMath::Square(EntityCell.X - PlayerCell.X) + FMath::Square(EntityCell.Y - PlayerCell.Y));
                if (DistSq <= DespawnRadiusSq) {
                    bNearPlayer = true;
                    break;
                }
            }

            if (!bNearPlayer) {
                const FMassEntityHandle DespawnEntity = ChunkContext.GetEntity(i);
                if (AMythicNPCCharacter *Actor = LWS->FindEmbodiedActor(DespawnEntity)) {
                    Actor->Destroy();
                }
                LWS->UnregisterEmbodiedActor(DespawnEntity);
                Context.Defer().DestroyEntity(DespawnEntity);
                --DespawnBudget;
            }
        }
    });
}

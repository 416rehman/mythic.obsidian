// Mythic Living World — Population Spawner Processor Implementation

#include "Mass/Processors/PopulationSpawnerProcessor.h"
#include "MassEntitySubsystem.h"
#include "MassExecutionContext.h"
#include "MassCommandBuffer.h"
#include "MassCommands.h"
#include "Mass/Fragments/MythicMassFragments.h"
#include "Mass/Tags/MythicMassTags.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/Settlements/SettlementRegistry.h"
#include "World/LivingWorld/Settlements/MythicSettlement.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

UMythicPopulationSpawnerProcessor::UMythicPopulationSpawnerProcessor() {
    // Run during PrePhysics on server/standalone only — clients receive replicated state eventually
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
    ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);
    bRequiresGameThreadExecution = true;
    bAutoRegisterWithProcessingPhases = true;

    // This processor spawns the initial NPC entities — it must run even when zero NPC
    // archetypes exist. Without this, MASS prunes it at startup (chicken-and-egg deadlock).
    QueryBasedPruning = EMassQueryBasedPruning::Never;

    // Register queries in constructor so CallInitialize can bind the entity manager before ConfigureQueries
    ExistingNPCQuery.RegisterWithProcessor(*this);
}

void UMythicPopulationSpawnerProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) {
    // Query existing NPC entities by Identity + Significance fragments + NPC tag
    ExistingNPCQuery.AddRequirement<FMythicIdentityFragment>(EMassFragmentAccess::ReadOnly);
    ExistingNPCQuery.AddRequirement<FMythicSignificanceFragment>(EMassFragmentAccess::ReadOnly);
    ExistingNPCQuery.AddTagRequirement<FMythicNPCTag>(EMassFragmentPresence::All);
}

void UMythicPopulationSpawnerProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicPopulationSpawner_Execute);

    // Get the living world subsystem
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

    // Throttle: only run at configured interval
    TimeSinceLastTick += Context.GetDeltaTimeSeconds();
    if (TimeSinceLastTick < Settings->PopulationSpawnIntervalSeconds) {
        return;
    }
    TimeSinceLastTick = 0.0f;

    UE_LOG(LogMythLivingWorld, Verbose, TEXT("PopulationSpawner: tick fired (interval %.1fs)"),
           Settings->PopulationSpawnIntervalSeconds);

    UMythicTerritoryGrid *Grid = LWS->GetTerritoryGrid();
    UMythicSettlementRegistry *Registry = LWS->GetSettlementRegistry();
    UMythicFactionDatabase *FactionDB = LWS->GetFactionDatabase();

    if (!Grid || !Registry || !FactionDB) {
        return;
    }

    // ─── Step 1: Gather player positions ───────────────

    TArray<FMythicCellCoord> PlayerCells;
    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It) {
        if (const APlayerController *PC = It->Get()) {
            if (const APawn *Pawn = PC->GetPawn()) {
                PlayerCells.Add(Grid->WorldToCell(Pawn->GetActorLocation()));
            }
        }
    }

    if (PlayerCells.IsEmpty()) {
        UE_LOG(LogMythLivingWorld, Verbose, TEXT("PopulationSpawner: no player pawns found, skipping."));
        return;
    }

    UE_LOG(LogMythLivingWorld, Verbose, TEXT("PopulationSpawner: %d player(s) detected."), PlayerCells.Num());

    // ─── Step 2: Count existing entities per cell ──────

    TMap<FMythicCellCoord, int32> CellEntityCounts;
    ExistingNPCQuery.ForEachEntityChunk(Context, [&CellEntityCounts](FMassExecutionContext &ChunkContext) {
        const int32 NumEntities = ChunkContext.GetNumEntities();
        const auto IdentityView = ChunkContext.GetFragmentView<FMythicIdentityFragment>();

        for (int32 i = 0; i < NumEntities; ++i) {
            CellEntityCounts.FindOrAdd(IdentityView[i].Cell)++;
        }
    });

    // ─── Step 3: Determine cells needing spawns ────────

    const float SpawnRadiusSq = FMath::Square(Settings->PopulationSpawnRadius);
    const int32 SpawnRadiusCells = FMath::CeilToInt(Settings->PopulationSpawnRadius);
    int32 SpawnBudget = Settings->MaxSpawnsPerTick;

    struct FMythicNPCPopulationSpawnData {
        FMythicIdentityFragment Identity;
        FMythicScheduleFragment Schedule;
        FMythicSignificanceFragment Significance;
    };
    TArray<FMythicNPCPopulationSpawnData> SpawnDataArray;
    SpawnDataArray.Reserve(SpawnBudget);

    // Collect unique cells within spawn radius of any player
    TSet<FMythicCellCoord> CellsToPopulate;
    for (const FMythicCellCoord &PlayerCell : PlayerCells) {
        for (int32 DY = -SpawnRadiusCells; DY <= SpawnRadiusCells && SpawnBudget > 0; ++DY) {
            for (int32 DX = -SpawnRadiusCells; DX <= SpawnRadiusCells && SpawnBudget > 0; ++DX) {
                // Circular check
                if ((DX * DX + DY * DY) > SpawnRadiusSq) {
                    continue;
                }

                const FMythicCellCoord CandidateCell(PlayerCell.X + DX, PlayerCell.Y + DY);
                if (!Grid->IsValidCoord(CandidateCell)) {
                    continue;
                }

                // Only populate cells that belong to a settlement
                const FMythicSettlementData *Settlement = Registry->GetSettlementAtCell(CandidateCell);
                if (!Settlement || !Settlement->GoverningFaction.IsValid()) {
                    continue;
                }

                if (CellsToPopulate.Contains(CandidateCell)) {
                    continue;
                }
                CellsToPopulate.Add(CandidateCell);

                FMythicFactionData FactionData;
                if (!FactionDB->GetFaction(Settlement->GoverningFaction, FactionData)) {
                    continue;
                }

                // Capacity = ControlledCellCount × PopulationPerCell (same formula as WorldSimThread)
                const int32 FactionCapacity = FactionData.ControlledCellCount * Settings->PopulationPerCell;

                const int32 TargetCount = ComputeTargetDensity(
                    Settlement->MaxPopulationDensity,
                    Settings->MaxEntitiesPerCell,
                    FactionData.Population,
                    FactionCapacity
                    );

                const int32 CurrentCount = CellEntityCounts.FindRef(CandidateCell);
                const int32 Deficit = TargetCount - CurrentCount;

                if (Deficit <= 0) {
                    continue;
                }

                // Spawn entities to fill the deficit (budget-capped)
                const int32 ToSpawn = FMath::Min(Deficit, SpawnBudget);

                for (int32 SpawnIdx = 0; SpawnIdx < ToSpawn; ++SpawnIdx) {
                    FMythicNPCPopulationSpawnData SpawnData;
                    SpawnData.Identity.Faction = Settlement->GoverningFaction;
                    SpawnData.Identity.Cell = CandidateCell;
                    SpawnData.Identity.NameHash = GetTypeHash(CandidateCell) ^ (SpawnIdx * 2654435761u);
                    SpawnData.Identity.VisualArchetype = static_cast<uint8>(SpawnData.Identity.NameHash % 8);

                    SpawnData.Schedule.Phase = EMythicSchedulePhase::Idle;
                    SpawnData.Schedule.HomeCell = CandidateCell;
                    SpawnData.Schedule.WorkCell = CandidateCell;

                    SpawnData.Significance.Tier = EMythicSignificanceTier::Tier0_Ambient;

                    SpawnDataArray.Add(SpawnData);
                }

                SpawnBudget -= ToSpawn;

                UE_LOG(LogMythLivingWorld, Verbose, TEXT("PopulationSpawner: cell (%d,%d) — target=%d, current=%d, spawned=%d (faction=%d, pop=%d, cap=%d)"),
                       CandidateCell.X, CandidateCell.Y, TargetCount, CurrentCount, ToSpawn,
                       Settlement->GoverningFaction.Index, FactionData.Population, FactionCapacity);
            }
        }
    }

    if (SpawnDataArray.Num() > 0) {
        Context.Defer().PushCommand<FMassDeferredCreateCommand>([SpawnDataArray](FMassEntityManager &Manager) {
            TRACE_CPUPROFILER_EVENT_SCOPE(MythicPopulationSpawner_DeferredSpawn);

            const UScriptStruct *Composition[] = {
                FMythicIdentityFragment::StaticStruct(),
                FMythicScheduleFragment::StaticStruct(),
                FMythicSignificanceFragment::StaticStruct(),
                FMythicNPCTag::StaticStruct()
            };
            FMassArchetypeHandle Archetype = Manager.CreateArchetype(MakeArrayView(Composition));

            TArray<FMassEntityHandle> SpawnedEntities;
            Manager.BatchCreateEntities(Archetype, SpawnDataArray.Num(), SpawnedEntities);

            for (int32 i = 0; i < SpawnDataArray.Num(); ++i) {
                const FMythicNPCPopulationSpawnData &Data = SpawnDataArray[i];
                FMassEntityHandle Entity = SpawnedEntities[i];

                Manager.GetFragmentDataChecked<FMythicIdentityFragment>(Entity) = Data.Identity;
                Manager.GetFragmentDataChecked<FMythicScheduleFragment>(Entity) = Data.Schedule;
                Manager.GetFragmentDataChecked<FMythicSignificanceFragment>(Entity) = Data.Significance;
            }
        });
    }

    // ─── Step 4: Despawn entities too far from all players ──

    const float DespawnRadiusSq = FMath::Square(Settings->PopulationDespawnDistance);
    int32 DespawnBudget = Settings->MaxDespawnsPerTick;

    ExistingNPCQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext &ChunkContext) {
        if (DespawnBudget <= 0) {
            return;
        }

        const int32 NumEntities = ChunkContext.GetNumEntities();
        const auto IdentityView = ChunkContext.GetFragmentView<FMythicIdentityFragment>();

        for (int32 i = 0; i < NumEntities && DespawnBudget > 0; ++i) {
            const FMythicCellCoord &EntityCell = IdentityView[i].Cell;

            // Check if any player is within despawn distance
            bool bNearPlayer = false;
            for (const FMythicCellCoord &PlayerCell : PlayerCells) {
                const float DistSq = static_cast<float>(
                    FMath::Square(EntityCell.X - PlayerCell.X) + FMath::Square(EntityCell.Y - PlayerCell.Y)
                );
                if (DistSq <= DespawnRadiusSq) {
                    bNearPlayer = true;
                    break;
                }
            }

            if (!bNearPlayer) {
                Context.Defer().DestroyEntity(ChunkContext.GetEntity(i));
                --DespawnBudget;
            }
        }
    });
}

int32 UMythicPopulationSpawnerProcessor::ComputeTargetDensity(
    int32 SettlementMaxDensity,
    int32 SystemMaxPerCell,
    int32 FactionPopulation,
    int32 FactionCapacity) const {
    // Cap by system limit
    const int32 EffectiveMax = FMath::Min(SettlementMaxDensity, SystemMaxPerCell);

    // Scale by faction health (population / capacity)
    if (FactionCapacity <= 0) {
        return 0;
    }

    const float FillRatio = FMath::Clamp(static_cast<float>(FactionPopulation) / static_cast<float>(FactionCapacity), 0.0f, 1.0f);
    return FMath::CeilToInt(EffectiveMax * FillRatio);
}

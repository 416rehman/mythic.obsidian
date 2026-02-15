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

    // Register queries in constructor so CallInitialize can bind the entity manager before ConfigureQueries
    RegisterQuery(ExistingNPCQuery);
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
        return;
    }

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

                const FMythicFactionData *FactionData = FactionDB->GetFaction(Settlement->GoverningFaction);
                if (!FactionData) {
                    continue;
                }

                // Capacity = ControlledCellCount × PopulationPerCell (same formula as WorldSimThread)
                const int32 FactionCapacity = FactionData->ControlledCellCount * Settings->PopulationPerCell;

                const int32 TargetCount = ComputeTargetDensity(
                    Settlement->MaxPopulationDensity,
                    Settings->MaxEntitiesPerCell,
                    FactionData->Population,
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
                    FMassEntityHandle NewEntity = EntityManager.ReserveEntity();

                    FMythicIdentityFragment IdentityData;
                    IdentityData.Faction = Settlement->GoverningFaction;
                    IdentityData.Cell = CandidateCell;
                    IdentityData.NameHash = GetTypeHash(CandidateCell) ^ (SpawnIdx * 2654435761u);
                    IdentityData.VisualArchetype = static_cast<uint8>(IdentityData.NameHash % 8);

                    FMythicScheduleFragment ScheduleData;
                    ScheduleData.Phase = EMythicSchedulePhase::Idle;
                    ScheduleData.HomeCell = CandidateCell;
                    ScheduleData.WorkCell = CandidateCell;

                    FMythicSignificanceFragment SignificanceData;
                    SignificanceData.Tier = EMythicSignificanceTier::Tier0_Ambient;

                    // Deferred build: reserve → build with fragments → add tag
                    Context.Defer().PushCommand<FMassCommandBuildEntity<FMythicIdentityFragment, FMythicScheduleFragment, FMythicSignificanceFragment>>(
                        NewEntity, MoveTemp(IdentityData), MoveTemp(ScheduleData), MoveTemp(SignificanceData)
                        );
                    Context.Defer().AddTag<FMythicNPCTag>(NewEntity);
                }

                SpawnBudget -= ToSpawn;
            }
        }
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

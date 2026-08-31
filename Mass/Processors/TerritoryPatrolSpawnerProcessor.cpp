
#include "Mass/Processors/TerritoryPatrolSpawnerProcessor.h"
#include "MassEntitySubsystem.h"
#include "MassExecutionContext.h"
#include "MassCommandBuffer.h"
#include "MassCommands.h"
#include "Mass/Fragments/MythicMassFragments.h"
#include "Mass/Tags/MythicMassTags.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/Territory/MythicBiome.h"
#include "World/LivingWorld/Settlements/MythicSettlement.h"
#include "World/LivingWorld/NPCGeneration/NPCGenerator.h"
#include "World/LivingWorld/MythicTags_LivingWorld.h"
#include "World/LivingWorld/Persistence/PersistentNPCRegistry.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

UMythicTerritoryPatrolSpawnerProcessor::UMythicTerritoryPatrolSpawnerProcessor() {
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
    ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);
    bRequiresGameThreadExecution = true;
    bAutoRegisterWithProcessingPhases = true;

    QueryBasedPruning = EMassQueryBasedPruning::Never;

    ExecutionOrder.ExecuteAfter.Add(TEXT("UMythicPopulationSpawnerProcessor"));

    ExistingNPCQuery.RegisterWithProcessor(*this);
}

void UMythicTerritoryPatrolSpawnerProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) {
    ExistingNPCQuery.AddRequirement<FMythicIdentityFragment>(EMassFragmentAccess::ReadOnly);
    ExistingNPCQuery.AddRequirement<FMythicSignificanceFragment>(EMassFragmentAccess::ReadOnly);
    ExistingNPCQuery.AddTagRequirement<FMythicNPCTag>(EMassFragmentPresence::All);
    ExistingNPCQuery.AddTagRequirement<FMythicEncounterEntityTag>(EMassFragmentPresence::None);
}

void UMythicTerritoryPatrolSpawnerProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicTerritoryPatrolSpawner_Execute);

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
    if (TimeSinceLastTick < Settings->TerritoryPatrolSpawnIntervalSeconds) {
        return;
    }
    TimeSinceLastTick = 0.0f;

    UMythicTerritoryGrid *Grid = LWS->GetTerritoryGrid();
    UMythicFactionDatabase *FactionDB = LWS->GetFactionDatabase();
    if (!Grid || !FactionDB) {
        return;
    }

    UMythicPersistentNPCRegistry *PersistentRegistry = LWS->GetPersistentNPCRegistry();
    if (!PersistentRegistry) {
        return;
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

    TMap<FMythicCellCoord, int32> CellEntityCounts;
    ExistingNPCQuery.ForEachEntityChunk(Context, [&CellEntityCounts](FMassExecutionContext &ChunkContext) {
        const int32 NumEntities = ChunkContext.GetNumEntities();
        const auto IdentityView = ChunkContext.GetFragmentView<FMythicIdentityFragment>();
        for (int32 i = 0; i < NumEntities; ++i) {
            CellEntityCounts.FindOrAdd(IdentityView[i].Cell)++;
        }
    });

    const float SpawnRadius = Settings->TerritoryPatrolSpawnRadius;
    const float SpawnRadiusSq = FMath::Square(SpawnRadius);
    const int32 SpawnRadiusCells = FMath::CeilToInt(SpawnRadius);
    int32 SpawnBudget = Settings->MaxPatrolSpawnsPerTick;

    struct FMythicTerritorySpawnData {
        FMythicIdentityFragment Identity;
        FMythicScheduleFragment Schedule;
        FMythicSignificanceFragment Significance;
    };
    TArray<FMythicTerritorySpawnData> SoldierSpawnData;
    TArray<FMythicTerritorySpawnData> TravelerSpawnData;
    SoldierSpawnData.Reserve(SpawnBudget);

    constexpr uint32 TravelerChanceSalt = 0x54726176u;

    TSet<FMythicCellCoord> ConsideredCells;
    for (const FMythicCellCoord &PlayerCell : PlayerCells) {
        for (int32 DY = -SpawnRadiusCells; DY <= SpawnRadiusCells && SpawnBudget > 0; ++DY) {
            for (int32 DX = -SpawnRadiusCells; DX <= SpawnRadiusCells && SpawnBudget > 0; ++DX) {
                if ((DX * DX + DY * DY) > SpawnRadiusSq) {
                    continue;
                }

                const FMythicCellCoord CandidateCell(PlayerCell.X + DX, PlayerCell.Y + DY);
                if (!Grid->IsValidCoord(CandidateCell)) {
                    continue;
                }

                if (ConsideredCells.Contains(CandidateCell)) {
                    continue;
                }
                ConsideredCells.Add(CandidateCell);

                const FMythicTerritoryCell TC = Grid->GetCell(CandidateCell);

                if (!TC.DominantFaction.IsValid() || TC.bPlayerOwned) {
                    continue;
                }

                FMythicSettlementData SettlementScratch;
                if (LWS->CopySettlementAtCell(CandidateCell, SettlementScratch)) {
                    continue;
                }

                FMythicFactionData FactionData;
                if (!FactionDB->GetFaction(TC.DominantFaction, FactionData)) {
                    continue;
                }
                if (!FactionData.bAlive || FactionData.Status != EMythicFactionStatus::Active) {
                    continue;
                }

                const EMythicBiome Biome = Grid->GetBiomeAtCell(CandidateCell);
                const float BiomeMod = BiomeGarrisonModifier(Biome);

                const int32 BaseSoldierTarget = ComputeTerritoryPatrolDensity(
                    FactionData.MilitaryStrength, TC.Influence,
                    Settings->MaxSoldiersPerControlledCell, Settings->MaxEntitiesPerCell);

                int32 SoldierTarget = FMath::CeilToInt(static_cast<float>(BaseSoldierTarget) * BiomeMod);
                SoldierTarget = FMath::Clamp(SoldierTarget, 0, FMath::Min(Settings->MaxSoldiersPerControlledCell, Settings->MaxEntitiesPerCell));

                bool bContestedBorder = false;
                {
                    static const FMythicCellCoord NeighborOffsets[4] = {
                        FMythicCellCoord(1, 0), FMythicCellCoord(-1, 0),
                        FMythicCellCoord(0, 1), FMythicCellCoord(0, -1)};
                    for (const FMythicCellCoord &Offset : NeighborOffsets) {
                        const FMythicCellCoord NeighborCell(CandidateCell.X + Offset.X, CandidateCell.Y + Offset.Y);
                        const FMythicTerritoryCell NeighborTC = Grid->GetCell(NeighborCell);

                        if (!NeighborTC.DominantFaction.IsValid() ||
                            NeighborTC.DominantFaction == TC.DominantFaction) {
                            continue;
                        }

                        if (FactionDB->GetRelationship(TC.DominantFaction, NeighborTC.DominantFaction) ==
                            EMythicFactionRelation::Hostile) {
                            bContestedBorder = true;
                            break;
                        }
                    }
                }

                SoldierTarget = ApplyContestedBorderBoost(
                    SoldierTarget, bContestedBorder, Settings->ContestedBorderSoldierMultiplier,
                    Settings->MaxSoldiersPerControlledCell, Settings->MaxEntitiesPerCell);

                const int32 CurrentCount = CellEntityCounts.FindRef(CandidateCell);
                const int32 SoldierDeficit = SoldierTarget - CurrentCount;
                const int32 SoldiersToSpawn = (SoldierDeficit > 0) ? FMath::Min(SoldierDeficit, SpawnBudget) : 0;

                for (int32 SpawnIdx = 0; SpawnIdx < SoldiersToSpawn; ++SpawnIdx) {
                    FMythicTerritorySpawnData Data;
                    Data.Identity.Faction = TC.DominantFaction;
                    Data.Identity.Cell = CandidateCell;

                    const int32 SpawnSerial = PersistentRegistry->AllocateNameSeedSerial();
                    Data.Identity.NameSeed = FMythicNPCGenerator::GenerateNameHash(
                        TC.DominantFaction.Index, CandidateCell, SpawnSerial);
                    Data.Identity.EntityId = PersistentRegistry->AllocateEntityIdentity(
                        Data.Identity.NameSeed,
                        EMythicEntityIdentityProvenance::TerritoryPatrol);
                    if (!Data.Identity.EntityId.IsValid()) {
                        continue;
                    }
                    Data.Identity.VisualArchetype = FMythicNPCGenerator::GenerateVisualArchetype(Data.Identity.NameSeed, 8);
                    Data.Identity.DemographicFlags = FMythicNPCGenerator::GenerateDemographicFlags(
                        Data.Identity.NameSeed, FactionData.Population > 50);

                    Data.Identity.RoleTag = Settings->SoldierRoleTag.IsValid() ? Settings->SoldierRoleTag : TAG_NPC_ROLE_SOLDIER;

                    Data.Schedule.Phase = EMythicSchedulePhase::Idle;
                    Data.Schedule.HomeCell = CandidateCell;
                    Data.Schedule.WorkCell = CandidateCell;

                    Data.Significance.Tier = EMythicSignificanceTier::Tier0_Ambient;

                    SoldierSpawnData.Add(MoveTemp(Data));
                }

                int32 CellSpawned = SoldiersToSpawn;

                if (CellSpawned < SpawnBudget) {
                    const uint32 TravelerSeed = FMythicNPCGenerator::GenerateNameHash(
                        TC.DominantFaction.Index, CandidateCell, static_cast<int32>(TravelerChanceSalt));
                    const float TravelerRoll = static_cast<float>(TravelerSeed & 0xFFFFFFu) / 16777216.0f;

                    const int32 PostTraveler = CurrentCount + CellSpawned;
                    if (TravelerRoll < Settings->TravelerSpawnChancePerCell &&
                        PostTraveler < Settings->MaxEntitiesPerCell) {
                        FMythicTerritorySpawnData Data;
                        Data.Identity.Faction = TC.DominantFaction;
                        Data.Identity.Cell = CandidateCell;

                        const int32 SpawnSerial = PersistentRegistry->AllocateNameSeedSerial();
                        Data.Identity.NameSeed = FMythicNPCGenerator::GenerateNameHash(
                            TC.DominantFaction.Index, CandidateCell, SpawnSerial);
                        Data.Identity.EntityId = PersistentRegistry->AllocateEntityIdentity(
                            Data.Identity.NameSeed,
                            EMythicEntityIdentityProvenance::TerritoryTraveler);
                        if (!Data.Identity.EntityId.IsValid()) {
                            continue;
                        }
                        Data.Identity.VisualArchetype = FMythicNPCGenerator::GenerateVisualArchetype(Data.Identity.NameSeed, 8);
                        Data.Identity.DemographicFlags = FMythicNPCGenerator::GenerateDemographicFlags(
                            Data.Identity.NameSeed, FactionData.Population > 50);
                        Data.Identity.RoleTag = Settings->TravelerRoleTag.IsValid() ? Settings->TravelerRoleTag : TAG_NPC_ROLE_TRAVELER;

                        Data.Schedule.Phase = EMythicSchedulePhase::Travel;
                        Data.Schedule.HomeCell = CandidateCell;
                        Data.Schedule.WorkCell = CandidateCell;

                        Data.Significance.Tier = EMythicSignificanceTier::Tier0_Ambient;

                        TravelerSpawnData.Add(MoveTemp(Data));
                        ++CellSpawned;
                    }
                }

                SpawnBudget -= CellSpawned;
            }
        }
    }

    if (SoldierSpawnData.Num() > 0) {
        Context.Defer().PushCommand<FMassDeferredCreateCommand>(
            [SoldierSpawnData](FMassEntityManager &Manager) {
                TRACE_CPUPROFILER_EVENT_SCOPE(MythicTerritoryPatrolSpawner_DeferredSoldiers);

                const UScriptStruct *Composition[] = {
                    FMythicIdentityFragment::StaticStruct(),
                    FMythicScheduleFragment::StaticStruct(),
                    FMythicSignificanceFragment::StaticStruct(),
                    FMythicNPCTag::StaticStruct(),
                    FMythicSoldierTag::StaticStruct()
                };
                FMassArchetypeHandle Archetype = Manager.CreateArchetype(MakeArrayView(Composition));

                TArray<FMassEntityHandle> Spawned;
                Manager.BatchCreateEntities(Archetype, SoldierSpawnData.Num(), Spawned);
                for (int32 i = 0; i < SoldierSpawnData.Num(); ++i) {
                    const FMythicTerritorySpawnData &D = SoldierSpawnData[i];
                    const FMassEntityHandle E = Spawned[i];
                    Manager.GetFragmentDataChecked<FMythicIdentityFragment>(E) = D.Identity;
                    Manager.GetFragmentDataChecked<FMythicScheduleFragment>(E) = D.Schedule;
                    Manager.GetFragmentDataChecked<FMythicSignificanceFragment>(E) = D.Significance;
                }
            });
    }

    if (TravelerSpawnData.Num() > 0) {
        Context.Defer().PushCommand<FMassDeferredCreateCommand>(
            [TravelerSpawnData](FMassEntityManager &Manager) {
                TRACE_CPUPROFILER_EVENT_SCOPE(MythicTerritoryPatrolSpawner_DeferredTravelers);

                const UScriptStruct *Composition[] = {
                    FMythicIdentityFragment::StaticStruct(),
                    FMythicScheduleFragment::StaticStruct(),
                    FMythicSignificanceFragment::StaticStruct(),
                    FMythicNPCTag::StaticStruct(),
                    FMythicTravelerTag::StaticStruct()
                };
                FMassArchetypeHandle Archetype = Manager.CreateArchetype(MakeArrayView(Composition));

                TArray<FMassEntityHandle> Spawned;
                Manager.BatchCreateEntities(Archetype, TravelerSpawnData.Num(), Spawned);
                for (int32 i = 0; i < TravelerSpawnData.Num(); ++i) {
                    const FMythicTerritorySpawnData &D = TravelerSpawnData[i];
                    const FMassEntityHandle E = Spawned[i];
                    Manager.GetFragmentDataChecked<FMythicIdentityFragment>(E) = D.Identity;
                    Manager.GetFragmentDataChecked<FMythicScheduleFragment>(E) = D.Schedule;
                    Manager.GetFragmentDataChecked<FMythicSignificanceFragment>(E) = D.Significance;
                }
            });
    }
}

int32 UMythicTerritoryPatrolSpawnerProcessor::ComputeTerritoryPatrolDensity(
    float MilitaryStrength, float Influence, int32 MaxSoldiersPerControlledCell, int32 MaxEntitiesPerCell) {
    if (MaxSoldiersPerControlledCell <= 0) {
        return 0;
    }

    const float StrengthFactor = FMath::Clamp(MilitaryStrength, 0.0f, 1.0f);
    const float InfluenceFactor = FMath::Clamp(Influence, 0.0f, 1.0f);
    const float Scaled = static_cast<float>(MaxSoldiersPerControlledCell) * StrengthFactor * InfluenceFactor;

    int32 Target = FMath::CeilToInt(Scaled);

    Target = FMath::Min(Target, MaxSoldiersPerControlledCell);
    Target = FMath::Min(Target, MaxEntitiesPerCell);
    return FMath::Max(Target, 0);
}

float UMythicTerritoryPatrolSpawnerProcessor::BiomeGarrisonModifier(EMythicBiome Biome) {
    switch (Biome) {
    case EMythicBiome::Plains:
        return 1.0f;
    case EMythicBiome::Mountain:
        return 1.0f;
    case EMythicBiome::Forest:
        return 0.75f;
    case EMythicBiome::Wetland:
        return 0.75f;
    case EMythicBiome::Wasteland:
        return 0.5f;
    case EMythicBiome::Desert:
        return 0.5f;
    default:
        return 1.0f;
    }
}

int32 UMythicTerritoryPatrolSpawnerProcessor::ApplyContestedBorderBoost(
    int32 BaseSoldierTarget, bool bContested, float ContestedMultiplier,
    int32 MaxSoldiersPerControlledCell, int32 MaxEntitiesPerCell) {
    if (!bContested) {
        return BaseSoldierTarget;
    }

    const float Mult = FMath::Max(ContestedMultiplier, 1.0f);
    const int32 Boosted = FMath::CeilToInt(static_cast<float>(BaseSoldierTarget) * Mult);

    const int32 Ceiling = FMath::Min(MaxSoldiersPerControlledCell, MaxEntitiesPerCell);
    return FMath::Max(0, FMath::Min(Boosted, Ceiling));
}
